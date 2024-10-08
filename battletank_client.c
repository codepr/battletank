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
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

#include "game_state.h"
#include "network.h"
#include "protocol.h"
#include "raylib.h"
#include "sprite.h"

#define BUFSIZE 2048

Sprite_Repo sprite_repo;

/*
 * RENDERING HELPERS
 * =================
 * For the time being this represents the sole "graphic" layer, it's so small
 * it can comfortably live embedded in the client module.
 */
static void render_tank(const Tank *const tank, size_t i) {
    if (tank->alive) {
        struct sprite tank_sprite;
        sprite_repo_get(&sprite_repo, &tank_sprite, SPACESHIP, i);

        float rotation = 0.0f;
        switch (tank->direction) {
            case DOWN:
                rotation = 180.0f;
                break;
            case LEFT:
                rotation = 270.0f;
                break;
            case RIGHT:
                rotation = 90.0f;
                break;
            default:
                break;
        }

        sprite_render_rotated(&tank_sprite, (float)tank->x, (float)tank->y,
                              rotation);
    }
}

static void render_bullet(const Bullet *const bullet) {
    if (bullet->active) {
        // Draw the bullet at its current position, to do it
        // we first load the texture from the repository
        // TODO although the operation is pretty inexpensive as at this
        // point it's just a lookup O(1) in an arraya we can probably
        // attach the sprite directly to the tank and avoid this lookup
        // altogether
        struct sprite bullet_sprite;
        sprite_repo_get(&sprite_repo, &bullet_sprite, BULLET, 0);

        float rotation = 0.0f;
        switch (bullet->direction) {
            case DOWN:
                rotation = 90.0f;
                break;
            case LEFT:
                rotation = 180.0f;
                break;
            case UP:
                rotation = 270.0f;
                break;
            default:
                break;
        }
        sprite_render_rotated(&bullet_sprite, (float)bullet->x,
                              (float)bullet->y, rotation);
    }
}

static void render_power_up(const Game_State *state) {
    if (state->power_up.kind == NONE) return;

    struct sprite powerup_sprite;
    sprite_repo_get(&sprite_repo, &powerup_sprite, POWERUP, 0);

    switch (state->power_up.kind) {
        case HP_PLUS_ONE:
            sprite_render(&powerup_sprite, state->power_up.x, state->power_up.y,
                          YELLOW);
            break;
        case HP_PLUS_THREE:
            sprite_render(&powerup_sprite, state->power_up.x, state->power_up.y,
                          DARKGREEN);
            break;
        case AMMO_PLUS_ONE:
            sprite_render(&powerup_sprite, state->power_up.x, state->power_up.y,
                          DARKBLUE);
            break;
        default:
            break;
    }
}

static void render_stats(const Game_State *state, size_t index) {
    int bullet_count = game_state_ammo(state, index);
    DrawText(TextFormat("X: %d Y: %d", state->players[index].x,
                        state->players[index].y),
             1, 1, 10, DARKBLUE);
    DrawText(TextFormat("HP: %d", state->players[index].hp), 1, 12, 10,
             DARKBLUE);

    DrawText(TextFormat("AMMO: %d", bullet_count), 1, 24, 10, DARKBLUE);
}

static void render_game(const Game_State *state, size_t index) {
    BeginDrawing();
    ClearBackground(BLACK);
    for (size_t i = 0; i < MAX_PLAYERS; ++i) {
        render_tank(&state->players[i], i);
        for (size_t j = 0; j < MAX_AMMO; ++j)
            render_bullet(&state->players[i].bullet[j]);
    }

    render_power_up(state);
    render_stats(state, index);

    EndDrawing();
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

static int client_send_data(int sockfd, const unsigned char *data,
                            size_t datasize) {
    ssize_t n = network_send(sockfd, data, datasize);
    if (n < 0) {
        perror("write() error");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    return n;
}

static int client_recv_data(int sockfd, unsigned char *data) {
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
    unsigned char buf[BUFSIZE];
    // Sync the game state for the first time
    int n = client_recv_data(sockfd, buf);
    protocol_deserialize_game_state(buf, &state);
    size_t index = state.player_index;
    unsigned action = IDLE;
    bool is_direction = false;
    bool can_fire = false;
    float key_cooldown = 0.02f;  // 200 ms between keypresses
    float last_key_press_time = 0.0f;

    while (!WindowShouldClose()) {
        float current_time = GetTime();
        if (current_time - last_key_press_time > key_cooldown) {
            action = IDLE;
            if (IsKeyDown(KEY_RIGHT)) {
                action = RIGHT;
                last_key_press_time = current_time;
            } else if (IsKeyDown(KEY_LEFT)) {
                action = LEFT;
                last_key_press_time = current_time;
            } else if (IsKeyDown(KEY_UP)) {
                action = UP;
                last_key_press_time = current_time;
            } else if (IsKeyDown(KEY_DOWN)) {
                action = DOWN;
                last_key_press_time = current_time;
            }
            if (IsKeyPressed(KEY_SPACE)) {
                action = FIRE;
                last_key_press_time = current_time;
            }

            is_direction = (action == UP || action == DOWN || action == LEFT ||
                            action == RIGHT);
            can_fire = (action == FIRE && game_state_ammo(&state, index) > 0);
            if (is_direction || can_fire) {
                memset(buf, 0x00, sizeof(buf));
                n = protocol_serialize_action(action, buf);
                client_send_data(sockfd, buf, n);
            }
        }
        n = client_recv_data(sockfd, buf);
        // render the battlefield and the tanks only when some payload is
        // actually received
        if (n > (int)sizeof(int)) {
            protocol_deserialize_game_state(buf, &state);
            render_game(&state, index);
        }
    }
}

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT,
               "raylib battletank (or spacebattle)");

    sprite_repo_load(&sprite_repo);

    SetTargetFPS(120);
    game_loop();

    sprite_repo_free(&sprite_repo);

    CloseWindow();  // Close window and OpenGL context
    return 0;
}
