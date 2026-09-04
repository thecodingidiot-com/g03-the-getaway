#include "shot.h"

/* A fixed-size pool, not a dynamic list -- the same shape every fixed-
** count array in this project already uses (obstacles, markers). No
** allocation, no free, just an "active" flag per slot. */
void    shot_fire(t_shot shots[MAX_SHOTS], t_vec2 pos, float height)
{
    int i;

    i = 0;
    while (i < MAX_SHOTS) {
        if (!shots[i].active) {
            shots[i].pos = pos;
            shots[i].height = height;
            shots[i].age = 0;
            shots[i].active = 1;
            return ;
        }
        i++;
    }
}

/* This game's world only ever has one forward direction -- +x, always,
** because nothing here ever rotates the camera. A shot just steps
** along it and ages out after SHOT_LIFETIME frames, so a miss doesn't
** stay "active" forever waiting for an obstacle that's already behind
** it. */
void    shot_update(t_shot shots[MAX_SHOTS])
{
    int i;

    i = 0;
    while (i < MAX_SHOTS) {
        if (shots[i].active) {
            shots[i].pos.x += SHOT_SPEED;
            shots[i].age++;
            if (shots[i].age > SHOT_LIFETIME)
                shots[i].active = 0;
        }
        i++;
    }
}
