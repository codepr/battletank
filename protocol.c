/*
 * Serialization and deserialization of data structures related to the game,
 * including game state, tank, and bullet objects.
 * Includes some general utility functions to serialize and deserialize
 * 32 bit integers over the wire.
 */
#include "protocol.h"

#define SIZEOF_TANK sizeof(int) * 2 + sizeof(unsigned char) * 2
#define SIZEOF_BULLET sizeof(int) * 2 + sizeof(unsigned char) * 2

void bin_write_i32(char *buf, int val) {
    *buf++ = val >> 24;
    *buf++ = val >> 16;
    *buf++ = val >> 8;
    *buf++ = val;
}

int bin_read_i32(const char *buf) {
    int i2 =
        ((int)buf[0] << 24) | ((int)buf[1] << 16) | ((int)buf[2] << 8) | buf[3];
    int val;

    // change unsigned numbers to signed
    if ((unsigned)i2 <= 0x7fffffffu)
        val = i2;
    else
        val = -1 - (int)(0xffffffffu - i2);

    return val;
}

static int protocol_serialize_bullet(const Bullet *bullet, char *buf) {
    bin_write_i32(buf, bullet->x);
    buf += sizeof(int);

    bin_write_i32(buf, bullet->y);
    buf += sizeof(int);

    *buf++ = bullet->active;
    *buf++ = bullet->direction;

    return SIZEOF_BULLET;
}

static int protocol_serialize_tank(const Tank *tank, char *buf) {
    // Serialize the tank
    bin_write_i32(buf, tank->x);
    buf += sizeof(int);

    bin_write_i32(buf, tank->y);
    buf += sizeof(int);

    *buf++ = tank->alive;
    *buf++ = tank->direction;

    // Serialize the bullet
    int offset = 0;
    for (int i = 0; i < AMMO; ++i)
        offset += protocol_serialize_bullet(&tank->bullet[i], buf + offset);

    return offset + SIZEOF_TANK;
}

static int protocol_deserialize_bullet(const char *buf, Bullet *bullet) {
    bullet->x = bin_read_i32(buf);
    buf += sizeof(int);

    bullet->y = bin_read_i32(buf);
    buf += sizeof(int);

    bullet->active = *buf++;
    bullet->direction = *buf++;

    return SIZEOF_BULLET;
}

static int protocol_deserialize_tank(const char *buf, Tank *tank) {
    tank->x = bin_read_i32(buf);
    buf += sizeof(int);

    tank->y = bin_read_i32(buf);
    buf += sizeof(int);

    tank->alive = *buf++;
    tank->direction = *buf++;

    int offset = 0;
    for (int i = 0; i < AMMO; ++i)
        offset += protocol_deserialize_bullet(buf + offset, &tank->bullet[i]);

    return offset + SIZEOF_TANK;
}

/*
 * The serialized binary representation of the game state:
 *
 * The structure is simple enough for the time being, just a
 * players count followed by the tanks, each tank has only a
 * single bullet.
 *
 * Header
 * ------
 * bytes (1-4):    total packet length (38 bytes)
 * bytes (5-9):    player index
 * bytes (10-14):  players count
 *
 * State (suppose 1 tank)
 * -----
 * For each player (e.g. <players count> players)
 * bytes (15-19)   x
 * bytes (20-24)   y
 * bytes (25)      alive
 * bytes (26)      direction
 *
 * Bullet
 * ------
 * bytes (27-31)   x
 * bytes (32-36)   y
 * bytes (37)      active
 * bytes (38)      direction
 */
int protocol_serialize_game_state(const Game_State *state, char *buf) {
    // Serialize the game state header
    int active_players = state->active_players;
    // Total length will include itself in the full length of the packet
    int total_length = (SIZEOF_TANK + (SIZEOF_BULLET * AMMO)) * MAX_PLAYERS +
                       (sizeof(int) * 3);

    bin_write_i32(buf, total_length);
    int offset = sizeof(int);

    // Player index
    bin_write_i32(buf + offset, state->player_index);
    offset += sizeof(int);

    // Players count
    bin_write_i32(buf + offset, active_players);
    offset += sizeof(int);

    // Serialize each player
    for (int i = 0; i < MAX_PLAYERS; i++) {
        offset += protocol_serialize_tank(&state->players[i], buf + offset);
    }

    return offset;
}

int protocol_deserialize_game_state(const char *buf, Game_State *state) {
    // Deserialize the game state header
    int total_length = bin_read_i32(buf);
    buf += sizeof(int);

    state->player_index = bin_read_i32(buf);
    buf += sizeof(int);

    state->active_players = bin_read_i32(buf);
    buf += sizeof(int);

    int offset = 0;

    // Deserialize the players
    for (size_t i = 0; i < MAX_PLAYERS; ++i) {
        offset += protocol_deserialize_tank(buf + offset, &state->players[i]);
    }

    return total_length;
}

int protocol_serialize_action(unsigned action, char *buf) {
    // Total length will include itself in the full length of the packet
    int total_length = sizeof(int) + sizeof(unsigned char);

    bin_write_i32(buf, total_length);
    int offset = sizeof(int);

    *(buf + offset) = action;
    return total_length;
}

int protocol_deserialize_action(const char *buf, unsigned *action) {
    int total_length = bin_read_i32(buf);
    buf += sizeof(int);

    *action = *buf;
    return total_length;
}
