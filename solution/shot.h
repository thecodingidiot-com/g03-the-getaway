#ifndef SHOT_H
# define SHOT_H

# include "vec2.h"

# define MAX_SHOTS      8
# define SHOT_SPEED     2.0f
# define SHOT_LIFETIME  120
# define SHOT_HIT_DIST  1.3f
# define FIRE_COOLDOWN  12

typedef struct s_shot
{
    t_vec2  pos;
    float   height;
    int     age;
    int     active;
}   t_shot;

void    shot_fire(t_shot shots[MAX_SHOTS], t_vec2 pos, float height);
void    shot_update(t_shot shots[MAX_SHOTS]);

#endif
