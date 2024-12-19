#ifndef SPRITE_H
#define SPRITE_H

#include <stdio.h>

#include "raylib.h"

#define ASSETS_PATH           "./assets"
#define TEXTURE_PATH_SIZE     192
#define SPRITE_SPACESHIP_PATH "spaceships"
#define SPRITE_BULLET_PATH    "bullets"
#define SPRITE_POWERUP_PATH   "powerups"

typedef enum { SPACESHIP, BULLET, POWERUP, SPRITES_COUNT } Sprite_Kind;

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
} Sprite_Collection;

typedef struct {
    Sprite_Collection collections[SPRITES_COUNT];
} Sprite_Repo;

// Granular sprite managing, loading and rendering
struct sprite sprite_load(const char *path, Sprite_Kind kind, float scaling);
void sprite_render(const struct sprite *sprite, float x, float y, Color tint);
void sprite_render_rotated(const struct sprite *sprite, float x, float y,
                           float rotation);

// Sprite collection managing
Sprite_Collection sprite_collection_new(void);
int sprite_collection_load(Sprite_Collection *collection, Sprite_Kind kind);
void sprite_collection_unload(Sprite_Collection *collection);
void sprite_collection_free(Sprite_Collection *collection);
int sprite_collection_get(const Sprite_Collection *collection,
                          struct sprite *sprite, size_t i);

// Sprite repo managing
void sprite_repo_load(Sprite_Repo *repo);
void sprite_repo_get(const Sprite_Repo *repo, struct sprite *sprite,
                     Sprite_Kind kind, size_t i);
void sprite_repo_free(Sprite_Repo *repo);
#endif
