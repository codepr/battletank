/*
 * Serialization and deserialization of data structures related to the game,
 * including game state, tank, and bullet objects.
 * Includes some general utility functions to serialize and deserialize
 * 32 bit integers over the wire.
 */
#include "protocol.h"

#define SIZEOF_TANK sizeof(int) * 3 + sizeof(unsigned char) * 2
#define SIZEOF_BULLET sizeof(int) * 2 + sizeof(unsigned char) * 2

void bin_write_i32(unsigned char *buf, unsigned long val) {
    *buf++ = val >> 24;
    *buf++ = val >> 16;
    *buf++ = val >> 8;
    *buf++ = val;
}

long int bin_read_i32(const unsigned char *buf) {
    unsigned long i2 = ((unsigned long int)buf[0] << 24) |
                       ((unsigned long int)buf[1] << 16) |
                       ((unsigned long int)buf[2] << 8) | buf[3];
    long int val;

    // change unsigned numbers to signed
    if (i2 <= 0x7fffffffu)
        val = i2;
    else
        val = -1 - (long int)(0xffffffffu - i2);

    return val;
}

static int protocol_serialize_bullet(const Bullet *bullet, unsigned char *buf) {
    bin_write_i32(buf, bullet->x);
    buf += sizeof(int);

    bin_write_i32(buf, bullet->y);
    buf += sizeof(int);

    *buf++ = bullet->active;
    *buf++ = bullet->direction;

    return SIZEOF_BULLET;
}

static int protocol_serialize_tank(const Tank *tank, unsigned char *buf) {
    // Serialize the tank
    bin_write_i32(buf, tank->x);
    buf += sizeof(int);

    bin_write_i32(buf, tank->y);
    buf += sizeof(int);

    bin_write_i32(buf, tank->hp);
    buf += sizeof(int);

    *buf++ = tank->alive;
    *buf++ = tank->direction;

    // Serialize the bullet
    int offset = 0;
    for (int i = 0; i < MAX_AMMO; ++i)
        offset += protocol_serialize_bullet(&tank->bullet[i], buf + offset);

    return offset + SIZEOF_TANK;
}

static int protocol_deserialize_bullet(const unsigned char *buf,
                                       Bullet *bullet) {
    bullet->x = bin_read_i32(buf);
    buf += sizeof(int);

    bullet->y = bin_read_i32(buf);
    buf += sizeof(int);

    bullet->active = *buf++;
    bullet->direction = *buf++;

    return SIZEOF_BULLET;
}

static int protocol_deserialize_tank(const unsigned char *buf, Tank *tank) {
    tank->x = bin_read_i32(buf);
    buf += sizeof(int);

    tank->y = bin_read_i32(buf);
    buf += sizeof(int);

    tank->hp = bin_read_i32(buf);
    buf += sizeof(int);

    tank->alive = *buf++;
    tank->direction = *buf++;

    int offset = 0;
    for (int i = 0; i < MAX_AMMO; ++i)
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
 * bytes (1-4)     total packet length (54 bytes)
 * bytes (5-9)     player index
 * bytes (10-14)   active players count
 * bytes (15-19)   active power-up x
 * bytes (20-24)   active power-up y
 * bytes (25)      power-up kind
 *
 * State (suppose 1 tank)
 * -----
 * For each player (e.g. <players count> players)
 * bytes (26-30)   x
 * bytes (31-35)   y
 * bytes (36-40)   hp
 * bytes (41)      alive
 * bytes (42)      direction
 *
 * Bullet
 * ------
 * bytes (43-47)   x
 * bytes (48-52)   y
 * bytes (53)      active
 * bytes (54)      direction
 */
int protocol_serialize_game_state(const Game_State *state, unsigned char *buf) {
    // Serialize the game state header
    int active_players = state->active_players;
    // Total length will include itself in the full length of the packet
    int total_length =
        ((SIZEOF_TANK + (SIZEOF_BULLET * MAX_AMMO)) * MAX_PLAYERS) +
        (sizeof(int) * 5) + sizeof(unsigned char);

    bin_write_i32(buf, total_length);
    int offset = sizeof(int);

    // Player index
    bin_write_i32(buf + offset, state->player_index);
    offset += sizeof(int);

    // Players count
    bin_write_i32(buf + offset, active_players);
    offset += sizeof(int);

    // Power up
    bin_write_i32(buf + offset, state->power_up.x);
    offset += sizeof(int);

    bin_write_i32(buf + offset, state->power_up.y);
    offset += sizeof(int);

    *(buf + offset) = state->power_up.kind;
    offset++;

    // Serialize each player
    for (int i = 0; i < MAX_PLAYERS; i++) {
        offset += protocol_serialize_tank(&state->players[i], buf + offset);
    }

    return offset;
}

int protocol_deserialize_game_state(const unsigned char *buf,
                                    Game_State *state) {
    // Deserialize the game state header
    int total_length = bin_read_i32(buf);
    buf += sizeof(int);

    state->player_index = bin_read_i32(buf);
    buf += sizeof(int);

    state->active_players = bin_read_i32(buf);
    buf += sizeof(int);

    state->power_up.x = bin_read_i32(buf);
    buf += sizeof(int);

    state->power_up.y = bin_read_i32(buf);
    buf += sizeof(int);

    state->power_up.kind = *buf++;

    int offset = 0;

    // Deserialize the players
    for (size_t i = 0; i < MAX_PLAYERS; ++i) {
        offset += protocol_deserialize_tank(buf + offset, &state->players[i]);
    }

    return total_length;
}

int protocol_serialize_action(unsigned action, unsigned char *buf) {
    // Total length will include itself in the full length of the packet
    int total_length = sizeof(int) + sizeof(unsigned char);

    bin_write_i32(buf, total_length);
    int offset = sizeof(int);

    *(buf + offset) = action;
    return total_length;
}

int protocol_deserialize_action(const unsigned char *buf, unsigned *action) {
    int total_length = bin_read_i32(buf);
    buf += sizeof(int);

    *action = *buf;
    return total_length;
}
