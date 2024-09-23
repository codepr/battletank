#include "battletank-client.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "network.h"

#define BUFSIZE 1024

static int create_socket(void) {
    // Create socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket() error");
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

static int socket_connect(const char *host, int port) {
    int sockfd = create_socket();
    struct sockaddr_in serveraddr;

    // Server address setup
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &serveraddr.sin_addr) <= 0) {
        perror("inet_pton() error");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) <
        0) {
        perror("connect() error");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

int client_connect(const char *host, int port) {
    return socket_connect(host, port);
}

int client_disconnect(int sockfd) {
    close(sockfd);
    return 0;
}

int client_send_data(int sockfd, const char *data, size_t datasize) {
    ssize_t n = network_send(sockfd, data, datasize);
    if (n < 0) {
        perror("write() error");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    return n;
}

int client_recv_data(int sockfd, char *data, size_t datasize) {
    ssize_t n = network_recv(sockfd, data, datasize);
    if (n < 0) {
        perror("read() error");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    return n;
}
