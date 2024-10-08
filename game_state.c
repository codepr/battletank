// Game state management, Tank and bullet logics, including collision
#include "game_state.h"

#include <stdlib.h>

#define RANDOM(min, max) min + rand() / (RAND_MAX / (max - min + 1) + 1)

static void init_bullet(const Tank *tank, Bullet *bullet) {
    bullet->active = false;
    bullet->x = tank->x;
    bullet->y = tank->y;
    bullet->direction = tank->direction;
}

void game_state_init(Game_State *state) {
    state->active_players = 0;
    state->power_up.x = 0;
    state->power_up.y = 0;
    state->power_up.kind = NONE;
    for (size_t i = 0; i < MAX_PLAYERS; ++i) {
        state->players[i].x = 0;
        state->players[i].y = 0;
        state->players[i].hp = 0;
        state->players[i].direction = IDLE;
        state->players[i].alive = false;
        for (size_t j = 0; j < MAX_AMMO; ++j)
            init_bullet(&state->players[i], &state->players[i].bullet[j]);
    }
}

void game_state_free(Game_State *state) { free(state->players); }

void game_state_spawn_tank(Game_State *state, size_t index) {
    if (!state->players[index].alive) {
        state->players[index].alive = true;
        state->players[index].hp = BASE_HP;
        state->players[index].x = RANDOM(15, SCREEN_WIDTH);
        state->players[index].y = RANDOM(15, SCREEN_HEIGHT);
        state->players[index].direction = IDLE;
        state->active_players++;
    }
}

void game_state_dismiss_tank(Game_State *state, size_t index) {
    state->players[index].alive = false;
    state->players[index].hp = 0;
    state->active_players--;
}

void game_state_generate_power_up(Game_State *state) {
    state->power_up.x = RANDOM(1, SCREEN_WIDTH);
    state->power_up.y = RANDOM(1, SCREEN_HEIGHT);
    state->power_up.kind = RANDOM(1, 3);
}

static void fire_bullet(Tank *tank) {
    for (int i = 0; i < MAX_AMMO; ++i) {
        if (!tank->bullet[i].active) {
            tank->bullet[i].active = true;
            tank->bullet[i].x = tank->x;
            tank->bullet[i].y = tank->y;
            tank->bullet[i].direction = tank->direction;
            break;
        }
    }
}

void game_state_update_tank(Game_State *state, size_t tank_index,
                            unsigned action) {
    switch (action) {
        case UP:
            state->players[tank_index].y -= 3;
            state->players[tank_index].direction = UP;
            break;
        case DOWN:
            state->players[tank_index].y += 3;
            state->players[tank_index].direction = DOWN;
            break;
        case LEFT:
            state->players[tank_index].x -= 3;
            state->players[tank_index].direction = LEFT;
            break;
        case RIGHT:
            state->players[tank_index].x += 3;
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
            bullet->y -= 6;
            break;
        case DOWN:
            bullet->y += 6;
            break;
        case LEFT:
            bullet->x -= 8;
            break;
        case RIGHT:
            bullet->x += 8;
            break;
        default:
            break;
    }

    if (bullet->x < 0 || bullet->x >= SCREEN_WIDTH || bullet->y < 0 ||
        bullet->y >= SCREEN_HEIGHT) {
        bullet->active = false;
    }
}

static void check_power_up(Game_State *state, Tank *tank) {
    if (state->power_up.kind == NONE) return;

    switch (state->power_up.kind) {
        case HP_PLUS_ONE:
            if (tank->y == state->power_up.y &&
                (tank->x == state->power_up.x ||
                 tank->x == state->power_up.x + 1 ||
                 tank->x == state->power_up.x + 2 ||
                 tank->x == state->power_up.x + 3)) {
                tank->hp++;
                state->power_up.kind = NONE;
            }
            break;
        case HP_PLUS_THREE:
            if (tank->y == state->power_up.y &&
                (tank->x == state->power_up.x ||
                 tank->x == state->power_up.x + 1 ||
                 tank->x == state->power_up.x + 2 ||
                 tank->x == state->power_up.x + 3)) {
                tank->hp += 3;
                state->power_up.kind = NONE;
            }
            break;
        case AMMO_PLUS_ONE:
            // TODO make this meaningful
            break;
        default:
            break;
    }
}

static void check_collision(Tank *tank, Bullet *bullet) {
    if (bullet->active && tank->x == bullet->x && tank->y == bullet->y) {
        tank->hp--;
        if (tank->hp < 0) tank->hp = 0;
        if (tank->hp == 0) tank->alive = false;
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
 *     bullet using `check_collision`.
 * - Skips collision checks between a player and their own bullet.
 */
void game_state_update(Game_State *state) {
    Bullet *bullets[MAX_PLAYERS][MAX_AMMO];
    for (size_t i = 0; i < MAX_PLAYERS; ++i) {
        check_power_up(state, &state->players[i]);
        for (size_t j = 0; j < MAX_AMMO; ++j) {
            update_bullet(&state->players[i].bullet[j]);
            bullets[i][j] = &state->players[i].bullet[j];
        }
    }

    for (size_t i = 0; i < MAX_PLAYERS; ++i) {
        for (size_t k = 0; k < MAX_PLAYERS; ++k) {
            for (size_t j = 0; j < MAX_AMMO; ++j) {
                if (k == i) continue;
                check_collision(&state->players[i], bullets[k][j]);
            }
        }
    }
}

// TODO make this more efficient by counting the ammo directly in
// the player state instead of scanning for active
int game_state_ammo(const Game_State *state, size_t index) {
    int count = 0;
    for (int i = 0; i < MAX_AMMO; ++i) {
        if (!state->players[index].bullet[i].active) count++;
    }
    return count;
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
