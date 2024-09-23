#include <ncurses.h>

#include "battletank-client.h"

// Possible directions a tank or bullet can move.
typedef enum { UP, DOWN, LEFT, RIGHT } Direction;

// Represents a tank with its position, direction, and status.
typedef struct {
    int x;
    int y;
    Direction direction;
    bool alive;
} Tank;

// Represents a bullet with its position, direction, and status.
typedef struct {
    int x;
    int y;
    Direction direction;
    bool active;
} Bullet;

void init_screen(void) {
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

void render_tank(const Tank *const tank) {
    if (tank->alive) {
        // Draw the tank at its current position
        mvaddch(tank->y, tank->x, 'T');
    }
}

void render_bullet(const Bullet *const bullet) {
    if (bullet->active) {
        // Draw the bullet at its current position
        mvaddch(bullet->y, bullet->x, 'o');
    }
}

void render_game(const Tank *const tank, const Bullet *const bullet) {
    clear();
    render_tank(tank);
    render_bullet(bullet);
    refresh();
}

void update_bullet(Bullet *bullet) {
    if (!bullet->active) return;
    switch (bullet->direction) {
        case UP:
            bullet->y--;
            break;
        case DOWN:
            bullet->y++;
            break;
        case LEFT:
            bullet->x -= 3;
            break;
        case RIGHT:
            bullet->x += 3;
            break;
    }

    if (bullet->x < 0 || bullet->x >= COLS || bullet->y < 0 ||
        bullet->y >= LINES) {
        bullet->active = false;
    }
}

void fire_bullet(Tank *tank, Bullet *bullet) {
    if (!bullet->active) {
        bullet->active = true;
        bullet->x = tank->x;
        bullet->y = tank->y;
        bullet->direction = tank->direction;
    }
}

void check_collision(Tank *tank, Bullet *bullet) {
    if (bullet->active && tank->x == bullet->x && tank->y == bullet->y) {
        tank->alive = false;
        bullet->active = false;
    }
}

void handle_input(Tank *tank, Bullet *bullet) {
    int ch = getch();
    switch (ch) {
        case KEY_UP:
            tank->y--;
            tank->direction = UP;
            break;
        case KEY_DOWN:
            tank->y++;
            tank->direction = DOWN;
            break;
        case KEY_LEFT:
            tank->x--;
            tank->direction = LEFT;
            break;
        case KEY_RIGHT:
            tank->x++;
            tank->direction = RIGHT;
            break;
        case ' ':
            fire_bullet(tank, bullet);
            break;
    }
}

void game_loop(Tank *tank, Bullet *bullet) {
    while (1) {
        handle_input(tank, bullet);
        update_bullet(bullet);
        check_collision(tank, bullet);
        render_game(tank, bullet);
        napms(50);
    }
}

int main(void) {
    Tank player = {5, 5, 0, 1};
    Bullet bullet = {0, 0, 0, 0};

    init_screen();
    game_loop(&player, &bullet);

    endwin();
    return 0;
}
