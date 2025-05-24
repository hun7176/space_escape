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

extern int my_player_id;
extern int opponent_score;
extern pthread_mutex_t network_mutex;

//플레이어 상태 구조체
typedef struct {
    int score;
    int game_over;
} player_status_t;

//게임시작시 클라이언트로 id정보 보내줌
typedef struct {
    int player_id;
} game_start_info_t;

//서버용 메시지 타입 정의
typedef enum {
    MSG_LOBBY_JOIN = 1,      //로비 참가 요청
    MSG_LOBBY_LEAVE = 2,     //로비 떠나기
    MSG_READY_STATUS = 3,    //준비 상태 변경 / 클라이언트 -> 서버
    MSG_LOBBY_UPDATE = 4,    //로비 상태 업데이트 / 서버 -> 클라이언트
    MSG_GAME_START = 5,      //게임 시작 신호 / 서버 -> 클라이언트
    MSG_PLAYER_STATUS = 6,   //기존 게임 상태 전송 / 클라이언트 -> 서버
    MSG_GAME_END = 7         //게임 종료 및 결과 서버 -> 클라이언트
} MessageType;

//로비 관련 구조체
typedef struct {
    int player_id;
    int ready;
    char name[32];
    int connected;
} NetworkPlayerInfo;

typedef struct {
    NetworkPlayerInfo players[2];
    int player_count;
    int game_started;
} NetworkLobbyRoom;

//네트워크 메시지 구조체
typedef struct {
    MessageType type;
    int data_size;
    char data[256]; 
} NetworkMessage;

//로비 참가 요청 구조체
typedef struct {
    char player_name[32];
} lobby_join_request_t;

//준비 상태 구조체
typedef struct {
    int ready;
} ready_status_t;

typedef struct {
    int winner_id;
    int player_scores[2];
    char result_message[64];
} game_result_t;

extern NetworkLobbyRoom network_lobby;

#endif