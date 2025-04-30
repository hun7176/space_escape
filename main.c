#include <locale.h>
#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#define WIDTH 60
#define HEIGHT 40
#define MAX_BULLETS 10
#define MAX_OBSTACLES 20

#define SCREEN_ROWS 42
#define SCREEN_COLS 90

typedef struct {
    int x, y;
    int active;
} Bullet;

typedef struct {
    int x, y;
    int active;
} Obstacle;

int player_x;
int player_y;
int life = 3;
int score = 0;
int game_over = 0;
int paused = 0;

Bullet bullets[MAX_BULLETS];
Obstacle obstacles[MAX_OBSTACLES];

void init_colors() {
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLACK);   // 기본 텍스트
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);  // 로고/제목
    init_pair(3, COLOR_CYAN, COLOR_BLACK);    // 설명 텍스트
    init_pair(4, COLOR_BLACK, COLOR_YELLOW);  // 선택된 메뉴 강조
}

int current_key = -1;
void show_main_menu() {
    const char *menu[] = {
        ">  1인 모드 시작",
        ">  2인 모드 (미지원)",
        ">  만든이",
        ">  종료"
    };
    int choice = 0;
    int menu_size = sizeof(menu) / sizeof(menu[0]);

    while (1) {
        clear();

        // 전체 그리드 테두리 출력 (90x42)
        for (int x = 0; x < SCREEN_COLS; x++) {
            mvaddch(0, x, '#');
            mvaddch(SCREEN_ROWS - 1, x, '#');
        }
        for (int y = 0; y < SCREEN_ROWS; y++) {
            mvaddch(y, 0, '#');
            mvaddch(y, SCREEN_COLS - 1, '#');
        }

        // 가운데 기준 좌표 계산
        int center_x = SCREEN_COLS / 2;
        int base_y = SCREEN_ROWS / 2 - 8;

        attron(COLOR_PAIR(2));
        mvprintw(base_y - 2, center_x - 16, "+-----------------------------+");
        mvprintw(base_y - 1, center_x - 16, "|       SPACE ESCAPE          |");
        mvprintw(base_y + 0, center_x - 16, "|      with controller        |");
        mvprintw(base_y + 1, center_x - 16, "+-----------------------------+");
        attroff(COLOR_PAIR(2));

        for (int i = 0; i < menu_size; i++) {
            if (i == choice) {
                attron(COLOR_PAIR(4));
                mvprintw(base_y + 3 + i, center_x - 10, "%s", menu[i]);
                attroff(COLOR_PAIR(4));
            } else {
                attron(COLOR_PAIR(1));
                mvprintw(base_y + 3 + i, center_x - 10, "%s", menu[i]);
                attroff(COLOR_PAIR(1));
            }
        }

        attron(COLOR_PAIR(3));
        mvprintw(base_y + 9, center_x - 18, "↑↓ 방향키로 이동, Enter로 선택");
        mvprintw(base_y + 11, center_x - 18, "게임 시작 전 좌우 키로 캐릭터 위치 조정 가능");
        mvprintw(base_y + 12, center_x - 18, "→ 준비되었으면 Enter 키를 눌러 시작!");
        attroff(COLOR_PAIR(3));

        int ch = getch();
        switch (ch) {
            case KEY_UP:
                choice = (choice - 1 + menu_size) % menu_size;
                break;
            case KEY_DOWN:
                choice = (choice + 1) % menu_size;
                break;
            case KEY_LEFT:
                if (choice == 0 && player_x > 1) player_x--;
                break;
            case KEY_RIGHT:
                if (choice == 0 && player_x < WIDTH - 2) player_x++;
                break;
            case '\n':
            case ' ':
                if (choice == 0) return;  // 1인 모드 시작
                else if (choice == 1) {
                    mvprintw(base_y + 14, center_x - 18, "2인 모드는 준비 중입니다.");
                    refresh(); usleep(1000000);
                } else if (choice == 2) {
                    mvprintw(base_y + 14, center_x - 18, "제작자: 전준형 (경북대 컴퓨터학부)");
                    mvprintw(base_y + 15, center_x - 18, "제작자: 최승헌 (경북대 전자공학부)");
                    refresh(); usleep(3000000);
                } else if (choice == 3) {
                    endwin();
                    printf("게임을 종료합니다.\n");
                    exit(0);
                }
                break;
        }
        refresh();
    }
}



void func_pause(int sig) {
    signal(SIGTSTP, SIG_IGN);
    int mid_x = WIDTH / 2;
    int mid_y = HEIGHT / 2;
    mvprintw(mid_y - 1, mid_x - 7, "== PAUSED ==");
    mvprintw(mid_y, mid_x - 15, "Press 'p' or Ctrl+Z to resume");
    refresh();

    timeout(-1);
    while (1) {
        int ch = getch();
        if (ch == 'p' || ch == 'P' || ch == 26) break;
    }
    timeout(50);
    signal(SIGTSTP, func_pause);
}

void end_by_signal(int sig) {
    int mid_x = WIDTH / 2;
    int mid_y = HEIGHT / 2;
    mvprintw(mid_y, mid_x - 18, "Are you sure you want to quit? (y/n)");
    refresh();

    timeout(-1);
    int ch = getch();
    timeout(50);

    if (ch == 'y' || ch == 'Y') {
        endwin();
        printf("Terminated by user. Final Score: %d\n", score);
        exit(0);
    }
}

void init_game() {
    initscr();
    noecho();
    curs_set(FALSE);
    timeout(50);
    keypad(stdscr, TRUE);
    srand(time(NULL));
    player_x = WIDTH / 2;
    player_y = HEIGHT - 2;

    for (int i = 0; i < MAX_BULLETS; i++) bullets[i].active = 0;
    for (int i = 0; i < MAX_OBSTACLES; i++) obstacles[i].active = 0;

    signal(SIGTSTP, func_pause);
    signal(SIGINT, end_by_signal);
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


    for (int i = 0; i < MAX_BULLETS; i++)
        if (bullets[i].active)
            mvprintw(bullets[i].y, bullets[i].x, "|");

    for (int i = 0; i < MAX_OBSTACLES; i++)
        if (obstacles[i].active)
            mvprintw(obstacles[i].y, obstacles[i].x, "*");

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
        case '5':  // 정지 or 대기 (사용 X)
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


int main() {
    setlocale(LC_ALL, "");
    initscr();
    noecho();
    curs_set(FALSE);
    keypad(stdscr, TRUE);
    timeout(-1);  // 메뉴는 블로킹

    init_colors();
    show_main_menu();

    endwin();  // 메뉴 후 게임 시작 전 화면 초기화
    init_game();
    int frame = 0;

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
    return 0;
}
