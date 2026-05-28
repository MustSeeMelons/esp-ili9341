#ifndef SCENE_H_
#define SCENE_H_

typedef struct {
    void (*scene_init)(void *ctx);
    void (*scene_update)(void *ctx);
} scene_t;

#endif