#include "network.h"

#include <errno.h>

#include "protocol.h"

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

/**
 * Receives a serialized packet from the network over a socket file descriptor.
 *
 * - Reads the first 4 bytes to determine the total length of the packet.
 * - Reads data in a loop until the entire packet is received or an error
 * occurs.
 * - If a non-blocking socket is used, it will return immediately when no data
 * is available (errno = EAGAIN or EWOULDBLOCK).
 */
ssize_t network_recv(int fd, char *buf) {
    int received = 0, count = sizeof(int);

    // Read the header length
    ssize_t n = read(fd, buf, count);
    if (n == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            goto exit;
        else
            return n;
    }

    int total_length = bin_read_i32(buf);
    received += count;

    // Read from the client the remaining packet
    while (received < total_length) {
        n = read(fd, buf + received, total_length - received);
        if (n == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            else
                return n;
        }

        received += n;
    }

exit:
    return received;
}
