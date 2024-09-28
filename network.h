#ifndef NETWORK_H
#define NETWORK_H

#include <unistd.h>

ssize_t network_send(int fd, const char *buf, size_t count);
ssize_t network_recv(int fd, char *buf);

#endif
