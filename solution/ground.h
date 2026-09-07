#ifndef GROUND_H
# define GROUND_H

# include "scaler.h"

/*
** Where the ground is on screen, at a given depth.
**
** One question, one answer. It used to have two: the stripes descended
** from the horizon while billboards sat on it, and nothing could tell
** because the constants lived behind an SDL2 include.
**
** The scaler already computes the only quantity this needs. `size` is
** WINDOW_H / depth -- the inverse-depth measure everything in this
** renderer is scaled by -- so the ground drops away from the horizon at
** exactly the rate a billboard at the same depth grows. Reusing it
** instead of dividing again is what keeps the two in step.
**
** This does not live in scaler.c on purpose. r01 has no ground, and
** g04 carries the same scaler; a ground plane is g03's idea, not the
** scaler's, and scaler.c stays byte for byte what those chapters have.
*/
# define GROUND_DROP_SCALE  0.5f

int     ground_row(t_projection const *proj);

#endif
