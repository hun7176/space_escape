#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "../include/common.h"

#define PORT 12345
#define BUF_SIZE 1024
#define MAX_CLI 2

typedef struct {
    int fd;
    int idx;
    int score;
    int game_over; //게임종료 되었을시 1, 진행중이면 0
} client_t;

client_t clients[MAX_CLI];
pthread_t  threads[MAX_CLI];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void *recv_client_state(void *arg) {
    client_t *c = (client_t*)arg;
    player_status_t status;
    int net_sc;
    time_t last_print = 0;

    //클라이언트로부터 player_status_t 구조체 받기
    while (1) {
        ssize_t bytes = recv(c->fd, &status, sizeof(status), 0);
        if (bytes <= 0) {
            perror("recv");
            break;
        }

        int net_score = ntohl(status.score);
        int net_game_over = ntohl(status.game_over);

        pthread_mutex_lock(&clients_mutex);
        c->score = net_score;
        c->game_over = net_game_over;
        pthread_mutex_unlock(&clients_mutex);

        printf("[Server] Client%d updated score=%d, game_over=%d\n",
               c->idx, c->score, c->game_over);

        time_t now = time(NULL);
        if (now - last_print >= 1) {
            // printf("[Server] Client%d score=%d, game_over=%d\n",
            //        c->idx, c->score, c->game_over);
            last_print = now;
        }

         if (net_game_over == 1) {
            printf("[Server] Client%d game over with final score=%d\n",
                   c->idx, c->score);
            break;
        }

    }
    return NULL;
}


int main() {
    int serv_sock, clnt_sock;

    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;
    char buffer[BUF_SIZE];

    serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1) {
        perror("socket");
        exit(1);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    if (bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("bind");
        close(serv_sock);
        exit(1);
    }

    //listen으로 최대 2 클라이언트 대기
    if (listen(serv_sock, 2) == -1) {
        perror("listen");
        close(serv_sock);
        exit(1);
    }

    printf("Server is listening on port %d...\n", PORT);

    //accept() 를 두 번 호출해 두 명을 연결 대기
    for (int i = 0; i < 2; i++) {
        clients[i].fd  = accept(serv_sock, NULL, NULL);
        clients[i].idx = i;
        pthread_create(&threads[i], NULL, recv_client_state, &clients[i]);
    }

    for (int i = 0; i < MAX_CLI; i++)
        pthread_join(threads[i], NULL);


    

    //승자 결정
    int win_idx = (clients[0].score > clients[1].score) ? 0
                 : (clients[1].score > clients[0].score) ? 1
                 : -1;
    for (int i = 0; i < MAX_CLI; i++) {
        int res = (win_idx < 0 ? 0 : (i == win_idx ? 1 : 2));
        //1 = win, 2 = lose (or 0 = draw)
        int net_r = htonl(res);
        send(clients[i].fd, &net_r, sizeof(net_r), 0);
        close(clients[i].fd);
    }
    close(serv_sock);
    printf("[Server] Done.\n");
    return 0;
}
