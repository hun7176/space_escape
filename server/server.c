#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "../include/common.h"


#define PORT 12345
#define BUF_SIZE 1024
#define MAX_CLIENTS 2
#define LOBBY_SIZE 2

typedef struct {
    int fd;
    int idx;
    int score;
    int game_over; //게임종료 되었을시 1, 진행중이면 0
} client_t;

// 서버 클라이언트 정보
typedef struct {
    int fd;
    int player_id; //로비에서의 playerid
    pthread_t thread;
    int connected; //서버에 연결된 상태? -> 게임시작시 init_connection으로 초기에 서버와는 연결됨. 로비는 별개
    int in_lobby; //로비에 참가한 상태
    char name[32];
    int ready;
    int score;
    int game_over;
} server_client_t;

server_client_t clients[MAX_CLIENTS];
pthread_t  threads[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lobby_mutex = PTHREAD_MUTEX_INITIALIZER;
NetworkLobbyRoom lobby = {0};
int server_running = 1;


//네트워크 메시지 전송
int send_to_client(int client_fd, MessageType type, void* data, int data_size) {
    NetworkMessage msg;
    msg.type = htonl(type);
    msg.data_size = htonl(data_size);
    
    if (data && data_size > 0 && data_size <= 256) {
        memcpy(msg.data, data, data_size);
    }
    
    int result = send(client_fd, &msg, sizeof(NetworkMessage), 0);
    if (result == -1) {
        perror("send_to_client");
    }
    return result;
}


//네트워크 메시지 수신 (로비 참가 요청 등의 형식)
int receive_from_client(int client_fd, NetworkMessage* msg){
    int received = recv(client_fd, msg, sizeof(NetworkMessage), 0);
    if (received <= 0){
        return received;
    }

    msg->type = ntohl(msg->type);
    msg->data_size = ntohl(msg->data_size);

    return received;
}

//로비 상태를 로비참가자에게 전송
void broadcast_lobby_update() {
    pthread_mutex_lock(&lobby_mutex);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        //서버연결되어 있으며 + 로비에 참가한 클라이언트에게만 전송
        if (clients[i].connected && clients[i].in_lobby) {
            send_to_client(clients[i].fd, MSG_LOBBY_UPDATE, &lobby, sizeof(NetworkLobbyRoom));
        }
    }
    
    pthread_mutex_unlock(&lobby_mutex);
}

//게임 시작 신호를 로비의 모든 플레이어에게 전송
void broadcast_game_start() {
    pthread_mutex_lock(&lobby_mutex);
    
    lobby.game_started = 1;
    int seed = time(NULL); //각 클라이언트에게 동일한 시드 부여

    //디버깅용
    printf("[server] Game Seed : %d\n", seed);

    for (int i = 0; i < LOBBY_SIZE; i++) {
        if (lobby.players[i].connected) {
            int client_idx = lobby.players[i].player_id;
            if (client_idx >= 0 && client_idx < MAX_CLIENTS && clients[client_idx].connected) {
                game_start_info_t info;
                info.player_id = htonl(i); //각 player id를 보내줌
                info.seed = htonl(seed);


                send_to_client(clients[client_idx].fd, MSG_GAME_START, &info, sizeof(info)); //클라이언트는 이걸 보고 실행하면됨
            }
        }
    }
    
    pthread_mutex_unlock(&lobby_mutex);
    
    printf("[Server] Game started!\n");
}

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

/**
 * 로비에 플레이어를 추가합니다
 * 로비가 가득찬 경우, 실패시 -1
 * 성공시 0을 return 합니다.
 * client측에서 lobby join을 위해 name을 서버측에 보내는데, 그 name을 인자로 받습니다.
 */
int add_player_to_lobby(int client_idx, const char* name){
    pthread_mutex_lock(&lobby_mutex);

    //이미 로비에 참가한 상태라면 무시
    if (clients[client_idx].in_lobby) {
        pthread_mutex_unlock(&lobby_mutex);
        return 0;
    }

    if(lobby.player_count >= LOBBY_SIZE){
        pthread_mutex_unlock(&lobby_mutex);
        return -1;
    }

    //로비에 빈 슬롯찾기
    for(int i=0; i<LOBBY_SIZE; i++){
        if(!lobby.players[i].connected){ //초기엔 전부 0으로 초기화되어있음
            lobby.players[i].player_id = client_idx; //해당 clnt index를 할당해줌
            lobby.players[i].connected = 1; //로비에 connected되었다는 뜻
            lobby.players[i].ready = 0;
            strncpy(lobby.players[i].name, name, sizeof(lobby.players[i].name) -1);
            lobby.players[i].name[sizeof(lobby.players[i].name) -1] =  '\0';

            //클라이언트 정보 완성
            clients[client_idx].player_id = i;
            clients[client_idx].in_lobby = 1; //로비 참가 상태로 설정
            strcpy(clients[client_idx].name, name);

            lobby.player_count++;
            break;
        }
    }
    pthread_mutex_unlock(&lobby_mutex);
    printf("[Server] Player %s joined lobby (slot %d)\n", name, clients[client_idx].player_id);
    broadcast_lobby_update(); //클라이언트에게도 전송
    return 0; 

}

/**
 * 로비에서 플레이어를 제거합니다.
 */
void remove_player_from_lobby(int client_idx){
    pthread_mutex_lock(&lobby_mutex);

    //로비에 참가하지 않은 클라이언트라면 무시
    if (!clients[client_idx].in_lobby) {
        pthread_mutex_unlock(&lobby_mutex);
        return;
    }

    int player_slot = clients[client_idx].player_id;
    if (player_slot >= 0 && player_slot < LOBBY_SIZE) {
        lobby.players[player_slot].connected = 0;
        lobby.players[player_slot].ready = 0;
        memset(lobby.players[player_slot].name, 0, sizeof(lobby.players[player_slot].name));
        lobby.player_count--;
        
        //게임이 진행 중이었다면 게임 종료
        if (lobby.game_started) {
            printf("[Server] Game has been terminated (Player Exited)\n"); //이경우는 플레이어가 게임시작했는데, 중간에 나가서 서버와 연결이 종료된경우
            pthread_mutex_unlock(&lobby_mutex);
            handle_game_end(); //handle_game_end()에서 lobby.game_started = 0 인경우, return을 하므로, game종료시키기 전에 처리해야함!
            pthread_mutex_lock(&lobby_mutex);
            lobby.game_started = 0;
            
        }
    }
    
    
    //클라이언트의 로비 상태만 초기화(서버연결은 유지)
    clients[client_idx].in_lobby = 0;
    clients[client_idx].player_id = -1;
    clients[client_idx].ready = 0;
    clients[client_idx].score = 0;
    clients[client_idx].game_over = 0;

    pthread_mutex_unlock(&lobby_mutex);
    printf("[Server] Player removed from lobby\n");
    broadcast_lobby_update();
}

//준비 상태 업데이트
void update_ready_status(int client_idx, int ready) {
    pthread_mutex_lock(&lobby_mutex);
    
    //로비에 참가하지 않은 클라이언트는 무시
    if (!clients[client_idx].in_lobby) {
        pthread_mutex_unlock(&lobby_mutex);
        return;
    }
    
    int player_slot = clients[client_idx].player_id;
    if (player_slot >= 0 && player_slot < LOBBY_SIZE) {
        lobby.players[player_slot].ready = ready;
        clients[client_idx].ready = ready;
        
        printf("[Server] Player %s ready status: %d\n", 
               lobby.players[player_slot].name, ready);
        
        //모든 플레이어가 준비되었는지 확인
        int all_ready = 1;
        for (int i = 0; i < LOBBY_SIZE; i++) {
            if (lobby.players[i].connected && !lobby.players[i].ready) {
                all_ready = 0;
                break;
            }
        }
        
        //모든 플레이어가 준비되고 로비가 가득 찼으면 게임 시작
        if (all_ready && lobby.player_count == LOBBY_SIZE && !lobby.game_started) {
            pthread_mutex_unlock(&lobby_mutex);
            broadcast_game_start();
            return;
        }
    }
    
    pthread_mutex_unlock(&lobby_mutex);
    broadcast_lobby_update();
}

void update_game_status(int client_idx, int score, int game_over){
    clients[client_idx].score = score;
    clients[client_idx].game_over = game_over; 
    //여기서 해야할건 client_idx로 들어온 점수, game_over여부를 상대방에게 보내주는 것

    //상대방에게 점수 전송
    pthread_mutex_lock(&lobby_mutex);
    
    if(lobby.game_started){
        player_status_t status;
        status.score = htonl(score);
        status.game_over = htonl(game_over);

        //다른 플레이어에게 전송
        for(int i=0;i<MAX_CLIENTS;i++){
            if(lobby.players[i].connected && lobby.players[i].player_id != client_idx){ //내가 아닌 상대 -> lobby에 connected되어있으면서, player_id가 내 client인덱스와 달라야함
                int other_client = lobby.players[i].player_id;
                if(other_client >= 0 && other_client < MAX_CLIENTS && clients[other_client].connected){
                    send_to_client(clients[other_client].fd, MSG_PLAYER_STATUS, &status, sizeof(status)); //해당 상대에게 보냄
                }
            }
        }
    }

    pthread_mutex_unlock(&lobby_mutex);
}

/**
 * 게임 종료를 처리함 (점수 비교, 누가 이겼는지를 클라이언트에게 전달)
 * 우선 플레이어 두명이 둘다 끝나야만 처리가능
 */
void handle_game_end(){
    pthread_mutex_lock(&lobby_mutex);
    
    //게임 시작상태가 아니면 그냥 return
    if (!lobby.game_started) {
        pthread_mutex_unlock(&lobby_mutex);
        return;
    }

    int connected_count = 0;
    int last_connected_idx = -1;
    //모든 플레이어가 끝났는지를 확인 -> 한명의 플레이어가 튕긴경우도 고려
    int all_finished = 1;
    for(int i=0;i<MAX_CLIENTS;i++){
        if(lobby.players[i].connected){
            connected_count++;
            last_connected_idx = i; //남아있는 플레이어
            int client_idx = lobby.players[i].player_id;
             if (client_idx >= 0 && client_idx < MAX_CLIENTS && !clients[client_idx].game_over) { //만약 client중 gameover = 0인 사람이 있으면, all_finished = 0
                all_finished = 0;
            }
        }
    }
    
    //승자 결정 (한명만 남았거나 모두 끝났을때 처리)
    if(connected_count == 1 || all_finished){

        game_result_t result;
        memset(&result, 0, sizeof(result));

        if(connected_count == 1){
            //한명만 남은 경우
            result.winner_id = last_connected_idx;
            strcpy(result.result_message, "상대방이 게임을 종료했습니다.");
            result.player_scores[last_connected_idx] = clients[last_connected_idx].score;
            result.player_scores[!last_connected_idx] = 0;
        }
        else{

            int score[2] = {0, 0};
            int player = 0;
    
    
            for(int i=0;i<MAX_CLIENTS;i++){
                if(lobby.players[i].connected){
                    int client_idx = lobby.players[i].player_id;
                    //두명의 클라이언트의 최종 점수를 담음
                    score[player] = clients[client_idx].score;
                    result.player_scores[player] = score[player];
                    player++;
                }
            }
    
    
            if(score[0] > score[1]){ //player 1의 승리
                result.winner_id = 0;
                strcpy(result.result_message, "Player 1 wins!");
            } else if (score[1] > score[0]) {
                result.winner_id = 1;
                strcpy(result.result_message, "Player 2 wins!");
            } else {
                result.winner_id = -1;
                strcpy(result.result_message, "Draw!");
            }
        }

        //각 플레이어에게 result구조체에 담아서 결과전송
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (lobby.players[i].connected) {
                int client_idx = lobby.players[i].player_id;
                if (client_idx >= 0 && client_idx < MAX_CLIENTS && clients[client_idx].connected) {
                    send_to_client(clients[client_idx].fd, MSG_GAME_END, &result, sizeof(result));
                }
            }
        }

        //게임이 끝났으니, 로비상태를 초기화함
        lobby.game_started = 0;
        for (int i = 0; i < LOBBY_SIZE; i++) {
            if (lobby.players[i].connected) {
                lobby.players[i].ready = 0;
                int client_idx = lobby.players[i].player_id;
                if (client_idx >= 0 && client_idx < MAX_CLIENTS) {
                    clients[client_idx].ready = 0;
                    clients[client_idx].game_over = 0;
                    clients[client_idx].score = 0;
                }
            }
        }
        printf("[Server] Game ended. Winner: %d\n", result.winner_id);
    }

    pthread_mutex_unlock(&lobby_mutex);
    broadcast_lobby_update();
}

void * handle_client(void * arg){
    //해당 클라이언트 index를 인자로 받아옴
    int client_idx = *(int*)arg;
    int client_fd = clients[client_idx].fd;
    NetworkMessage msg;

    printf("[Server] Client %d connected\n", client_idx);

    while (server_running && clients[client_idx].connected){
        int recv_data = receive_from_client(client_fd, &msg); //루프돌면서 계속 해당 client의 message를 받아옴

        if(recv_data <= 0){
            printf("[Server] Client %d disconnected\n", client_idx);
            break;
        }

        
        //지정되어있는 msg type을 통해 어떤 요청을 했는지 구분

        switch (msg.type){
            case MSG_LOBBY_JOIN : {
                lobby_join_request_t* request = (lobby_join_request_t*)msg.data; //여기서는 player name이 data
                if (add_player_to_lobby(client_idx, request->player_name) == -1) {
                printf("[Server] Lobby is full, rejecting client %d\n", client_idx);
                }
                break;
            }
            case MSG_LOBBY_LEAVE:
                remove_player_from_lobby(client_idx);
                break;

            case MSG_READY_STATUS: {
                ready_status_t* status = (ready_status_t*)msg.data;
                update_ready_status(client_idx, status->ready);
                break;
            }
            case MSG_PLAYER_STATUS:{ //player가 자신의 상태(점수, game_over여부)를 상대에게 send
                player_status_t* status = (player_status_t*)msg.data;
                int score = ntohl(status->score);
                int game_over = ntohl(status->game_over);

                update_game_status(client_idx, score, game_over); 
                //상대에게 game_status를 알려주고, game_over = 1이라면, handle_game_end를 핸들링함.

                if(game_over){
                    handle_game_end(); 
                }
                break;
            }
            
            default:
                printf("[Server] Unknown message type: %d\n", msg.type);
                break;
  
        }
    }

    if (clients[client_idx].in_lobby) {
        remove_player_from_lobby(client_idx);
    }
    
    clients[client_idx].connected = 0;
    clients[client_idx].fd = -1;
    close(client_fd);
    
    printf("[Server] Client %d fully disconnected\n", client_idx);
    
    return NULL;
    
    

    

}
int main() {
    int serv_sock, clnt_sock;

    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;
    char buffer[BUF_SIZE];


    //클라이언트 배열 초기화
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].connected = 0;
        clients[i].in_lobby = 0;
        clients[i].player_id = -1;
        clients[i].fd = -1;
        memset(clients[i].name, 0, sizeof(clients[i].name));
    }


    serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1) {
        perror("socket");
        exit(1);
    }

    int opt = 1;
    if (setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        close(serv_sock);
        exit(1);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    //바인딩
    if (bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("bind");
        close(serv_sock);
        exit(1);
    }

    //listen으로 최대 2 클라이언트 대기
    if (listen(serv_sock, MAX_CLIENTS) == -1) {
        perror("listen");
        close(serv_sock);
        exit(1);
    }

    printf("Server is listening on port %d...\n", PORT);


    //클라이언트 연결수락 루프
    while(server_running){
        clnt_addr_size = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);

        if(clnt_sock == -1){
            perror("accept");
            continue;
        }

        //빈 클라이언트 슬롯 찾기(현재는 2명까지 허용)
        int client_idx = -1;
        for(int i=0; i<MAX_CLIENTS; i++){
            if(!clients[i].connected){ //client 연결이 안되어있는 상태라면 거기 할당
                client_idx = i;
                break;
            } //만약 클라이언트가 꽉차있으면 그대로 client_idx = -1일것
        }
        if (client_idx == -1) {
            printf("[Server] Maximum clients reached, rejecting connection\n");
            //클라이언트에게 서버가 가득 찼음을 알림
            const char* reject_msg = "SERVER_FULL";
            send(clnt_sock, reject_msg, strlen(reject_msg), 0);
            printf("[Server] send %s\n",reject_msg);
            usleep(100000);
            close(clnt_sock);
            continue;
        } else{
            //슬롯 할당 성공
            const char* welcome_msg = "WELCOME";
            send(clnt_sock, welcome_msg, strlen(welcome_msg), 0);
        }

        //클라이언트 정보 설정
        clients[client_idx].fd = clnt_sock;
        clients[client_idx].connected = 1;
        clients[client_idx].in_lobby = 0;   //처음에는 로비 x
        clients[client_idx].player_id = -1;
        clients[client_idx].ready = 0;
        clients[client_idx].score = 0;
        clients[client_idx].game_over = 0;
        memset(clients[client_idx].name, 0, sizeof(clients[client_idx].name));

        //클라이언트 처리 스레드 생성
        //스레드에 인자로 넘길 idx
        int* client_idx_ptr = malloc(sizeof(int));
        *client_idx_ptr = client_idx;
        
        if (pthread_create(&clients[client_idx].thread, NULL, handle_client, client_idx_ptr) != 0) {
            perror("pthread_create");
            close(clnt_sock);
            clients[client_idx].connected = 0;
            free(client_idx_ptr);
        }
        
        pthread_detach(clients[client_idx].thread);

    }

    // //accept() 를 두 번 호출해 두 명을 연결 대기
    // for (int i = 0; i < 2; i++) {
    //     clients[i].fd  = accept(serv_sock, NULL, NULL);
    //     clients[i].idx = i;
    //     pthread_create(&threads[i], NULL, recv_client_state, &clients[i]);
    // }

    // for (int i = 0; i < MAX_CLIENTS; i++)
    //     pthread_join(threads[i], NULL);

    close(serv_sock);
    printf("[Server] Done.\n");
    return 0;
}
