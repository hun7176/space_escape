#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <sys/socket.h>
#include "common.h"

int net_sock;  // 네트워크용 전역 소켓 변수
//static int connection_established = 0;
NetworkLobbyRoom network_lobby = {0};
int my_player_id = -1;
int opponent_score = 0;
pthread_mutex_t network_mutex = PTHREAD_MUTEX_INITIALIZER;

//초기 연결 설정 (client 입장)
int init_connection(){
    int sock;
    struct sockaddr_in serv_addr;

    //서버 주소 설정
    const char * host_address = "0.tcp.jp.ngrok.io";
    int port = 14400;

    //도메인 이름을 ip주소로 변환할때 사용
    //변환 ip주소는 host ->  h_addr_list[0]에 저장장
    struct hostent* host = gethostbyname(host_address);
    if (!host) {
        fprintf(stderr, "gethostbyname() failed\n");
        return -1;
    }

    //TCP 소켓 생성 ipv4, tcp연결
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        return -1;
    }

    //htons -> 포트를 네트워크 바이트 순서로 변환
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    //IP주소 복사
    memcpy(&serv_addr.sin_addr, host->h_addr_list[0], host->h_length);

    //서버에 연결요청 TCP 3-way handshaking으로 연결 시도
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("connect");
        close(sock);
        return -1;
    }

    printf("Successfully connected to server\n");

    
    net_sock = sock;  // 소켓 저장 (main에서 접근 위함)
    return 0; //성공시 0
    
}

//score를 전송하는 함수 (뮤텍스로 보호)
//추후 게임상태 (game_over인지 아닌지도 보내서 판별할 수 있도록 구현)
void send_player_status(){
    //플레이어의 score, 게임종료 여부를 담은 구조체
    player_status_t status;
    static player_status_t last_sent_status = {0, 0};

    pthread_mutex_lock(&score_mutex);
    //현재 플레이어 상태 읽기
    status.score = score;
    status.game_over = game_over;
    pthread_mutex_unlock(&score_mutex);
    

    //이전에 보낸 상태와 같으면 안보냄 (부하 방지)
    if(status.score == last_sent_status.score &&
       status.game_over == last_sent_status.game_over) {
        return;
    }

    //네트워크 바이트 순서로 변환
    player_status_t net_status;
    net_status.score = htonl(status.score);
    net_status.game_over = htonl(status.game_over);
    if (send_network_message(MSG_PLAYER_STATUS, &net_status, sizeof(net_status)) == 0) {
        last_sent_status = status;
    }
}

//MSG_LOBBY_JOIN와 같은 형식화된 요청을 서버에게 전달하기 위함
int send_network_message(MessageType type, void* data, int data_size){
    NetworkMessage msg;
    msg.type = htonl(type);
    msg.data_size = htonl(data_size);

    if(data && data_size > 0){
        memcpy(msg.data, data, data_size);
    }

    if(send(net_sock, &msg, sizeof(NetworkMessage), 0) == -1){
        perror("send_network_message");
        return -1;
    }
    return 0;
}

//서버에 지정된 MessageType으로, lobby join요청을 보냄
int send_lobby_join_request(){
    lobby_join_request_t request; //player_name을 받음
    snprintf(request.player_name, sizeof(request.player_name), "Player_%d", getpid() % 1000);

    return send_network_message(MSG_LOBBY_JOIN, &request, sizeof(request)); //0이면 성공
}

//로비 떠나기 요청
int send_lobby_leave_request() {
    return send_network_message(MSG_LOBBY_LEAVE, NULL, 0);
}

//준비 상태 전송
int send_ready_status(int ready) { //1이면 ready, 0이면 not ready
    ready_status_t status;
    status.ready = ready;
    
    return send_network_message(MSG_READY_STATUS, &status, sizeof(status));
}

//네트워크 메시지 수신
int receive_network_message(NetworkMessage* msg) {
    int bytes_received = recv(net_sock, msg, sizeof(NetworkMessage), MSG_DONTWAIT);
    if (bytes_received == -1) {
        return -1; //받을 메시지가 없음
    }
    if (bytes_received == 0) {
        return 0; //연결 종료
    }
    
    
    msg->type = ntohl(msg->type);
    msg->data_size = ntohl(msg->data_size);
    
    return bytes_received;
}

//로비 상태 업데이트 처리 (서버에서 broadcast할때 MSG_LOBBY_UPDATE로 전송)
void handle_lobby_update(NetworkMessage* msg) {
    pthread_mutex_lock(&network_mutex);
    memcpy(&network_lobby, msg->data, sizeof(NetworkLobbyRoom));
    pthread_mutex_unlock(&network_mutex);
}

// 상대방 게임 상태 처리
void handle_opponent_status(NetworkMessage* msg) {
    player_status_t* opponent_status = (player_status_t*)msg->data;
    
    pthread_mutex_lock(&network_mutex);
    opponent_score = ntohl(opponent_status->score);
    pthread_mutex_unlock(&network_mutex);
}

//todo: 나중에 게임끝날때 UI만들기
void handle_game_result(NetworkMessage* msg){
     game_result_t* result = (game_result_t*)msg->data;
    
    printf("\n=== 게임 결과 ===\n");
    printf("Player 1 점수: %d\n", result->player_scores[0]);
    printf("Player 2 점수: %d\n", result->player_scores[1]);
    
    if (result->winner_id == -1) {
        printf("무승부!\n");
    } else {
        printf("승자: Player %d\n", result->winner_id + 1);
        if (result->winner_id == my_player_id) {
            printf("당신이 이겼습니다!\n");
        } else {
            printf("아쉽게도 졌습니다.\n");
        }
    }
    //printf("%s\n", result->result_message);
}


//서버에서 오는 메시지 처리
//방식 -> MSG_LOBBY_UPDATE같은경우, lobby의 정보를 서버에서 보내줄거고, 그걸 잘담아놓은다음, 이 로비정보를 활용할 코드에서 참조하면됨
void process_network_messages() {
    NetworkMessage msg;
    
    while (receive_network_message(&msg) > 0) {
        switch (msg.type) {
            case MSG_LOBBY_UPDATE:
                handle_lobby_update(&msg);
                break;
            case MSG_GAME_START:
                handle_game_start(&msg);
                break;
            case MSG_PLAYER_STATUS:
                handle_opponent_status(&msg);
                break;
            case MSG_GAME_END:
                handle_game_result(&msg);
                break;
            default:
                break;
        }
    }
}

//게임 시작 신호 처리
void handle_game_start(NetworkMessage* msg) {
    game_start_info_t* info = (game_start_info_t*)msg->data;
    pthread_mutex_lock(&network_mutex);
    network_lobby.game_started = 1;
    my_player_id = ntohl(info->player_id);
    pthread_mutex_unlock(&network_mutex);
}

void update_lobby_status() {
    process_network_messages();
}

//네트워크 스레드 함수: 주기적으로 score 전송
void* run_network(void* arg) {
    (void)arg;
    while (1) {
        process_network_messages();
        
        //게임이 시작된 경우에만 플레이어 상태 전송
        pthread_mutex_lock(&network_mutex);
        int game_started = network_lobby.game_started;
        pthread_mutex_unlock(&network_mutex);

         if (game_started) {
            send_player_status();
        }

        usleep(100000);
    }
    close(net_sock);
    return NULL;
}