// test_server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 12345
#define MAX_CLI 2

typedef struct {
    int fd;
    int idx;
    int score;
} client_t;

client_t clients[MAX_CLI];
pthread_t  threads[MAX_CLI];

void *recv_score(void *arg) {
    client_t *c = (client_t*)arg;
    int net_sc;

    //클라이언트로부터 int 점수 받기
    if (recv(c->fd, &net_sc, sizeof(net_sc), 0) <= 0) {
        perror("recv");
        c->score = -1;
    } else {
        c->score = ntohl(net_sc);
        printf("[Server] Client%d sent score=%d\n", c->idx, c->score);
    }
    return NULL;
}

int main() {
    //tcp 소켓 생성성
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in srv = { .sin_family=AF_INET, .sin_addr.s_addr=INADDR_ANY, .sin_port=htons(PORT) };

    //bind로 0.0.0.0:12345에 연결
    bind(listen_fd, (struct sockaddr*)&srv, sizeof(srv));

    //listen으로 최대 2 클라이언트 대기
    listen(listen_fd, MAX_CLI);
    printf("[Server] Listening on port %d...\n", PORT);

    //accept() 를 두 번 호출해 두 명을 연결 대기
    for (int i = 0; i < MAX_CLI; i++) {
        clients[i].fd  = accept(listen_fd, NULL, NULL);
        clients[i].idx = i;
        pthread_create(&threads[i], NULL, recv_score, &clients[i]);
    }

    //점수 수신 대기(쓰레드 종료 대기)
    for (int i = 0; i < MAX_CLI; i++)
        pthread_join(threads[i], NULL);

    // 3) Decide winner (higher score wins; tie => draw)
    int win_idx = (clients[0].score > clients[1].score) ? 0
                 : (clients[1].score > clients[0].score) ? 1
                 : -1;
    for (int i = 0; i < MAX_CLI; i++) {
        int res = (win_idx < 0 ? 0 : (i == win_idx ? 1 : 2));
        // 1 = win, 2 = lose (or 0 = draw)
        int net_r = htonl(res);
        send(clients[i].fd, &net_r, sizeof(net_r), 0);
        close(clients[i].fd);
    }
    close(listen_fd);
    printf("[Server] Done.\n");
    return 0;
}
