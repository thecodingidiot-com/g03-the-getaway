#include <fcntl.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include "libtci.h"
#include "map.h"

static void trim_newline(char *line)
{
    size_t  len;

    len = tci_strlen(line);
    if (len > 0 && line[len - 1] == '\n')
        line[len - 1] = '\0';
}

static int  parse_int_line(int fd, int *out)
{
    char    *line;

    line = tci_getline(fd);
    if (!line)
        return (0);
    trim_newline(line);
    *out = tci_atoi(line);
    free(line);
    return (1);
}

static int  parse_obstacle_line(int fd, t_obstacle *ob)
{
    char    *line;
    char    *first_space;
    char    *second_space;

    line = tci_getline(fd);
    if (!line)
        return (0);
    trim_newline(line);
    first_space = tci_strchr(line, ' ');
    if (!first_space) {
        free(line);
        return (0);
    }
    *first_space = '\0';
    second_space = tci_strchr(first_space + 1, ' ');
    if (!second_space) {
        free(line);
        return (0);
    }
    *second_space = '\0';
    ob->pos.x = (float)tci_atoi(line);
    ob->pos.y = (float)tci_atoi(first_space + 1);
    ob->sprite_id = tci_atoi(second_space + 1);
    ob->destroyed = 0;
    free(line);
    return (1);
}

int map_load(t_map *map, char const *path)
{
    int fd;
    int finish_dist_int;
    int count;
    int i;

    fd = open(path, O_RDONLY);
    if (fd < 0)
        return (0);
    if (!parse_int_line(fd, &finish_dist_int)) {
        close(fd);
        return (0);
    }
    map->finish_dist = (float)finish_dist_int;
    if (!parse_int_line(fd, &count) || count < 0 || count > MAX_OBSTACLES) {
        close(fd);
        return (0);
    }
    map->count = 0;
    i = 0;
    while (i < count) {
        if (!parse_obstacle_line(fd, &map->obstacles[map->count])) {
            close(fd);
            return (0);
        }
        map->count++;
        i++;
    }
    close(fd);
    return (1);
}

t_event map_check_collision(t_map const *map, t_camera const *cam,
        float cam_height)
{
    t_vec2  diff;
    float   dist;
    int     i;

    if (cam_height >= OBSTACLE_HEIGHT)
        return (EVENT_NONE);
    i = 0;
    while (i < map->count) {
        if (!map->obstacles[i].destroyed) {
            diff = vec2_sub(cam->pos, map->obstacles[i].pos);
            dist = sqrtf(diff.x * diff.x + diff.y * diff.y);
            if (dist < COLLISION_DIST)
                return (EVENT_DIED);
        }
        i++;
    }
    return (EVENT_NONE);
}

t_event map_check_finish(t_map const *map, t_camera const *cam)
{
    float   dist;

    dist = sqrtf(cam->pos.x * cam->pos.x + cam->pos.y * cam->pos.y);
    if (dist >= map->finish_dist)
        return (EVENT_WON);
    return (EVENT_NONE);
}

/* shot.c knows nothing about obstacles -- this is the glue between a
** generic projectile and this game's own world, the same split
** map_check_collision() already draws between "distance" (shared
** idea) and "what counts as a hit" (this game's own rule). A shot
** that's flying above OBSTACLE_HEIGHT misses everything at ground
** level for the same reason the player does at that height: nothing
** here can hit what it's already flying clear over. */
void    map_check_shots(t_map *map, t_shot shots[MAX_SHOTS])
{
    t_vec2  diff;
    float   dist;
    int     i;
    int     j;

    i = 0;
    while (i < MAX_SHOTS) {
        if (shots[i].active && shots[i].height < OBSTACLE_HEIGHT) {
            j = 0;
            while (j < map->count) {
                if (!map->obstacles[j].destroyed) {
                    diff = vec2_sub(shots[i].pos, map->obstacles[j].pos);
                    dist = sqrtf(diff.x * diff.x + diff.y * diff.y);
                    if (dist < SHOT_HIT_DIST) {
                        map->obstacles[j].destroyed = 1;
                        shots[i].active = 0;
                    }
                }
                j++;
            }
        }
        i++;
    }
}

/* Every obstacle destroyed this run comes back for the next one --
** a restart is a fresh attempt at the same course, the same reason
** camera_init() puts the camera back at the start instead of wherever
** it crashed. */
void    map_reset(t_map *map)
{
    int i;

    i = 0;
    while (i < map->count) {
        map->obstacles[i].destroyed = 0;
        i++;
    }
}
