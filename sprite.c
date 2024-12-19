#include "sprite.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "raylib.h"

static float scalings[SPRITES_COUNT] = {0.12f, 0.06f, 0.12f};

struct sprite sprite_load(const char *path, Sprite_Kind kind, float scaling)
{
    struct sprite sprite;
    const char *subpath = kind == SPACESHIP ? SPRITE_SPACESHIP_PATH
                          : kind == POWERUP ? SPRITE_POWERUP_PATH
                                            : SPRITE_BULLET_PATH;
    int n = snprintf(sprite.path, TEXTURE_PATH_SIZE, "%s/%s/%s", ASSETS_PATH,
                     subpath, path);
    if (n < 0) fprintf(stderr, "snprintf encoding failure\n");

    sprite.kind    = kind;
    sprite.scaling = scaling;
    sprite.texture = LoadTexture(sprite.path);

    return sprite;
}

void sprite_render(const struct sprite *sprite, float x, float y, Color tint)
{
    Vector2 position = {(int)x, (int)y};
    DrawTextureEx(sprite->texture, position, 0.0f, sprite->scaling, tint);
}

/*
 * This function allows the rendering of a texture with rotation from its
 * center: Raylib by default rotates textures from the top-left corner.
 */
void sprite_render_rotated(const struct sprite *sprite, float x, float y,
                           float rotation)
{
    Rectangle src_rec = {0.0f, 0.0f, (float)sprite->texture.width,
                         (float)sprite->texture.height};

    // Define destination rectangle (where and how to draw the texture)
    Rectangle dst_rec = {
        x, y, sprite->texture.width * sprite->scaling,
        sprite->texture.height * sprite->scaling};  // Position and

    // Define origin for rotation (center of the texture)
    Vector2 origin = {(sprite->texture.width * sprite->scaling) / 2.0f,
                      (sprite->texture.height * sprite->scaling) / 2.0f};

    DrawTexturePro(sprite->texture, src_rec, dst_rec, origin, rotation, WHITE);
}

Sprite_Collection sprite_collection_new(void)
{
    return (Sprite_Collection){
        .count = 0, .capacity = 4, .sprites = calloc(4, sizeof(struct sprite))};
}

static void sprite_collection_add(Sprite_Collection *collection,
                                  struct sprite sprite)
{
    if (collection->count == collection->capacity - 1) {
        collection->capacity *= 2;
        collection->sprites =
            realloc(collection->sprites,
                    sizeof(collection->sprites[0]) * collection->capacity);
    }
    collection->sprites[collection->count++] = sprite;
}

// Scans the assets directory on the FS for textures
int sprite_collection_load(Sprite_Collection *collection, Sprite_Kind kind)
{
    if (kind >= SPRITES_COUNT) return -1;
    char pathbuf[TEXTURE_PATH_SIZE];
    const char *subpath = kind == SPACESHIP ? SPRITE_SPACESHIP_PATH
                          : kind == POWERUP ? SPRITE_POWERUP_PATH
                                            : SPRITE_BULLET_PATH;
    int n = snprintf(pathbuf, sizeof(pathbuf), "%s/%s", ASSETS_PATH, subpath);
    if (n < 0) return -1;

    struct stat st;
    if (stat(pathbuf, &st) != 0) return -1;

    struct dirent **namelist;
    int err = 0;
    n       = scandir(pathbuf, &namelist, NULL, alphasort);
    if (n == -1) {
        err = -1;
        goto exit;
    }

    for (int i = 0; i < n; ++i) {
        const char *dot = strrchr(namelist[i]->d_name, '.');
        if (strncmp(dot, ".png", 4) == 0) {
            struct sprite s =
                sprite_load(namelist[i]->d_name, kind, scalings[kind]);
            sprite_collection_add(collection, s);
        }

        free(namelist[i]);
    }

    free(namelist);

    return 0;

exit:
    free(namelist);
    return err;
}

void sprite_collection_unload(Sprite_Collection *collection)
{
    for (size_t i = 0; i < collection->count; ++i)
        UnloadTexture(collection->sprites[i].texture);
}

void sprite_collection_free(Sprite_Collection *collection)
{
    free(collection->sprites);
}

int sprite_collection_get(const Sprite_Collection *collection,
                          struct sprite *sprite, size_t i)
{
    if (i >= collection->capacity) return -1;

    *sprite = collection->sprites[i];
    return 0;
}

void sprite_repo_load(Sprite_Repo *repo)
{
    int err = 0;
    for (Sprite_Kind kind = SPACESHIP; kind < SPRITES_COUNT; ++kind) {
        repo->collections[kind] = sprite_collection_new();
        err = sprite_collection_load(&repo->collections[kind], kind);
        if (err < 0)
            fprintf(stderr, "error loading sprites of kind [%d]\n", kind);
    }
}

void sprite_repo_get(const Sprite_Repo *repo, struct sprite *sprite,
                     Sprite_Kind kind, size_t i)
{
    int err = sprite_collection_get(&repo->collections[kind], sprite, i);
    if (err < 0)
        fprintf(stderr, "failed to get sprites collection of kind %d - %ld\n",
                kind, i);
}

void sprite_repo_free(Sprite_Repo *repo)
{
    for (Sprite_Kind kind = SPACESHIP; kind < SPRITES_COUNT; ++kind) {
        sprite_collection_unload(&repo->collections[kind]);
        sprite_collection_free(&repo->collections[kind]);
    }
}
