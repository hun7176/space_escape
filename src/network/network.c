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
    if (send(net_sock, &net_status, sizeof(net_status), 0) == -1) {
        perror("send");
    } else {
        //전송에 성공하면 last_sent_status를 갱신
        last_sent_status = status;
    }
}

//네트워크 스레드 함수: 주기적으로 score 전송
void* run_network(void* arg) {
    (void)arg;
    while (1) {
        send_player_status();
        usleep(100000);
    }
    close(net_sock);
    return NULL;
}