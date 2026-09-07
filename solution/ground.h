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

/*
** The same row, asked for by depth rather than by projection. The
** stripe pattern has a depth in hand and no billboard to project, so it
** comes in this way. Both doors reach one formula; that is the whole
** point of them living here together.
*/
int     ground_row_at(float depth);

/*
** Stand a projected billboard on that ground: its base lands on
** ground_row(), not on the horizon. scaler_project() cannot do this --
** it is shared, byte for byte, with r01 and g04, neither of which has a
** ground -- so the placement happens here, on this side of the call,
** and stays testable without a display.
*/
void    ground_place(t_projection *proj);

/*
** The screen row of a shot at this depth.
**
** Shots cannot be stood on the ground like an obstacle, because the
** thing that fires them is not on the ground either: the ship is drawn
** in screen space -- centre-bottom, rising with cam_height -- and never
** projected at all, the way Space Harrier's own player sprite never is.
** Anything projected will therefore disagree with it, which is exactly
** what a shot standing on the ground looked like: it dropped to the
** floor while the ship sat at the top of the screen.
**
** So a shot is placed by where it must begin and where it must end. At
** the near plane it is at the ship. Far away it is at the vanishing
** point. In between it follows the same 1/depth every other distance in
** this renderer follows.
*/
int     ground_shot_row(float depth, int ship_row);

#endif
