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
 * Client, connects to the battletank server, provides a very crude terminal
 * based graphic battlefield and handles input from the player.
 *
 * - client connects to the game server and syncs with the game state, receiving
 *   an index that uniquely identifies the player tank in the game state
 * - the server continually broadcasts the game state to keep the clients in
 *   sync
 * - clients will send actions to the servers such as movements or bullet fire
 * - the server will update the general game state and let it be broadcast in
 *   the following cycle
 */
#include <arpa/inet.h>
#include <ncurses.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

#include "game_state.h"
#include "network.h"
#include "protocol.h"

#define BUFSIZE 1024

/*
 * RENDERING HELPERS
 * =================
 * For the time being this represents the sole "graphic" layer, it's so small
 * it can comfortably live embedded in the client module.
 */
static void init_screen(void) {
    // Start curses mode
    initscr();
    cbreak();
    // Don't echo keypresses to the screen
    noecho();
    // Enable keypad mode
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    // Hide the cursor
    curs_set(FALSE);
}

static void render_tank(const Tank *const tank) {
    if (tank->alive) {
        // Draw the tank at its current position
        mvaddch(tank->y, tank->x, 'T');
    }
}

static void render_bullet(const Bullet *const bullet) {
    if (bullet->active) {
        // Draw the bullet at its current position
        mvaddch(bullet->y, bullet->x, 'o');
    }
}

static void render_power_up(const Game_State *state) {
    if (state->power_up.kind == NONE) return;

    switch (state->power_up.kind) {
        case HP_PLUS_ONE:
            mvprintw(state->power_up.y, state->power_up.x, "HP+1");
            break;
        case HP_PLUS_THREE:
            mvprintw(state->power_up.y, state->power_up.x, "HP+3");
            break;
        case AMMO_PLUS_ONE:
            mvprintw(state->power_up.y, state->power_up.x, "HP+1");
            break;
        default:
            break;
    }
}

static void render_stats(const Game_State *state, size_t index) {
    int bullet_count = game_state_ammo(state, index);
    mvprintw(0, 0, "HP: %d", state->players[index].hp);
    mvprintw(1, 0, "AMMO: %d", bullet_count);
}

static void render_game(const Game_State *state, size_t index) {
    clear();
    for (size_t i = 0; i < MAX_PLAYERS; ++i) {
        render_tank(&state->players[i]);
        for (size_t j = 0; j < AMMO; ++j)
            render_bullet(&state->players[i].bullet[j]);
    }

    render_power_up(state);

    render_stats(state, index);
    refresh();
}

static unsigned handle_input(void) {
    unsigned action = IDLE;
    int ch = getch();
    switch (ch) {
        case KEY_UP:
            action = UP;
            break;
        case KEY_DOWN:
            action = DOWN;
            break;
        case KEY_LEFT:
            action = LEFT;
            break;
        case KEY_RIGHT:
            action = RIGHT;
            break;
        case ' ':
            action = FIRE;
            break;
    }

    return action;
}

/*
 * NETWORK HELPERS
 * ===============
 * Just some connection and read/write throught the wire helpers
 */
static int socket_connect(const char *host, int port) {
    struct sockaddr_in serveraddr;
    struct hostent *server;
    struct timeval tv = {0, 10000};

    // socket: create the socket
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd < 0) goto err;

    setsockopt(sfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval));
    setsockopt(sfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(struct timeval));

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

static int client_connect(const char *host, int port) {
    return socket_connect(host, port);
}

static int client_send_data(int sockfd, const char *data, size_t datasize) {
    ssize_t n = network_send(sockfd, data, datasize);
    if (n < 0) {
        perror("write() error");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    return n;
}

static int client_recv_data(int sockfd, char *data) {
    ssize_t n = network_recv(sockfd, data);
    if (n < 0) {
        perror("read() error");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    return n;
}

// Main game loop, capture input from the player and communicate with the game
// server
static void game_loop(void) {
    int sockfd = client_connect("127.0.0.1", 6699);
    if (sockfd < 0) exit(EXIT_FAILURE);
    Game_State state;
    game_state_init(&state);
    char buf[BUFSIZE];
    // Sync the game state for the first time
    int n = client_recv_data(sockfd, buf);
    protocol_deserialize_game_state(buf, &state);
    size_t index = state.player_index;
    unsigned action = IDLE;
    bool valid_action = false;
    bool is_direction = false;
    bool can_fire = false;

    while (1) {
        action = handle_input();
        is_direction = (action == UP || action == DOWN || action == LEFT ||
                        action == RIGHT);
        can_fire = (action == FIRE && game_state_ammo(&state, index) > 0);
        valid_action = is_direction || can_fire;
        if (valid_action) {
            memset(buf, 0x00, sizeof(buf));
            n = protocol_serialize_action(action, buf);
            client_send_data(sockfd, buf, n);
        }
        n = client_recv_data(sockfd, buf);
        protocol_deserialize_game_state(buf, &state);
        render_game(&state, index);
    }
}

int main(void) {
    init_screen();
    game_loop();

    endwin();
    return 0;
}
