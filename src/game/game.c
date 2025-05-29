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
#include "controller.h"
pthread_mutex_t score_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t render_mutex = PTHREAD_MUTEX_INITIALIZER;  // 렌더링 보호용 뮤텍스

//컨트롤러용
char imu_direction = '5';  // 초기 방향
pthread_mutex_t imu_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_t imu_thread;


extern int seed; //network로 부여받은 seed

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

// 게임 대기 상태 변수 추가
int waiting_for_opponent = 0;     // 상대방 종료 대기 중인지 표시

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

int debug_init_controller=0;
void init_controller(){
    int imu_fd = init_adxl345();
    if (imu_fd >= 0) {
        create_thread(&imu_thread, imu_thread_func, &imu_fd);
    }
    debug_init_controller=1;
}

void init_game() {
    initscr();
    clear();
    refresh();


    noecho();
    curs_set(FALSE);
    timeout(0);
    keypad(stdscr, TRUE);
    srand(seed);
    //디버깅
    printf("[client] seed: %d\n", seed);
    
    
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

    if(current_game_mode == GAME_MODE_SINGLE){

        clear();
        mvprintw(5, 10, "=== 게임 결과 ===");
        mvprintw(7, 10, "당신의 점수: %d", score);

        refresh();
        usleep(5000000);

    }
    //게임 종료시 스레드 종료조건인 is_running = 0
    extern volatile int is_running;
    is_running = 0;
    printf("\nGame Over! Score: %d\n", score);
}


// 대기 화면 그리기
void draw_waiting_screen() {
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
    
    // 대기 메시지 박스
    int box_width = 40;
    int box_height = 8;
    int start_x = (WIDTH - box_width) / 2;
    int start_y = (HEIGHT - box_height) / 2;
    
    // 박스 테두리
    for (int i = 0; i < box_width; i++) {
        mvwprintw(game_win, start_y, start_x + i, "=");
        mvwprintw(game_win, start_y + box_height - 1, start_x + i, "=");
    }
    for (int i = 0; i < box_height; i++) {
        mvwprintw(game_win, start_y + i, start_x, "|");
        mvwprintw(game_win, start_y + i, start_x + box_width - 1, "|");
    }
    
    // 메시지 내용
    mvwprintw(game_win, start_y + 2, start_x + 2, "게임 종료!");
    mvwprintw(game_win, start_y + 3, start_x + 2, "당신의 점수: %d", score);
    mvwprintw(game_win, start_y + 4, start_x + 2, "상대방 점수: %d", get_opponent_score());
    mvwprintw(game_win, start_y + 5, start_x + 2, "상대방이 게임을 끝낼 때까지 대기 중...");
    
    // 애니메이션용 점들 (선택사항)
    static int dot_count = 0;
    dot_count = (dot_count + 1) % 4;
    for (int i = 0; i < dot_count; i++) {
        wprintw(game_win, ".");
    }
}


// 게임 영역만 그리기 (더블 버퍼링)
void draw_game_area() {

    // 대기 중이면 대기 화면 그리기
    if (waiting_for_opponent) {
        draw_waiting_screen();
        return;
    }

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
            mvwprintw(ui_win, 0, 0, "Seed : %d", seed);
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

        if (waiting_for_opponent) {
            mvwprintw(ui_win, 21, 0, "Status: Waiting...");
        }
        
        mvwprintw(ui_win, 24, 0, "controller init : %d",debug_init_controller);
        
        mvwprintw(ui_win, 25, 0, "ch : %c", imu_direction);
        //mvwprintw(ui_win, 25, 0, "Life : %03d", life);

        last_score = current_score;
        last_opponent_score = current_opponent_score;
    }
}


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
                if (life <= 0){
                    game_over = 1;
                    if(current_game_mode == GAME_MODE_SINGLE){
                        extern volatile int game_result_received;
                        game_result_received = 1;
                    }
                } 
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
    

    pthread_mutex_lock(&imu_lock);
    char ch = imu_direction;
    pthread_mutex_unlock(&imu_lock);

    //대기 중이면 입력 무시
    if (waiting_for_opponent) {
        wgetch(game_win); // 입력 버퍼 비우기
        return;
    }
    //int ch = wgetch(game_win);  // 게임 윈도우에서 입력 받기
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
    init_controller();
    int frame = 0;
    
    // 게임 루프 (루프 한번 돌때마다 frame이 증가함.)
    extern volatile int game_result_received;

    while (!game_result_received) { //network.c의 handle_game_result까지 (모든 클라이언트가 끝나야만 호출됨)
        if (!paused) {
            //렌더링 뮤텍스로 보호
            pthread_mutex_lock(&render_mutex);

            //게임이 끝났지만 아직 결과를 받지 못한 경우
            if (game_over && current_game_mode == GAME_MODE_MULTI && !waiting_for_opponent) {
                waiting_for_opponent = 1;  //대기 상태로 전환
            }
            //대기 중이 아닐땐 동작제어가능
            if (!waiting_for_opponent) {
                handle_input();
                update_bullets();
                
                if (frame % obstacle_interval == 0) update_obstacles();
                if (frame % spawn_interval == 0) spawn_obstacle();
                
                check_collisions();
                
                frame++;
                
                // 레벨업 조건
                if (frame % 500 == 0) {
                    level++;
                    obstacle_interval = (obstacle_interval > 5) ? obstacle_interval - 2 : 5;
                    spawn_interval = (spawn_interval > 3) ? spawn_interval - 1 : 3;
                }
            } else {
                // 대기 중에도 입력은 처리 (버퍼 비우기 위해)
                handle_input();
            }

            // 화면 그리기 (대기 중이든 아니든)
            draw_game_area();
            draw_ui_area();

            // 윈도우 새로고침
            wnoutrefresh(game_win);
            wnoutrefresh(ui_win);
            doupdate();

            pthread_mutex_unlock(&render_mutex);
        } else {
            pthread_mutex_lock(&render_mutex);
            mvwprintw(game_win, HEIGHT / 2, WIDTH / 2 - 5, "Paused");
            wnoutrefresh(game_win);
            doupdate();
            pthread_mutex_unlock(&render_mutex);
        }
        usleep(30000);
    }
    
    
    //extern volatile int game_result_received;
    // while (!game_result_received) {
    //     usleep(100000);
    // }
    
    end_game();
    join_thread(imu_thread);
    return NULL;
}