#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

#define BUF_SIZE 1024

int main() {
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[BUF_SIZE];

    // ngrok이 제공한 주소와 포트로 수정하세요
    const char* ngrok_host = "hun7176.iptime.org";  // 예: 0.tcp.ngrok.io
    int ngrok_port = 12345;                   // ngrok이 출력한 포트로 교체

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

    write(sock, "Hello from client!", 19);
    read(sock, buffer, BUF_SIZE);
    printf("Received from server: %s\n", buffer);

    close(sock);
    return 0;
}
