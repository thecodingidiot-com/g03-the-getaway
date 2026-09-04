#ifndef ROAD_H
# define ROAD_H

# include "vec2.h"
# include "camera.h"

# define MAX_OBSTACLES      64
# define COLLISION_DIST     1.2f
# define OBSTACLE_HEIGHT    2.0f
# define MIN_HEIGHT         0.0f
# define MAX_HEIGHT         3.5f

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
}   t_obstacle;

typedef struct s_road
{
    t_obstacle  obstacles[MAX_OBSTACLES];
    int         count;
    float       finish_dist;
}   t_road;

int     road_load(t_road *road, char const *path);
t_event road_check_collision(t_road const *road, t_camera const *cam,
            float cam_height);
t_event road_check_finish(t_road const *road, t_camera const *cam);

#endif
