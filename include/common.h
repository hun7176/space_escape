//공용 헤더 (공유데이터)
#ifndef COMMON_H
#define COMMON_H
#include <pthread.h>

extern int score;
extern pthread_mutex_t score_mutex;


// 네트워크 전역 소켓 변수
extern int net_sock;

#endif