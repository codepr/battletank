// Game state management, Tank and bullet logics, including collision
#include "game_state.h"

#include <ncurses.h>
#include <stdlib.h>

#define RANDOM(min, max) min + rand() / (RAND_MAX / (max - min + 1) + 1)

void game_state_init(Game_State *state) {
    state->players_count = 2;
    state->players = calloc(2, sizeof(Tank));
    for (size_t i = 0; i < state->players_count; ++i) {
        state->players[i].alive = false;
        state->players[i].bullet.active = false;
    }
}

void game_state_free(Game_State *state) { free(state->players); }

void game_state_spawn_tank(Game_State *state, size_t index) {
    // Extend the players pool if we're at capacity
    if (index > state->players_count) {
        state->players_count *= 2;
        state->players = realloc(state->players, state->players_count);
    }

    if (!state->players[index].alive) {
        state->players[index].alive = true;
        state->players[index].x = RANDOM(15, 25);
        state->players[index].y = RANDOM(15, 25);
        state->players[index].direction = 0;
    }
}

void game_state_dismiss_tank(Game_State *state, size_t index) {
    state->players[index].alive = false;
}

static void fire_bullet(Tank *tank) {
    if (!tank->bullet.active) {
        tank->bullet.active = true;
        tank->bullet.x = tank->x;
        tank->bullet.y = tank->y;
        tank->bullet.direction = tank->direction;
    }
}

void game_state_update_tank(Game_State *state, size_t tank_index,
                            unsigned action) {
    switch (action) {
        case UP:
            state->players[tank_index].y--;
            state->players[tank_index].direction = UP;
            break;
        case DOWN:
            state->players[tank_index].y++;
            state->players[tank_index].direction = DOWN;
            break;
        case LEFT:
            state->players[tank_index].x--;
            state->players[tank_index].direction = LEFT;
            break;
        case RIGHT:
            state->players[tank_index].x++;
            state->players[tank_index].direction = RIGHT;
            break;
        case FIRE:
            fire_bullet(&state->players[tank_index]);
            break;
        default:
            break;
    }
}

static void update_bullet(Bullet *bullet) {
    if (!bullet->active) return;

    switch (bullet->direction) {
        case UP:
            bullet->y--;
            break;
        case DOWN:
            bullet->y++;
            break;
        case LEFT:
            bullet->x -= 2;
            break;
        case RIGHT:
            bullet->x += 2;
            break;
        default:
            break;
    }

    if (bullet->x < 0 || bullet->x >= COLS || bullet->y < 0 ||
        bullet->y >= LINES) {
        bullet->active = false;
    }
}

static void check_collision(Tank *tank, Bullet *bullet) {
    if (bullet->active && tank->x == bullet->x && tank->y == bullet->y) {
        tank->alive = false;
        bullet->active = false;
    }
}

/**
 * Updates the game state by advancing bullets and checking for collisions
 * between tanks and bullets.
 *
 * - Creates an array of pointers to each player's bullet for easy access during
 *   collision checks.
 * - For each player:
 *   - Updates their bullet by calling `update_bullet`.
 *   - Checks for collisions between the player's tank and every other player's
 * bullet using `check_collision`.
 * - Skips collision checks between a player and their own bullet.
 */
void game_state_update(Game_State *state) {
    Bullet *bullets[state->players_count];
    for (size_t i = 0; i < state->players_count; ++i)
        bullets[i] = &state->players[i].bullet;

    for (size_t i = 0; i < state->players_count; ++i) {
        update_bullet(&state->players[i].bullet);
        for (size_t j = 0; j < state->players_count; ++j) {
            if (j == i) continue;  // Skip self collision
            check_collision(&state->players[i], bullets[j]);
        }
    }
}

const char *str_action(unsigned action) {
    switch (action) {
        case IDLE:
            return "IDLE";
        case UP:
            return "UP";
        case DOWN:
            return "DOWN";
        case LEFT:
            return "LEFT";
        case RIGHT:
            return "RIGHT";
        case FIRE:
            return "FIRE";
    }

    return "UNKNOWN";
}
