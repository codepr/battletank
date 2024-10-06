#include "sprite.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

int sprite_load(struct sprite *sprite, const char *path, Sprite_Kind kind,
                float scaling) {
    const char *subpath = kind == SPACESHIP ? "spaceships" : "bullets";
    int n = snprintf(sprite->path, TEXTURE_PATH_SIZE, "%s/%s/%s", ASSETS_PATH,
                     subpath, path);
    if (n < 0) return n;

    if (strncmp(path, "bullet-", 7) == 0)
        sprite->kind = BULLET;
    else
        sprite->kind = SPACESHIP;

    sprite->scaling = scaling;
    sprite->texture = LoadTexture(sprite->path);

    return 0;
}

void sprite_render_rotated(const struct sprite *sprite, float x, float y,
                           float rotation) {
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

Sprite_Repo sprite_repo_new(void) {
    return (Sprite_Repo){
        .count = 0, .capacity = 4, .sprites = calloc(4, sizeof(struct sprite))};
}

static void sprite_repo_add(Sprite_Repo *repo, struct sprite sprite) {
    if (repo->count == repo->capacity - 1) {
        repo->capacity *= 2;
        repo->sprites =
            realloc(repo->sprites, sizeof(repo->sprites[0]) * repo->capacity);
    }
    repo->sprites[repo->count++] = sprite;
}

int sprite_repo_init(Sprite_Repo *repo, Sprite_Kind kind) {
    char pathbuf[TEXTURE_PATH_SIZE];
    const char *subpath = kind == SPACESHIP ? "spaceships" : "bullets";
    int n = snprintf(pathbuf, sizeof(pathbuf), "%s/%s", ASSETS_PATH, subpath);
    if (n < 0) return -1;

    struct dirent **namelist;
    int err = 0, ok = 0;
    n = scandir(pathbuf, &namelist, NULL, alphasort);
    if (n == -1) {
        err = -1;
        goto exit;
    }

    for (int i = 0; i < n; ++i) {
        const char *dot = strrchr(namelist[i]->d_name, '.');
        if (strncmp(dot, ".png", 4) == 0) {
            struct sprite s;
            err = sprite_load(&s, namelist[i]->d_name, kind,
                              kind == SPACESHIP ? 0.08f : 0.12f);
            sprite_repo_add(repo, s);
        }

        free(namelist[i]);

        if (err < 0) goto exit;
    }

    free(namelist);

    return 0;

exit:
    free(namelist);
    return err;
}

void sprite_repo_deinit(Sprite_Repo *repo) {
    for (size_t i = 0; i < repo->count; ++i)
        UnloadTexture(repo->sprites[i].texture);
}

void sprite_repo_free(Sprite_Repo *repo) { free(repo->sprites); }

int sprite_repo_get(const Sprite_Repo *repo, struct sprite *sprite, size_t i) {
    if (i >= repo->capacity) return -1;

    *sprite = repo->sprites[i];
    return 0;
}
