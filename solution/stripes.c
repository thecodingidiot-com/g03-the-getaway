#include <math.h>
#include "stripes.h"

/* The near edge of band i: the pattern is fixed in the world, so the
** phase comes from how far along x the camera has driven. */
float   stripes_edge_depth(float pos_x, int i)
{
    return (STRIPE_SPACING - fmodf(pos_x, STRIPE_SPACING)
        + (float)i * STRIPE_SPACING);
}
