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
** First step: filled bands between consecutive depths, rather than
** marks at them. The spacing is unchanged for now, so this is a
** restructure and nothing else -- and the near-field test still fails,
** which is the useful part. Bands alone were not the answer.
**
** forward is (1, 0) for the whole game -- main.c steers by strafing,
** never by rotating -- so depth along the view is just distance along
** world x, and the phase is fmodf(pos.x, spacing).
*/
# define STRIPE_SPACING     6.0f
# define STRIPE_COUNT       14
# define STRIPE_Y_SCALE     0.5f

float   stripes_edge_depth(float pos_x, int i);
int     stripes_row(float depth);

#endif
