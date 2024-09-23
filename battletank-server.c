#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "network.h"

#define BUFSIZE 1024

/*
 * Creates a socket connection to the specified host:port
 */
static int server_socket(const char *host, int port) {
    struct sockaddr_in serveraddr;
    struct hostent *server;

    // socket: create the socket
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd < 0) goto err;

    // gethostbyname: get the server's DNS entry
    server = gethostbyname(host);
    if (server == NULL) goto err;

    // build the server's address
    bzero((char *)&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serveraddr.sin_addr.s_addr,
          server->h_length);
    serveraddr.sin_port = htons(port);

    // connect: create a connection with the server
    if (connect(sfd, (const struct sockaddr *)&serveraddr, sizeof(serveraddr)) <
        0)
        goto err;

    return sfd;

err:

    perror("socket(2) opening socket failed");
    return -1;
}

static int broadcast(int *client_fds, int fd, const char *buf, size_t count) {
    int written = 0;
    for (int i = 0; i < FD_SETSIZE; i++) {
        if (client_fds[i] >= 0 && client_fds[i] != fd) {
            // TODO check for errors writing
            written += network_send(client_fds[i], buf, count);
        }
    }

    return written;
}

static void server_loop(int server_fd) {
    fd_set readfds;
    int client_fds[FD_SETSIZE];
    int maxfd = server_fd;
    int i = 0;

    // Initialize client_fds array
    for (i = 0; i < FD_SETSIZE; i++) {
        client_fds[i] = -1;
    }

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);

        for (i = 0; i < FD_SETSIZE; i++) {
            if (client_fds[i] >= 0) {
                FD_SET(client_fds[i], &readfds);
                if (client_fds[i] > maxfd) {
                    maxfd = client_fds[i];
                }
            }
        }

        int num_events = select(maxfd + 1, &readfds, NULL, NULL, NULL);

        if (num_events == -1) {
            perror("select() error");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(server_fd, &readfds)) {
            // New connection request
            struct sockaddr_in clientaddr;
            socklen_t clientlen = sizeof(clientaddr);
            int client_fd =
                accept(server_fd, (struct sockaddr *)&clientaddr, &clientlen);
            if (client_fd < 0) {
                perror("accept() error");
                continue;
            }

            for (i = 0; i < FD_SETSIZE; i++) {
                if (client_fds[i] < 0) {
                    client_fds[i] = client_fd;
                    break;
                }
            }

            if (i == FD_SETSIZE) {
                fprintf(stderr, "Too many clients\n");
                close(client_fd);
            }
        }

        for (i = 0; i < FD_SETSIZE; i++) {
            int fd = client_fds[i];
            if (fd >= 0 && FD_ISSET(fd, &readfds)) {
                // Data from the server
                char buf[BUFSIZE];
                memset(buf, 0x00, sizeof(buf));

                ssize_t count = network_recv(fd, buf, sizeof(buf));
                if (count <= 0) {
                    close(fd);
                    client_fds[i] = -1;
                } else {
                    // TODO: send update to other players
                    broadcast(client_fds, fd, buf, count);
                }
            }
        }
    }
}

int main(void) {
    int server_fd = server_socket("localhost", 6699);
    if (server_fd < 0) exit(EXIT_FAILURE);

    server_loop(server_fd);

    return 0;
}
