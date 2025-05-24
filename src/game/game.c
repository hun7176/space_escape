#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <locale.h>
#include <signal.h>
#include <unistd.h>
#include "game.h"
#include "game_parameter.h"   // WIDTH, HEIGHT, MAX_BULLETS, MAX_OBSTACLES 등 정의
#include "intro_ui.h"          // init_colors(), show_main_menu(), func_pause(), end_by_signal(), 등
#include "common.h"

pthread_mutex_t score_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t render_mutex = PTHREAD_MUTEX_INITIALIZER;  // 렌더링 보호용 뮤텍스

// 전역 게임 변수들
int player_x, player_y;           // 플레이어 위치
int life = 3;                     // 남은 목숨
int score = 0;                    // 점수
int game_over = 0;                // 게임 종료 플래그
int paused = 0;                   // 일시정지 플래그
int current_key = -1;             // 입력 키 저장 (미사용)
Bullet bullets[MAX_BULLETS];      // 발사체 배열
Obstacle obstacles[MAX_OBSTACLES];// 장애물 배열

//게임 난이도 조정을 위한 변수들
int level = 1;                     // 현재 레벨
int obstacle_interval = 20;        // 장애물 이동 주기 (프레임 단위)
int spawn_interval    = 10;        // 장애물 생성 주기 (프레임 단위)


//더블 버퍼링을 위한 변수들
/**
 * game_win, un_win으로 화면을 분리하여 각각 독립적으로 업데이트
 * 깜빡임 최소화를 위함
 */
WINDOW *game_win = NULL;
WINDOW *ui_win = NULL;
static int last_score = -1;
static int last_opponent_score = -1;

int get_opponent_score() {
    int score;
    pthread_mutex_lock(&network_mutex);
    score = opponent_score;
    pthread_mutex_unlock(&network_mutex);
    return score;
}

void init_game() {
    initscr();
    clear();
    refresh();


    noecho();
    curs_set(FALSE);
    timeout(0);
    keypad(stdscr, TRUE);
    srand(time(NULL));

    
    //기존 윈도우 있으면 삭제
    if(game_win){
        delwin(game_win);
        game_win = NULL;
    }
    if(ui_win){
        delwin(ui_win);
        ui_win = NULL;
    }

    erase();
    refresh();

    //더블 버퍼링 윈도우
    game_win = newwin(HEIGHT, WIDTH, 0, 0);
    ui_win = newwin(HEIGHT, 30, 0, WIDTH + 2);

    nodelay(game_win, TRUE);
    nodelay(ui_win, TRUE);
    keypad(game_win, TRUE);

    //윈도우 초기화
    werase(game_win);
    werase(ui_win);
    wrefresh(game_win);
    wrefresh(ui_win);

    player_x = WIDTH / 2;
    player_y = HEIGHT - 2;


    //발사체와 장애물 초기화
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

    refresh();
}

void end_game() {
    if (game_win) {
        delwin(game_win);
        game_win = NULL;
    }
    if (ui_win) {
        delwin(ui_win);
        ui_win = NULL;
    }
    
    //화면 완전 클리어 후 종료
    clear();
    refresh();
    endwin();
    
    printf("Game Over! Score: %d\n", score);
}

// void draw_border() {
//     for (int i = 0; i < WIDTH; i++) {
//         mvprintw(0, i, "#");
//         mvprintw(HEIGHT - 1, i, "#");
//     }
//     for (int i = 0; i < HEIGHT; i++) {
//         mvprintw(i, 0, "#");
//         mvprintw(i, WIDTH - 1, "#");
//     }
// }

// void draw_life() {
//     mvprintw(1, WIDTH - 20, "Life: ");
//     for (int i = 0; i < life; i++) {
//         printw("O");
//     }
// }

// 게임 영역만 그리기 (더블 버퍼링)
void draw_game_area() {
    // 게임 윈도우 클리어
    werase(game_win);
    
    // 테두리 그리기
    for (int i = 0; i < WIDTH; i++) {
        mvwprintw(game_win, 0, i, "#");
        mvwprintw(game_win, HEIGHT - 1, i, "#");
    }
    for (int i = 0; i < HEIGHT; i++) {
        mvwprintw(game_win, i, 0, "#");
        mvwprintw(game_win, i, WIDTH - 1, "#");
    }
    
    // 플레이어 그리기
    mvwprintw(game_win, player_y, player_x, "A");
    
    // 발사체 그리기
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active)
            mvwprintw(game_win, bullets[i].y, bullets[i].x, "|");
    }
    
    // 장애물 그리기
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (obstacles[i].active)
            mvwprintw(game_win, obstacles[i].y, obstacles[i].x, "*");
    }
    
    // 라이프 표시
    mvwprintw(game_win, 1, WIDTH - 20, "Life: ");
    for (int i = 0; i < life; i++) {
        wprintw(game_win, "O");
    }
}

// void draw_ui() {
//     int base_x = WIDTH + 2;  // 게임 맵 오른쪽 바깥쪽 시작
//     mvprintw(2, base_x, "== SPACE ESCAPE ==");
//     mvprintw(4, base_x, "Life : %03d", life);
//     mvprintw(5, base_x, "Score: %d", score);
//     mvprintw(6, base_x, "Bullet: %d", MAX_BULLETS);
//     mvprintw(7, base_x, "LEVEL: %d", level);

//     mvprintw(10, base_x, "[ Controls ]");
//     mvprintw(11, base_x, "Move   : Numpad 1~9");
//     mvprintw(12, base_x, "         ↖↑↗");
//     mvprintw(13, base_x, "         ←5→");
//     mvprintw(14, base_x, "         ↙↓↘");
//     mvprintw(15, base_x, "Shoot  : [Space]");
//     mvprintw(16, base_x, "Pause  : [p] or Ctrl+Z");
//     mvprintw(17, base_x, "Exit   : Ctrl+C");
// }

// UI 영역만 그리기 (필요할 때만 업데이트)
void draw_ui_area() {
    int current_score;
    int current_opponent_score;
    pthread_mutex_lock(&score_mutex);
    current_score = score;
    pthread_mutex_unlock(&score_mutex);
    
    pthread_mutex_lock(&network_mutex);
    current_opponent_score = opponent_score;
    pthread_mutex_unlock(&network_mutex);

    // 점수가 변경되었을 때만 UI 업데이트
    if (current_score != last_score || current_opponent_score != last_opponent_score) {
        werase(ui_win);
        
        //게임모드에 따라 표시
        if (current_game_mode == GAME_MODE_SINGLE) {
            mvwprintw(ui_win, 1, 0, "== 1인 모드 ==");
        } else {
            mvwprintw(ui_win, 1, 0, "== 2인 모드 ==");
        }

        mvwprintw(ui_win, 2, 0, "== SPACE ESCAPE ==");
        mvwprintw(ui_win, 4, 0, "Life : %03d", life);
        mvwprintw(ui_win, 5, 0, "Score: %d", current_score);
        mvwprintw(ui_win, 6, 0, "Bullet: %d", MAX_BULLETS);
        mvwprintw(ui_win, 7, 0, "LEVEL: %d", level);

        mvwprintw(ui_win, 10, 0, "[ Controls ]");
        mvwprintw(ui_win, 11, 0, "Move   : Numpad 1~9");
        mvwprintw(ui_win, 12, 0, "         ↖↑↗");
        mvwprintw(ui_win, 13, 0, "         ←5→");
        mvwprintw(ui_win, 14, 0, "         ↙↓↘");
        mvwprintw(ui_win, 15, 0, "Shoot  : [Space]");
        mvwprintw(ui_win, 16, 0, "Pause  : [p] or Ctrl+Z");
        mvwprintw(ui_win, 17, 0, "Exit   : Ctrl+C");
        
        //2인 모드일 때 추가 정보 표시
        if (current_game_mode == GAME_MODE_MULTI) {
            mvwprintw(ui_win, 19, 0, "Opponent: %d", current_opponent_score);
        }

        last_score = current_score;
        last_opponent_score = current_opponent_score;
    }
}

// void draw_objects() {
//     mvprintw(player_y, player_x, "A");

//     for (int i = 0; i < MAX_BULLETS; i++) {
//         if (bullets[i].active)
//             mvprintw(bullets[i].y, bullets[i].x, "|");
//     }

//     for (int i = 0; i < MAX_OBSTACLES; i++) {
//         if (obstacles[i].active)
//             mvprintw(obstacles[i].y, obstacles[i].x, "*");
//     }

//     draw_life();
//     draw_ui();
//     mvprintw(HEIGHT, 0, "Score: %d", score);
// }

//장애물 생성 -> LEVEL이 증가할수록 빠르게 생성되거나 내려오도록
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

//mutex사용, score보호
void check_collisions() {
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (!obstacles[i].active) continue;
        for (int j = 0; j < MAX_BULLETS; j++) {
            if (bullets[j].active &&
                bullets[j].x == obstacles[i].x &&
                bullets[j].y == obstacles[i].y) {
                bullets[j].active = 0;
                obstacles[i].active = 0;
                pthread_mutex_lock(&score_mutex);
                score += 10;
                pthread_mutex_unlock(&score_mutex);
                break;
            }
        }
    }
}

void handle_input() {
    int ch = wgetch(game_win);  // 게임 윈도우에서 입력 받기
    if (ch == ERR) return;  // 입력이 없으면 바로 리턴
    //int ch = getch();
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


    //메뉴를 블로킹 방식으로 보여줌
    // initscr();
    // noecho();
    // curs_set(FALSE);
    // keypad(stdscr, TRUE);
    // timeout(-1);
    
    // init_colors();
    // show_main_menu();
    
    // endwin();  //메뉴 종료 후 화면 리셋
    // usleep(100000);
    
    // 게임 초기화
    init_game();
    int frame = 0;
    
    // 게임 루프 (루프 한번 돌때마다 frame이 증가함.)

    while (!game_over) {
        if (!paused) {
            //렌더링 뮤텍스로 보호
            pthread_mutex_lock(&render_mutex);

            handle_input();
            update_bullets();
            //20프레임마다 obstacle위치 update
            //10프레임마다 obstacle 생성 실행
            if (frame % obstacle_interval == 0) update_obstacles();
            if (frame % spawn_interval    == 0) spawn_obstacle();
           
            
            check_collisions();

            //화면 그리기 (더블 버퍼링)
            draw_game_area();
            draw_ui_area();


            //윈도우 새로고침
            wnoutrefresh(game_win);
            wnoutrefresh(ui_win);
            doupdate();  //모든 윈도우를 한번에 업데이트

            pthread_mutex_unlock(&render_mutex);

            

            frame++;

            //  --- 레벨업 조건 (예: 30초마다) ---
            // 30초 * (1초당 약 33프레임) ≒ 1000프레임
            // 현재 15초
            if (frame % 500 == 0) {
                level++;
                // 난이도 조절: 최소값을 지정해서 너무 빨라지지 않도록
                obstacle_interval = (obstacle_interval > 5)
                    ? obstacle_interval - 2
                    : 5;
                spawn_interval = (spawn_interval > 3)
                    ? spawn_interval - 1
                    : 3;
            }

        } else {
            // mvprintw(HEIGHT / 2, WIDTH / 2 - 5, "Paused");
            // refresh();
            // usleep(100000);
            pthread_mutex_lock(&render_mutex);
            mvwprintw(game_win, HEIGHT / 2, WIDTH / 2 - 5, "Paused");
            wnoutrefresh(game_win);
            doupdate();
            pthread_mutex_unlock(&render_mutex);
        }
        usleep(30000);
    }
    
    end_game();
    return NULL;
}