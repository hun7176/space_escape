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

//초기 연결 설정 (client 입장)
void init_connection(){
    int sock;
    struct sockaddr_in serv_addr;

    //서버 주소 설정
    const char * host_address = "0.tcp.jp.ngrok.io";
    int port = 16706;

    //도메인 이름을 ip주소로 변환할때 사용
    //변환 ip주소는 host ->  h_addr_list[0]에 저장장
    struct hostent* host = gethostbyname(host_address);
    if (!host) {
        fprintf(stderr, "gethostbyname() failed\n");
        return;
    }

    //TCP 소켓 생성 ipv4, tcp연결
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        exit(1);
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
        exit(1);
    }

    printf("Successfully connected to server\n");

    
    net_sock = sock;  // 소켓 저장 (main에서 접근 위함)
    
}

//score를 전송하는 함수 (뮤텍스로 보호)
void send_score(){
    int current_score;
    pthread_mutex_lock(&score_mutex);
    current_score = score;
    pthread_mutex_unlock(&score_mutex);
    
    int net_score = htonl(current_score);
    if (send(net_sock, &net_score, sizeof(net_score), 0) == -1) {
        perror("send");
    }
}

//네트워크 스레드 함수: 주기적으로 score 전송
void* run_network(void* arg) {
    (void)arg;
    while (1) {
        send_score();
        usleep(20000);  // 50Hz 전송 (추후 조정정)
    }
    close(net_sock);
    return NULL;
}