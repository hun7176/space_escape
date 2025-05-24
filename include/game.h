#ifndef GAME_H
#define GAME_H

#ifdef __cplusplus
extern "C" {
#endif

// run_game 스레드 함수 선언, main.c에서 참조합니다.
void* run_game(void* arg);

//게임모드
typedef enum {
    GAME_MODE_SINGLE = 1,
    GAME_MODE_MULTI = 2
} GameMode;

typedef enum {
    LOBBY_STATE_WAITING = 0,
    LOBBY_STATE_READY = 1,
    LOBBY_STATE_GAME_START = 2
} LobbyState;

typedef struct {
    int player_id;
    int ready;
    char name[32];
} PlayerInfo;

typedef struct {
    PlayerInfo players[2];
    int player_count;
    LobbyState state;
} LobbyRoom;

//게임 UI 변경을 위한 플래그
extern GameMode current_game_mode;
extern LobbyRoom lobby;

#ifdef __cplusplus
}
#endif

#endif // GAME_H