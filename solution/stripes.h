#ifndef STRIPES_H
# define STRIPES_H

# include "scaler.h"

/*
** The ground's moving reference marks.
**
** These used to be thin stripes: one thin rect per marked depth, every
** other one skipped. Measured over a full cycle of the pattern, that put
** an average of 0.68 marks below screen row 360 -- and for 39 frames in
** 120, none at all. The near field, the 120 pixels closest to the
** viewer where motion is most legible, was empty most of the time. Six
** of the seven marks in a typical frame landed in a ten-pixel smear at
** the horizon, because 1/depth compresses everything beyond the first
** one into the same few rows.
**
** Bands between consecutive depths, rather than marks at them -- and
** a shorter spacing, because bands alone still left the near field
** empty for 39 frames in 120. Sweeping spacing against count:
**
**     spacing  count   near-field px   empty frames
**         6.0     14              28             39
**         4.0     20              35              0
**         3.0     26              28              0
**         2.5     32              27              0
**         2.0     40              20              0
**
** 4.0 covers the most and is the first that is never empty. Shorter
** spacings do not help: past a point the extra bands land in the same
** compressed rows at the horizon and cost near-field coverage.
**
** forward is (1, 0) for the whole game -- main.c steers by strafing,
** never by rotating -- so depth along the view is just distance along
** world x, and the phase is fmodf(pos.x, spacing).
*/
# define STRIPE_SPACING     4.0f
# define STRIPE_COUNT       20
# define STRIPE_Y_SCALE     0.5f

float   stripes_edge_depth(float pos_x, int i);
int     stripes_row(float depth);

#endif
