/*
 *            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
 *                    Version 2, December 2004
 *
 * Copyright (C) 2004 Sam Hocevar <sam@hocevar.net>
 *
 * Everyone is permitted to copy and distribute verbatim or modified
 * copies of this license document, and changing it is allowed as long
 * as the name is changed.
 *
 *            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
 *   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION
 *
 *  0. You just DO WHAT THE FUCK YOU WANT TO.
 *
 * Main server, handles the game state and serves as the unique authoritative
 * source of truth.
 *
 * - clients sync at their first connection and their tank is spawned in the
 *   battlefield, the server will send a unique identifier to the clients (an
 *   int index for the time being, that represents the tank assigned to the
 *   player in the game state)
 * - the server continually broadcasts the game state to keep the clients in
 *   sync
 * - clients will send actions to the servers such as movements or bullet fire
 * - the server will update the general game state and let it be broadcast in
 *   the following cycle
 */
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <ncurses.h>
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

#include "game_state.h"
#include "network.h"
#include "protocol.h"

// We don't expect big payloads
#define BUFSIZE 1024
#define BACKLOG 128
#define TIMEOUT 26000
#define POWERUP_COUNTER 70

// Generic global game state
static Game_State game_state = {0};

/* Set non-blocking socket */
static int set_nonblocking(int fd) {
    int flags, result;
    flags = fcntl(fd, F_GETFL, 0);

    if (flags == -1) goto err;

    result = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    if (result == -1) goto err;

    return 0;

err:

    fprintf(stderr, "set_nonblocking: %s\n", strerror(errno));
    return -1;
}

static int server_listen(const char *host, int port, int backlog) {
    int listen_fd = -1;
    const struct addrinfo hints = {.ai_family = AF_UNSPEC,
                                   .ai_socktype = SOCK_STREAM,
                                   .ai_flags = AI_PASSIVE};
    struct addrinfo *result, *rp;
    char port_str[6];

    snprintf(port_str, 6, "%i", port);

    if (getaddrinfo(host, port_str, &hints, &result) != 0) goto err;

    /* Create a listening socket */
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        listen_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (listen_fd < 0) continue;

        /* set SO_REUSEADDR so the socket will be reusable after process kill */
        if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1},
                       sizeof(int)) < 0)
            goto err;

        /* Bind it to the addr:port opened on the network interface */
        if (bind(listen_fd, rp->ai_addr, rp->ai_addrlen) == 0)
            break;  // Succesful bind
        close(listen_fd);
    }

    freeaddrinfo(result);
    if (rp == NULL) goto err;

    /*
     * Let's make the socket non-blocking (strongly advised to use the
     * eventloop)
     */
    (void)set_nonblocking(listen_fd);

    /* Finally let's make it listen */
    if (listen(listen_fd, backlog) != 0) goto err;

    return listen_fd;
err:
    return -1;
}

static int server_accept(int server_fd) {
    int fd;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    /* Let's accept on listening socket */
    fd = accept(server_fd, (struct sockaddr *)&addr, &addrlen);
    if (fd <= 0) goto exit;

    (void)set_nonblocking(fd);
    return fd;
exit:
    if (errno != EWOULDBLOCK && errno != EAGAIN) perror("accept");
    return -1;
}

static int broadcast(int *client_fds, const unsigned char *buf, size_t count) {
    int written = 0;
    for (int i = 0; i < FD_SETSIZE; i++) {
        if (client_fds[i] >= 0) {
            // TODO check for errors writing
            written += network_send(client_fds[i], buf, count);
        }
    }

    return written;
}

static unsigned long long get_microseconds_timestamp(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    // Convert the time to microseconds
    return (unsigned long long)(ts.tv_sec * 1000000 + ts.tv_nsec / 1000);
}

static void server_loop(int server_fd) {
    fd_set readfds;
    int client_fds[FD_SETSIZE];
    int maxfd = server_fd;
    int i = 0;
    unsigned char buf[BUFSIZE];
    struct timeval tv = {0, TIMEOUT};
    size_t spawn_counter = 0;
    unsigned long long current_time_ns = 0, remaining_us = 0,
                       last_update_time_ns = 0;

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

        memset(buf, 0x00, sizeof(buf));

        int num_events = select(maxfd + 1, &readfds, NULL, NULL, &tv);

        if (num_events == -1) {
            perror("select() error");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(server_fd, &readfds)) {
            // New connection request
            int client_fd = server_accept(server_fd);
            if (client_fd < 0) {
                perror("accept() error");
                continue;
            }

            for (i = 0; i < FD_SETSIZE; i++) {
                if (client_fds[i] < 0) {
                    if (i > MAX_PLAYERS) {
                        printw("Players limit reached, dropping connection");
                        i = FD_SETSIZE;
                        break;
                    }
                    client_fds[i] = client_fd;
                    game_state.player_index = i;
                    break;
                }
            }

            if (i == FD_SETSIZE) {
                fprintf(stderr, "Too many clients\n");
                close(client_fd);
                continue;
            }

            printw("[info] New player connected\n");
            printw("[info] Syncing game state\n");
            printw("[info] Player assigned [%ld] tank\n",
                   game_state.player_index);

            // Spawn a tank in a random position for the new connected
            // player
            game_state_spawn_tank(&game_state, game_state.player_index);
            printw("[info] Tank for player-%d spawned\n",
                   game_state.player_index);
        }

        for (i = 0; i < FD_SETSIZE; i++) {
            int fd = client_fds[i];
            if (fd >= 0 && FD_ISSET(fd, &readfds)) {
                ssize_t count = network_recv(fd, buf);
                if (count <= 0) {
                    close(fd);
                    game_state_dismiss_tank(&game_state, i);
                    client_fds[i] = -1;
                    printw("[info] Player-%d disconnected\n", i);
                } else {
                    unsigned action = 0;
                    protocol_deserialize_action(buf, &action);
                    printw(
                        "[info] Received an action %s from player-%d (%ld "
                        "bytes)\n",
                        str_action(action), i, count);
                    game_state_update_tank(&game_state, i, action);
                    printw("[info] Updating game state completed\n");
                }
            }
        }
        // Poor man periodic task
        if (++spawn_counter >= POWERUP_COUNTER) {
            spawn_counter = 0;
            game_state_generate_power_up(&game_state);
            printw("[info] Generated power up\n");
        }

        // Send update to the connected clients, currently with a TIMEOUT of
        // 16ms is roughly equal to 60 FPS. Checks for the last update sent and
        // adjust the select timeout so to make it as precise and smooth as
        // possible and respect the deadline
        current_time_ns = get_microseconds_timestamp();
        remaining_us = current_time_ns - last_update_time_ns;
        if (remaining_us >= TIMEOUT) {
            // Main update loop here
            game_state_update(&game_state);
            size_t bytes = protocol_serialize_game_state(&game_state, buf);
            broadcast(client_fds, buf, bytes);
            last_update_time_ns = get_microseconds_timestamp();
            tv.tv_sec = 0;
            tv.tv_usec = TIMEOUT;
        } else {
            tv.tv_sec = 0;
            tv.tv_usec = TIMEOUT - remaining_us;
        }
        // We're using ncurses for convienince to initialise ROWS and LINES
        // without going raw mode in the terminal, this requires a refresh to
        // print the logs
        refresh();
    }
}

int main(void) {
    srand(time(NULL));
    // Use ncurses as its handy to calculate the screen size
    initscr();
    scrollok(stdscr, TRUE);

    printw("[info] Starting server %d %d\n", COLS, LINES);
    game_state_init(&game_state);

    int server_fd = server_listen("127.0.0.1", 6699, BACKLOG);
    if (server_fd < 0) exit(EXIT_FAILURE);

    server_loop(server_fd);

    return 0;
}
