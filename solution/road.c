#include <fcntl.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include "libtci.h"
#include "road.h"

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
    free(line);
    return (1);
}

int road_load(t_road *road, char const *path)
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
    road->finish_dist = (float)finish_dist_int;
    if (!parse_int_line(fd, &count) || count < 0 || count > MAX_OBSTACLES) {
        close(fd);
        return (0);
    }
    road->count = 0;
    i = 0;
    while (i < count) {
        if (!parse_obstacle_line(fd, &road->obstacles[road->count])) {
            close(fd);
            return (0);
        }
        road->count++;
        i++;
    }
    close(fd);
    return (1);
}

t_event road_check_collision(t_road const *road, t_camera const *cam)
{
    t_vec2  diff;
    float   dist;
    int     i;

    i = 0;
    while (i < road->count) {
        diff = vec2_sub(cam->pos, road->obstacles[i].pos);
        dist = sqrtf(diff.x * diff.x + diff.y * diff.y);
        if (dist < COLLISION_DIST)
            return (EVENT_DIED);
        i++;
    }
    return (EVENT_NONE);
}

t_event road_check_finish(t_road const *road, t_camera const *cam)
{
    float   dist;

    dist = sqrtf(cam->pos.x * cam->pos.x + cam->pos.y * cam->pos.y);
    if (dist >= road->finish_dist)
        return (EVENT_WON);
    return (EVENT_NONE);
}
