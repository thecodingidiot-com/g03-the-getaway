#include "ground.h"

int     ground_row(t_projection const *proj)
{
    return (HORIZON_Y + (int)((float)proj->size * GROUND_DROP_SCALE));
}
