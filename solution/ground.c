#include "ground.h"

/*
** One formula, two doors.
**
** The billboard side arrives holding a projection and asks where its
** feet go; the stripe side arrives holding a depth and asks where that
** row of ground is. Those were separate functions in separate files
** until the two branches met -- ground_row() taking a projection,
** stripes_row() taking a depth -- and they computed the same thing,
** because proj->size IS (int)(WINDOW_H / depth). Neither was wrong.
** They just could not see each other.
*/
static int  row_from_size(int size)
{
    return (HORIZON_Y + (int)((float)size * GROUND_DROP_SCALE));
}

int     ground_row(t_projection const *proj)
{
    return (row_from_size(proj->size));
}

int     ground_row_at(float depth)
{
    return (row_from_size((int)((float)WINDOW_H / depth)));
}

void    ground_place(t_projection *proj)
{
    proj->screen_y = ground_row(proj) - proj->size;
}

int     ground_shot_row(float depth, int ship_row)
{
    if (depth < NEAR_PLANE)
        depth = NEAR_PLANE;
    return (HORIZON_Y + (int)((float)(ship_row - HORIZON_Y)
        * (NEAR_PLANE / depth)));
}
