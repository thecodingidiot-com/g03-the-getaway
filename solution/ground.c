#include "ground.h"

int     ground_row(t_projection const *proj)
{
    return (HORIZON_Y + (int)((float)proj->size * GROUND_DROP_SCALE));
}

void    ground_place(t_projection *proj)
{
    proj->screen_y = ground_row(proj) - proj->size;
}
