#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <stdbool.h>
#include <stdio.h>

#define MAX_AMMO      5
#define MAX_PLAYERS   5
#define BASE_HP       3

#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600

// Possible directions a tank or bullet can move.
typedef enum { IDLE, UP, DOWN, LEFT, RIGHT } Direction;

// Only fire for now, can add something else such as
// DROP_MINE etc.
typedef enum {
    FIRE = 5,
} Action;

// Power-ups are meant to be spawned randomly in the battlefield, whoever
// steps on one gets the bonus, first arrived first served
typedef enum { NONE, HP_PLUS_ONE, HP_PLUS_THREE, AMMO_PLUS_ONE } Power_Up;

// Represents a bullet with its position, direction, and status.
// Can include bullet kinds as a possible update for the future.
typedef struct {
    int x;
    int y;
    Direction direction;
    bool active;
} Bullet;

// Represents a tank with its position, direction, and status.
// Contains a single bullet for simplicity, can be extended in
// the future to handle multiple bullets, life points, power-ups etc.
typedef struct {
    int x;
    int y;
    int hp;
    Direction direction;
    bool alive;
    Bullet bullet[MAX_AMMO];
} Tank;

typedef struct {
    Tank players[MAX_PLAYERS];
    size_t active_players;
    size_t player_index;
    struct {
        int x, y;
        Power_Up kind;
    } power_up;
} Game_State;

// General game state managing
void game_state_init(Game_State *state);
void game_state_free(Game_State *state);
void game_state_update(Game_State *state);
void game_state_generate_power_up(Game_State *state);

// Tank management
void game_state_spawn_tank(Game_State *state, size_t index);
void game_state_dismiss_tank(Game_State *state, size_t index);
void game_state_update_tank(Game_State *state, size_t tank_index,
                            unsigned action);
int game_state_ammo(const Game_State *state, size_t index);

const char *str_action(unsigned action);

#endif
