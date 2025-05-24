#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "game.h"
#include <locale.h>
// #include "network.h"
// #include "imu.h"
#include "thread_utils.h"
#include <signal.h>
#include <ncurses.h>
#include "network.h"
#include "intro_ui.h"
#include "game_parameter.h"

pthread_t game_thread, imu_thread, net_thread;
pthread_t test_thread;
volatile int is_running = 1;
GameMode current_game_mode = GAME_MODE_SINGLE;

void* run_game_thread(void* arg) {
    (void)arg;
    

    run_game(NULL);
        
    

    // while (is_running) {
    //     update_game_state();  // IMU 방향 반영
    //     render_game_ui();     // ncurses 출력
    //     usleep(33000);        // 약 30 FPS
    // }
    // return NULL;
    return NULL;
}


// void* run_imu(void* arg) {
//     while (is_running) {
//         read_imu_data();      // 센서 읽기
//         apply_filter();       // 필터링
//         update_direction();   // 전역 방향 값 갱신
//         usleep(10000);        // 100Hz
//     }
//     return NULL;
// }

void* run_network_thread(void* arg) {
    (void)arg;
    while (is_running) {
        run_network(NULL);
        usleep(100000); 
    }
    return NULL;
}

//테스트용

void* test_controller(void* arg) {
    (void)arg;
    // 여러 개의 입력 후보 (예: 숫자 키, 스페이스, p)
    int input_chars[] = {'7', '8', '9', '4', '6', '1', '2', '3', ' '};
    int num_inputs = sizeof(input_chars) / sizeof(input_chars[0]);
    
    // srand는 게임 초기화 시 이미 호출되었더라도, 혹은 따로 호출합니다.
    // srand(time(NULL));
    
    while(is_running) {
        int index = rand() % num_inputs;
        // ncurses 입력 큐에 임의의 키를 삽입
        ungetch(input_chars[index]);
        usleep(500000);  // 0.5초마다 하나씩 삽입
    }
    return NULL;
}

void sigint_handler(int sig){
    (void)sig;
    is_running = 0;
}
int main() {
    setlocale(LC_ALL, "");
    //setlocale(LC_ALL, "ko_KR.UTF-8");
    initscr();              // ncurses 초기화
    noecho();
    curs_set(FALSE);
    keypad(stdscr, TRUE);
    timeout(-1);
    init_colors();
    int connected = 0;
    //서버 연결
    if (connected = init_connection() == -1) {
        fprintf(stderr, "서버 연결 실패, 네트워크 기능 없이 게임을 진행합니다.\n");
        //네트워크 스레드 생성 없이 게임 진행
    } else {
        //서버 연결 성공 시 네트워크 스레드 생성
        //create_thread(&net_thread, run_network_thread, NULL);
    }
    // 먼저 메인 메뉴에서 모드 선택
    int game_mode = show_main_menu();
    
    if (game_mode == GAME_MODE_MULTI) {
        if (connected == -1) {
            mvprintw(SCREEN_ROWS/2, SCREEN_COLS/2 - 15, "서버 연결 실패, 단일 모드로 진행합니다.");
            refresh();
            usleep(2000000);
            current_game_mode = GAME_MODE_SINGLE;
        } else {
            if (show_multiplayer_lobby() == 1) {
                // 로비 준비 완료되면 네트워크 스레드 실행
                //create_thread(&net_thread, run_network_thread, NULL);
                current_game_mode = GAME_MODE_MULTI;
            }
            else {
                mvprintw(SCREEN_ROWS/2, SCREEN_COLS/2 - 15, "로비 준비 실패, 단일 모드로 진행합니다.");
                refresh();
                usleep(2000000);
                current_game_mode = GAME_MODE_SINGLE;
            }
        }
    }
    // init_game_ui();
    // init_imu();
    
    //서버 연결
    // if (init_connection() == -1) {
    //     fprintf(stderr, "서버 연결 실패, 네트워크 기능 없이 게임을 진행합니다.\n");
    //     //네트워크 스레드 생성 없이 게임 진행
    // } else {
    //     //서버 연결 성공 시 네트워크 스레드 생성
    //     create_thread(&net_thread, run_network_thread, NULL);
    // }
    printf("debug");

    signal(SIGINT, sigint_handler);
    create_thread(&game_thread, run_game_thread, NULL);
    create_thread(&test_thread, test_controller, NULL);
    if(connected != -1){
        create_thread(&net_thread, run_network_thread, NULL);
    }
    //create_thread(&imu_thread, run_imu, NULL);
    //create_thread(&net_thread, run_network_thread, NULL);

    // while (is_running) {
    //     if (getchar() == 'q') is_running = 0;
    // }
    
    join_thread(game_thread);
    join_thread(test_thread);
    if(net_thread) join_thread(net_thread);
    //join_thread(imu_thread);
    //join_thread(net_thread);

    // shutdown_game_ui();
    // shutdown_imu();
    // shutdown_network();
    endwin();
    return 0;
}
