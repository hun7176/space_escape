#ifndef GAME_H
#define GAME_H

#ifdef __cplusplus
extern "C" {
#endif

// run_game 스레드 함수 선언, main.c에서 참조합니다.
void* run_game(void* arg);

#ifdef __cplusplus
}
#endif

#endif // GAME_H