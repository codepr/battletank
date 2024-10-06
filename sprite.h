#ifndef SPRITE_H
#define SPRITE_H

#include <stdio.h>

#include "raylib.h"

#define ASSETS_PATH "./assets"
#define TEXTURE_PATH_SIZE 192

typedef enum { SPACESHIP, BULLET } Sprite_Kind;

struct sprite {
    char path[TEXTURE_PATH_SIZE];
    float scaling;
    Sprite_Kind kind;
    Texture2D texture;
};

typedef struct {
    size_t capacity;
    size_t count;
    struct sprite *sprites;
} Sprite_Repo;

typedef Sprite_Repo Spaceship_Repo;

typedef Sprite_Repo Bullet_Repo;

int sprite_load(struct sprite *sprite, const char *path, Sprite_Kind kind,
                float scaling);
void sprite_render_rotated(const struct sprite *sprite, float x, float y,
                           float rotation);

Sprite_Repo sprite_repo_new(void);
int sprite_repo_init(Sprite_Repo *repo, Sprite_Kind kind);
void sprite_repo_deinit(Sprite_Repo *repo);
void sprite_repo_free(Sprite_Repo *repo);
int sprite_repo_get(const Sprite_Repo *repo, struct sprite *sprite, size_t i);

#endif
