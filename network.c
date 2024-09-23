#include "network.h"

#include <errno.h>

ssize_t network_send(int fd, const char *buf, size_t count) {
    ssize_t n = 0;
    size_t written = 0;

    /* Let's reply to the client */
    while (written < count) {
        n = write(fd, buf + n, count);
        if (n == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            else
                return n;
        }
        written += n;
    }

    return written;
}

ssize_t network_recv(int fd, char *buf, size_t count) {
    ssize_t n = 0;
    size_t received = 0;
    /* Let's read from the client */
    while (received < count) {
        n = read(fd, buf + n, count);
        if (n == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            else
                return n;
        }
        received += n;
    }
    return received;
}
