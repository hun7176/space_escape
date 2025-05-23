#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <locale.h>
#include <signal.h>
#include <unistd.h>
#include "game.h"
#include "game_parameter.h"   // WIDTH, HEIGHT, MAX_BULLETS, MAX_OBSTACLES 등 정의
#include "intro_ui.h"          // init_colors(), show_main_menu(), func_pause(), end_by_signal(), 등

// 전역 게임 변수들
int player_x, player_y;           // 플레이어 위치
int life = 3;                     // 남은 목숨
int score = 0;                    // 점수
int game_over = 0;                // 게임 종료 플래그
int paused = 0;                   // 일시정지 플래그
int current_key = -1;             // 입력 키 저장 (미사용)
Bullet bullets[MAX_BULLETS];      // 발사체 배열
Obstacle obstacles[MAX_OBSTACLES];// 장애물 배열

void init_game() {
    initscr();
    noecho();
    curs_set(FALSE);
    timeout(50);
    keypad(stdscr, TRUE);
    srand(time(NULL));
    player_x = WIDTH / 2;
    player_y = HEIGHT - 2;

    // 발사체와 장애물 초기화
    for (int i = 0; i < MAX_BULLETS; i++) {
        bullets[i].active = 0;
    }
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        obstacles[i].active = 0;
    }
    
    // 시그널 핸들러 등록 (필요한 경우)
    //signal(SIGTSTP, func_pause);
    //signal(SIGINT, end_by_signal);
    
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    if (rows < HEIGHT + 2 || cols < WIDTH + 30) {
        endwin();
        printf("터미널 창 크기가 너무 작습니다.\n");
        printf("최소 %d행 x %d열 이상 필요합니다.\n", HEIGHT + 2, WIDTH + 30);
        exit(1);
    }
}

void end_game() {
    endwin();
    printf("Game Over! Score: %d\n", score);
}

void draw_border() {
    for (int i = 0; i < WIDTH; i++) {
        mvprintw(0, i, "#");
        mvprintw(HEIGHT - 1, i, "#");
    }
    for (int i = 0; i < HEIGHT; i++) {
        mvprintw(i, 0, "#");
        mvprintw(i, WIDTH - 1, "#");
    }
}

void draw_life() {
    mvprintw(1, WIDTH - 20, "Life: ");
    for (int i = 0; i < life; i++) {
        printw("O");
    }
}

void draw_ui() {
    int base_x = WIDTH + 2;  // 게임 맵 오른쪽 바깥쪽 시작
    mvprintw(2, base_x, "== SPACE ESCAPE ==");
    mvprintw(4, base_x, "Life : %03d", life);
    mvprintw(5, base_x, "Score: %d", score);
    mvprintw(7, base_x, "[ Controls ]");
    mvprintw(8, base_x, "Move   : Numpad 1~9");
    mvprintw(9, base_x, "         ↖↑↗");
    mvprintw(10, base_x, "         ←5→");
    mvprintw(11, base_x, "         ↙↓↘");
    mvprintw(12, base_x, "Shoot  : [Space]");
    mvprintw(13, base_x, "Pause  : [p] or Ctrl+Z");
    mvprintw(14, base_x, "Exit   : Ctrl+C");
}

void draw_objects() {
    mvprintw(player_y, player_x, "A");

    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active)
            mvprintw(bullets[i].y, bullets[i].x, "|");
    }

    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (obstacles[i].active)
            mvprintw(obstacles[i].y, obstacles[i].x, "*");
    }

    draw_life();
    draw_ui();
    mvprintw(HEIGHT, 0, "Score: %d", score);
}

void spawn_obstacle() {
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (!obstacles[i].active) {
            obstacles[i].active = 1;
            obstacles[i].x = rand() % (WIDTH - 22) + 1;
            obstacles[i].y = 1;
            break;
        }
    }
}

void update_bullets() {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {
            bullets[i].y--;
            if (bullets[i].y <= 0)
                bullets[i].active = 0;
        }
    }
}

void update_obstacles() {
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (obstacles[i].active) {
            obstacles[i].y++;
            if (obstacles[i].y >= HEIGHT - 2) {
                life--;
                obstacles[i].active = 0;
                if (life <= 0) game_over = 1;
            }
        }
    }
}

void check_collisions() {
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (!obstacles[i].active) continue;
        for (int j = 0; j < MAX_BULLETS; j++) {
            if (bullets[j].active &&
                bullets[j].x == obstacles[i].x &&
                bullets[j].y == obstacles[i].y) {
                bullets[j].active = 0;
                obstacles[i].active = 0;
                score += 10;
                break;
            }
        }
    }
}

void handle_input() {
    int ch = getch();
    switch (ch) {
        case '7':  // ↖
            if (player_x > 1) player_x--;
            if (player_y > 1) player_y--;
            break;
        case '8':  // ↑
            if (player_y > 1) player_y--;
            break;
        case '9':  // ↗
            if (player_x < WIDTH - 2) player_x++;
            if (player_y > 1) player_y--;
            break;
        case '4':  // ←
            if (player_x > 1) player_x--;
            break;
        case '5':  // 정지 (사용 X)
            break;
        case '6':  // →
            if (player_x < WIDTH - 2) player_x++;
            break;
        case '1':  // ↙
            if (player_x > 1) player_x--;
            if (player_y < HEIGHT - 2) player_y++;
            break;
        case '2':  // ↓
            if (player_y < HEIGHT - 2) player_y++;
            break;
        case '3':  // ↘
            if (player_x < WIDTH - 2) player_x++;
            if (player_y < HEIGHT - 2) player_y++;
            break;
        case ' ':
            for (int i = 0; i < MAX_BULLETS; i++) {
                if (!bullets[i].active) {
                    bullets[i].active = 1;
                    bullets[i].x = player_x;
                    bullets[i].y = player_y - 1;
                    break;
                }
            }
            break;
        case 'p':
            paused = !paused;
            break;
        default:
            break;
    }
}

// run_game() 함수는 스레드에서 호출되어 게임 루프 전체를 실행합니다.
void* run_game(void* arg) {
    (void)arg;
    setlocale(LC_ALL, "");
    // 메뉴를 블로킹 방식으로 보여줌
    initscr();
    noecho();
    curs_set(FALSE);
    keypad(stdscr, TRUE);
    timeout(-1);
    
    init_colors();
    show_main_menu();
    
    endwin();  // 메뉴 종료 후 화면 리셋

    // 게임 초기화
    init_game();
    int frame = 0;
    
    // 게임 루프
    while (!game_over) {
        if (!paused) {
            clear();
            draw_border();
            draw_objects();
            handle_input();
            update_bullets();
            if (frame % 20 == 0) update_obstacles();
            check_collisions();
            if (frame++ % 10 == 0) spawn_obstacle();
            refresh();
            usleep(30000);
        } else {
            mvprintw(HEIGHT / 2, WIDTH / 2 - 5, "Paused");
            refresh();
            usleep(100000);
        }
    }
    
    end_game();
    return NULL;
}