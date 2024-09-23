#ifndef BATTLETANK_CLIENT_H
#define BATTLETANK_CLIENT_H

#include <unistd.h>

int client_connect(const char *host, int port);
int client_disconnect(int sockfd);
int client_send_data(int sockfd, const char *data, size_t datasize);
int client_recv_data(int sockfd, char *data, size_t datasize);

#endif
