// test_client.c
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>
#include <ncurses.h>

#define PORT 12345
#define IP "hun7176.iptime.org"

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    //서버 ip, port지정
    struct sockaddr_in srv = { .sin_family=AF_INET, .sin_port=htons(PORT) };
    inet_pton(AF_INET, IP, &srv.sin_addr);
    if (connect(sock, (struct sockaddr*)&srv, sizeof(srv)) != 0) {
        perror("connect");
        printf("Failed to connect to %s:%d\n", IP, PORT);
        return 1;
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
