#ifndef STRIPES_H
# define STRIPES_H

# include "scaler.h"

/*
** The ground's moving reference marks: lane lines running away from the
** viewer, converging on the vanishing point.
**
** They used to be horizontal bands crossing the whole screen. Two
** things were wrong with that. It gave no lateral cue at all -- strafing
** left and right moved nothing, because a full-width band looks the
** same wherever you stand. And the bands themselves were nearly all
** horizon: 1/depth compresses everything past the first one into the
** same few rows, so most of the pattern was a smear at eye level with
** the near field empty.
**
** Lines along the direction of travel fix both. Their screen x depends
** on where you are laterally, so strafing slides them across the view;
** and the dashes along them move toward you, so the forward cue stays.
**
** forward is (1, 0) for the whole game -- main.c steers by strafing,
** never by rotating -- so depth is distance along world x and lateral
** offset is distance along world y.
**
** Where a depth lands on screen vertically is ground.h's question, not
** this file's. That separation is the whole point of the two units:
** the lines and the obstacles stand on the same ground because they ask
** the same function.
*/
# define LANE_COUNT     8
# define LANE_SPACING   3.0f
# define DASH_LENGTH    3.0f
# define STRIPE_NEAR    2.0f
# define STRIPE_FAR     120.0f
# define STRIPE_STEPS   64

/* World lateral offset of lane i, centred on the road. An even count on
** purpose: an odd one puts a lane exactly under the ship, which renders
** as a vertical pole up the middle of the screen rather than a road
** marking. */
float   stripes_lane_offset(int lane);

/* Screen column of a lane at this depth. Converges on WINDOW_W / 2. */
int     stripes_screen_x(float lane_y, float cam_y, float depth);

/* Depth of sample step s, spaced so near steps are short and far ones
** long -- even steps in depth would spend most of them at the horizon. */
float   stripes_step_depth(int step);

/* Is the dash painted at this depth? The pattern is fixed in the world,
** so the phase comes from how far along x the camera has driven. */
int     stripes_dash_on(float pos_x, float depth);

#endif
