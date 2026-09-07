#include <math.h>
#include "stripes.h"

/* The near edge of band i: the pattern is fixed in the world, so the
** phase comes from how far along x the camera has driven. */
float   stripes_edge_depth(float pos_x, int i)
{
    return (STRIPE_SPACING - fmodf(pos_x, STRIPE_SPACING)
        + (float)i * STRIPE_SPACING);
}

/* Where a ground depth lands on screen. The scaler's own WINDOW_H/depth
** is the inverse-depth measure everything here is scaled by, so the
** ground recedes at exactly the rate a billboard at that depth grows. */
int     stripes_row(float depth)
{
    return (HORIZON_Y + (int)((float)(int)((float)WINDOW_H / depth)
        * STRIPE_Y_SCALE));
}
