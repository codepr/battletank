#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <stdbool.h>
#include <stdio.h>

// Possible directions a tank or bullet can move.
typedef enum { IDLE, UP, DOWN, LEFT, RIGHT } Direction;

// Only fire for now, can add something else such as
// DROP_MINE etc.
typedef enum {
    FIRE = 5,
} Action;

// Represents a bullet with its position, direction, and status.
// Can include bullet kinds as a possible update for the future.
typedef struct {
    int x;
    int y;
    Direction direction;
    bool active;
} Bullet;

// Represents a tank with its position, direction, and status.
// Contains a single bullet for simplicity, can be extendend in
// the future to handle multiple bullets, life points, power-ups etc.
typedef struct {
    int x;
    int y;
    Direction direction;
    bool alive;
    Bullet bullet;
} Tank;

typedef struct {
    Tank *players;
    size_t players_count;
    size_t player_index;
} Game_State;

// General game state managing
void game_state_init(Game_State *state);
void game_state_free(Game_State *state);
void game_state_update(Game_State *state);

// Tank management
void game_state_spawn_tank(Game_State *state, size_t index);
void game_state_dismiss_tank(Game_State *state, size_t index);
void game_state_update_tank(Game_State *state, size_t tank_index,
                            unsigned action);

const char *str_action(unsigned action);

#endif
