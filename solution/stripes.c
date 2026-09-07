#include <math.h>
#include "stripes.h"

float   stripes_lane_offset(int lane)
{
    return (((float)lane - (float)(LANE_COUNT - 1) / 2.0f) * LANE_SPACING);
}

/* The same WINDOW_H / depth the scaler uses for size, applied sideways:
** a lane one unit off centre is WINDOW_H / depth pixels off centre. */
int     stripes_screen_x(float lane_y, float cam_y, float depth)
{
    return (WINDOW_W / 2
        + (int)((float)WINDOW_H * (lane_y - cam_y) / depth));
}

/* Geometric, not linear. Depth is what 1/depth compresses, so stepping
** evenly through depth puts nearly every sample at the horizon; stepping
** evenly through 1/depth spaces the samples evenly on screen instead. */
float   stripes_step_depth(int step)
{
    float   t;

    t = (float)step / (float)STRIPE_STEPS;
    return (1.0f / ((1.0f / STRIPE_NEAR)
        + t * ((1.0f / STRIPE_FAR) - (1.0f / STRIPE_NEAR))));
}

int     stripes_dash_on(float pos_x, float depth)
{
    float   phase;

    phase = fmodf(pos_x + depth, DASH_LENGTH * 2.0f);
    if (phase < 0.0f)
        phase += DASH_LENGTH * 2.0f;
    return (phase < DASH_LENGTH);
}
