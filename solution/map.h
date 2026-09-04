#ifndef MAP_H
# define MAP_H

# include "vec2.h"
# include "camera.h"
# include "shot.h"

# define MAX_OBSTACLES      64
# define COLLISION_DIST     1.2f
# define OBSTACLE_HEIGHT    2.0f
# define MIN_HEIGHT         0.0f
# define MAX_HEIGHT         3.5f
# define MIN_SIDE           -6.0f
# define MAX_SIDE           6.0f

typedef enum e_event
{
    EVENT_NONE,
    EVENT_DIED,
    EVENT_WON
}   t_event;

typedef struct s_obstacle
{
    t_vec2  pos;
    int     sprite_id;
    int     destroyed;
}   t_obstacle;

typedef struct s_map
{
    t_obstacle  obstacles[MAX_OBSTACLES];
    int         count;
    float       finish_dist;
}   t_map;

int     map_load(t_map *map, char const *path);
t_event map_check_collision(t_map const *map, t_camera const *cam,
            float cam_height);
t_event map_check_finish(t_map const *map, t_camera const *cam);
void    map_check_shots(t_map *map, t_shot shots[MAX_SHOTS]);
void    map_reset(t_map *map);

#endif
