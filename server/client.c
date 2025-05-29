#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>

#define BUF_SIZE 1024

int main() {
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[BUF_SIZE];

    // ngrok이 제공한 주소와 포트로 수정하세요
    const char* ngrok_host = "0.tcp.jp.ngrok.io";  // 예: 0.tcp.ngrok.io
    int ngrok_port = 16925;                   // ngrok이 출력한 포트로 교체

    struct hostent* host = gethostbyname(ngrok_host);
    if (!host) {
        fprintf(stderr, "gethostbyname() failed\n");
        return 1;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        exit(1);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(ngrok_port);
    memcpy(&serv_addr.sin_addr, host->h_addr_list[0], host->h_length);
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("connect");
        close(sock);
        exit(1);
    }

    printf("Successfully connected to server\n");

    //디버깅용: 랜덤 점수 생성
    srand(time(NULL) ^ getpid());
    int score = rand() % 100;
    printf("[Client] Sending score=%d\n", score);

    //점수 전송
    int net_sc = htonl(score);
    send(sock, &net_sc, sizeof(net_sc), 0);

    // 서버 결과 수신
    int net_r;
    if (recv(sock, &net_r, sizeof(net_r), 0) <= 0) {
        perror("recv");
    } else {
        int res = ntohl(net_r);
        if (res == 1)      printf("[Client] You WIN!\n");
        else if (res == 2) printf("[Client] You LOSE!\n");
        else               printf("[Client] DRAW.\n");
    }
    close(sock);
    return 0;
}
