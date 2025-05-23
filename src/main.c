#include <pthread.h>
#include <stdio.h>
// #include "game.h"
// #include "network.h"
// #include "imu.h"
#include "thread_utils.h"

pthread_t game_thread, imu_thread, net_thread;
volatile int is_running = 1;

void* run_game(void* arg) {
    while (is_running) {
        update_game_state();  // IMU 방향 반영
        render_game_ui();     // ncurses 출력
        usleep(33000);        // 약 30 FPS
    }
    return NULL;
}

void* run_imu(void* arg) {
    while (is_running) {
        read_imu_data();      // 센서 읽기
        apply_filter();       // 필터링
        update_direction();   // 전역 방향 값 갱신
        usleep(10000);        // 100Hz
    }
    return NULL;
}

void* run_network(void* arg) {
    while (is_running) {
        send_player_state();   // 내 상태 전송
        recv_enemy_state();    // 상대 정보 수신
        usleep(20000);         // 50Hz
    }
    return NULL;
}

int main() {
    init_game_ui();
    init_imu();
    init_network();

    create_thread(&game_thread, run_game, NULL);
    create_thread(&imu_thread, run_imu, NULL);
    create_thread(&net_thread, run_network, NULL);

    while (is_running) {
        if (getchar() == 'q') is_running = 0;
    }

    join_thread(game_thread);
    join_thread(imu_thread);
    join_thread(net_thread);

    shutdown_game_ui();
    shutdown_imu();
    shutdown_network();

    return 0;
}
