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


pthread_t game_thread, imu_thread, net_thread;
pthread_t test_thread;
volatile int is_running = 1;

void* run_game_thread(void* arg) {
    (void)arg;
    while(is_running){

        run_game(NULL);
        usleep(33000);
    }

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
        sleep(1);         // 50Hz
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
    setlocale(LC_ALL, "ko_KR.UTF-8");
    // init_game_ui();
    // init_imu();

    //서버 연결
    init_connection();
    printf("debug");

    signal(SIGINT, sigint_handler);
    create_thread(&game_thread, run_game_thread, NULL);
    create_thread(&test_thread, test_controller, NULL);
    //create_thread(&imu_thread, run_imu, NULL);
    create_thread(&net_thread, run_network_thread, NULL);

    // while (is_running) {
    //     if (getchar() == 'q') is_running = 0;
    // }
    
    join_thread(game_thread);
    join_thread(test_thread);
    join_thread(net_thread);
    //join_thread(imu_thread);
    //join_thread(net_thread);

    // shutdown_game_ui();
    // shutdown_imu();
    // shutdown_network();

    return 0;
}
