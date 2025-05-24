#ifndef NETWORK_H
#define NETWORK_H

#ifdef __cplusplus
extern "C" {
#endif

//main.c에서 참조하는 init_connection, 네트워크 스레드(점수)
int init_connection();
void* run_network(void* arg);
int send_lobby_join_request();
int send_lobby_leave_request();
int send_ready_status(int ready);
void update_lobby_status();

#ifdef __cplusplus
}
#endif

#endif