//공용 헤더 (공유데이터)
#ifndef COMMON_H
#define COMMON_H
#include <pthread.h>

extern int score;
//게임 종료 확인용 변수
extern int game_over;
extern pthread_mutex_t score_mutex;


// 네트워크 전역 소켓 변수
extern int net_sock;

// 플레이어 상태 구조체
typedef struct {
    int score;
    int game_over;
} player_status_t;

#endif