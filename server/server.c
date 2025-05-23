#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 12345
#define BUF_SIZE 1024

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

    if (listen(serv_sock, 5) == -1) {
        perror("listen");
        close(serv_sock);
        exit(1);
    }

    printf("Server is listening on port %d...\n", PORT);
    clnt_addr_size = sizeof(clnt_addr);
    clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);

    if (clnt_sock == -1) {
        perror("accept");
        close(serv_sock);
        exit(1);
    }

    read(clnt_sock, buffer, BUF_SIZE);
    printf("Received from client: %s\n", buffer);

    write(clnt_sock, "Hello from server!", 19);

    close(clnt_sock);
    close(serv_sock);
    return 0;
}
