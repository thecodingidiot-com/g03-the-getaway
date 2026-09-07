#include <math.h>
#include <stdlib.h>
#include "render.h"
#include "ground.h"
#include "stripes.h"
#include "scaler.h"

typedef struct s_draw_item
{
    t_projection    proj;
    int             sprite_id;
    int             flashing;
}   t_draw_item;

/* Painter's algorithm, unchanged from r01/r02: farthest first, so a
** near obstacle overdraws a far one exactly where they overlap. */
static int  compare_draw_items(void const *a, void const *b)
{
    t_draw_item const  *ia = a;
    t_draw_item const  *ib = b;

    if (ib->proj.depth > ia->proj.depth)
        return (1);
    if (ib->proj.depth < ia->proj.depth)
        return (-1);
    return (0);
}

/* Fixed, hand-placed decoration -- never loaded, never checked for
** collision. Scattered wider than any obstacle in map1.txt so they
** read as background, not as something to dodge. */
static t_vec2 const g_markers[] = {
    {10.0f, 8.0f}, {10.0f, -9.0f}, {22.0f, 10.0f}, {22.0f, -7.0f},
    {35.0f, 9.0f}, {35.0f, -11.0f}, {48.0f, 7.0f}, {48.0f, -9.0f},
    {58.0f, 11.0f}, {58.0f, -8.0f}, {68.0f, 9.0f}, {68.0f, -10.0f},
    {78.0f, 8.0f}, {78.0f, -9.0f},
};
# define MARKER_COUNT (int)(sizeof(g_markers) / sizeof(g_markers[0]))

/* Dark sky, dark ground, a violet horizon between them -- the same
** palette hestia's own theme and thecodingidiot.com's homepage tunnel
** already use, not a new one invented for this chapter alone. */
void    render_backdrop(SDL_Renderer *ren)
{
    SDL_Rect    sky;
    SDL_Rect    ground;

    sky.x = 0;
    sky.y = 0;
    sky.w = WINDOW_W;
    sky.h = HORIZON_Y;
    ground.x = 0;
    ground.y = HORIZON_Y;
    ground.w = WINDOW_W;
    ground.h = WINDOW_H - HORIZON_Y;
    SDL_SetRenderDrawColor(ren, 0x0f, 0x0f, 0x0f, 0xff);
    SDL_RenderFillRect(ren, &sky);
    SDL_SetRenderDrawColor(ren, 0x1c, 0x15, 0x26, 0xff);
    SDL_RenderFillRect(ren, &ground);
}

/* Space Harrier's own real floor is a moving scanline surface -- a
** different, larger technique this chapter still doesn't attempt.
** These are lane lines instead: world-space lines along the direction
** of travel, sampled at a run of depths and joined up, so they lean in
** toward the vanishing point the way road markings do.
**
** Every sample asks ground_row_at() for its row, so the lines and the
** obstacles stand on one ground. Thickness comes from the same
** WINDOW_H / depth as everything else, which is what stops a near line
** looking like a far one.
*/
void    render_ground_stripes(t_camera const *cam, SDL_Renderer *ren)
{
    float   lane_y;
    float   d0;
    float   d1;
    int     x0;
    int     x1;
    int     thickness;
    int     lane;
    int     step;
    int     t;

    SDL_SetRenderDrawColor(ren, 0x7c, 0x3a, 0xed, 0xff);
    lane = 0;
    while (lane < LANE_COUNT) {
        lane_y = stripes_lane_offset(lane);
        step = 0;
        while (step < STRIPE_STEPS) {
            d0 = stripes_step_depth(step);
            d1 = stripes_step_depth(step + 1);
            if (stripes_dash_on(cam->pos.x, d0)) {
                x0 = stripes_screen_x(lane_y, cam->pos.y, d0);
                x1 = stripes_screen_x(lane_y, cam->pos.y, d1);
                thickness = (int)((float)WINDOW_H / d0 / 40.0f);
                if (thickness < 1)
                    thickness = 1;
                if (thickness > 6)
                    thickness = 6;
                t = 0;
                while (t < thickness) {
                    SDL_RenderDrawLine(ren, x0 + t, ground_row_at(d0),
                        x1 + t, ground_row_at(d1));
                    t++;
                }
            }
            step++;
        }
        lane++;
    }
}

/* h = WINDOW_H / depth grows without bound as depth shrinks -- close
** to the camera it can dwarf the window itself. Same discipline r03's
** raycaster used (floor the value feeding a divide, not clamp what
** comes out of it) applied the only place it's possible here: scaler.c
** stays byte for byte unchanged from r01, so the cap has to live on
** this side of the call, in how the projection gets used. Re-anchor
** screen_y to the clamped size too, or a capped sprite floats above
** the ground instead of standing on it. */
static void cap_projection(t_projection *proj)
{
    if (proj->size > MAX_SPRITE_SIZE) {
        proj->size = MAX_SPRITE_SIZE;
        ground_place(proj);
    }
}

/* Ground-anchored objects sink toward the bottom of the screen the
** higher the camera climbs -- the same idea as looking down at
** something from above instead of standing level with it. Scaled by
** the object's own (already distance-scaled) size, the same way the
** size cap reuses a value scaler_project() already computed instead
** of inventing a second distance measure. */
static void apply_height_shift(t_projection *proj, float cam_height)
{
    proj->screen_y += (int)(cam_height * (float)proj->size
            * HEIGHT_SHIFT_SCALE);
}

/* A destroyed obstacle isn't skipped the instant it dies -- it still
** projects and draws for FLASH_DURATION frames, just as a solid flash
** instead of its own sprite, so a hit reads as a bright light hitting
** the target instead of the obstacle simply vanishing. Once
** flash_timer reaches zero it's finally skipped, same as before. */
void    render_map(t_map const *map, t_camera const *cam,
        float cam_height, SDL_Renderer *ren,
        SDL_Texture *sprites[SPRITE_COUNT])
{
    t_draw_item items[MAX_OBSTACLES];
    int         visible;
    int         i;
    SDL_Rect    dst;

    visible = 0;
    i = 0;
    while (i < map->count) {
        if (!map->obstacles[i].destroyed || map->obstacles[i].flash_timer > 0) {
            items[visible].proj = scaler_project(cam, map->obstacles[i].pos);
            items[visible].sprite_id = map->obstacles[i].sprite_id;
            items[visible].flashing = map->obstacles[i].destroyed;
            if (items[visible].proj.visible) {
                ground_place(&items[visible].proj);
                cap_projection(&items[visible].proj);
                apply_height_shift(&items[visible].proj, cam_height);
                visible++;
            }
        }
        i++;
    }
    qsort(items, visible, sizeof(items[0]), compare_draw_items);
    i = 0;
    while (i < visible) {
        dst.x = items[i].proj.screen_x;
        dst.y = items[i].proj.screen_y;
        dst.w = items[i].proj.size;
        dst.h = items[i].proj.size;
        if (items[i].flashing) {
            SDL_SetRenderDrawColor(ren, 0xff, 0xff, 0xff, 0xff);
            SDL_RenderFillRect(ren, &dst);
        } else
            SDL_RenderCopy(ren, sprites[items[i].sprite_id], NULL, &dst);
        i++;
    }
}

/* Same scaler_project() pipeline r01 built, applied to fixed decoration
** instead of gameplay obstacles -- shrunk after projecting, not a
** second technique. Drawn before render_map() calls, so a real
** obstacle painter's-algorithms over a marker at the same depth. */
void    render_markers(t_camera const *cam, float cam_height,
        SDL_Renderer *ren, SDL_Texture *marker_tex)
{
    t_draw_item items[MARKER_COUNT];
    int         visible;
    int         i;
    int         shrunk;
    SDL_Rect    dst;

    visible = 0;
    i = 0;
    while (i < MARKER_COUNT) {
        items[visible].proj = scaler_project(cam, g_markers[i]);
        if (items[visible].proj.visible) {
            ground_place(&items[visible].proj);
            cap_projection(&items[visible].proj);
            apply_height_shift(&items[visible].proj, cam_height);
            visible++;
        }
        i++;
    }
    qsort(items, visible, sizeof(items[0]), compare_draw_items);
    i = 0;
    while (i < visible) {
        shrunk = (int)((float)items[i].proj.size * MARKER_SCALE);
        dst.x = items[i].proj.screen_x + (items[i].proj.size - shrunk) / 2;
        dst.y = items[i].proj.screen_y + items[i].proj.size - shrunk;
        dst.w = shrunk;
        dst.h = shrunk;
        SDL_RenderCopy(ren, marker_tex, NULL, &dst);
        i++;
    }
}

/* Same scaler_project()-then-shrink shape render_markers() already
** uses, for a completely different reason: markers are shrunk because
** they're background, shots are shrunk because a bullet is small.
** Painter's algorithm still applies -- a shot flying past a near
** obstacle should draw in front of it, same as any other object at
** that depth. */
void    render_shots(t_shot const shots[MAX_SHOTS], t_camera const *cam,
        float cam_height, SDL_Renderer *ren, SDL_Texture *shot_tex)
{
    t_draw_item items[MAX_SHOTS];
    int         visible;
    int         i;
    int         shrunk;
    SDL_Rect    dst;

    visible = 0;
    i = 0;
    while (i < MAX_SHOTS) {
        if (shots[i].active) {
            items[visible].proj = scaler_project(cam, shots[i].pos);
            if (items[visible].proj.visible) {
                cap_projection(&items[visible].proj);
                items[visible].proj.screen_y = ground_shot_row(
                    items[visible].proj.depth,
                    render_ship_row(cam_height))
                    - items[visible].proj.size;
                visible++;
            }
        }
        i++;
    }
    qsort(items, visible, sizeof(items[0]), compare_draw_items);
    i = 0;
    while (i < visible) {
        shrunk = (int)((float)items[i].proj.size * SHOT_SCALE);
        dst.x = items[i].proj.screen_x + (items[i].proj.size - shrunk) / 2;
        dst.y = items[i].proj.screen_y + items[i].proj.size - shrunk;
        dst.w = shrunk;
        dst.h = shrunk;
        SDL_RenderCopy(ren, shot_tex, NULL, &dst);
        i++;
    }
}

/* No projection at all -- the player's own craft sits at a fixed
** horizontal point on screen every frame, the same way the dashboard
** in an Out Run cabinet never moves while the road does. Altitude is
** one exception: it moves the craft itself, in real pixels, not
** anything projected. Tilt is the other -- `SDL_RenderCopyEx()`
** instead of `SDL_RenderCopy()`, rotating the same texture around its
** own centre rather than moving it, the way a plane banks into a turn
** instead of sliding sideways rigid. */
/* The ship is a screen-space sprite -- centre-bottom, rising with
** cam_height -- never projected. Shots have to agree with it, so where
** its nose sits is a fact both need and neither should guess. */
int     render_ship_row(float cam_height)
{
    return (WINDOW_H - PLAYER_SPRITE_H - 24
        - (int)(cam_height * PLAYER_PX_PER_UNIT));
}

void    render_player(SDL_Renderer *ren, SDL_Texture *player_tex,
        float cam_height, float tilt)
{
    SDL_Rect    dst;

    dst.w = PLAYER_SPRITE_W;
    dst.h = PLAYER_SPRITE_H;
    dst.x = WINDOW_W / 2 - dst.w / 2;
    dst.y = render_ship_row(cam_height);
    SDL_RenderCopyEx(ren, player_tex, NULL, &dst, (double)tilt, NULL,
        SDL_FLIP_NONE);
}

/* The only text this chapter ever draws, and the only place SDL2_ttf
** gets called -- same one-file boundary render.c already keeps for
** every other SDL2 drawing call. A fresh texture per string per frame
** is exactly what g01b's own font.c already does; a HUD that's two
** short lines a frame doesn't need caching to stay honest about cost. */
static void draw_string(SDL_Renderer *ren, TTF_Font *font, int x, int y,
        char const *s)
{
    SDL_Color       white;
    SDL_Surface     *surf;
    SDL_Texture     *tex;
    SDL_Rect        dst;

    white.r = 255;
    white.g = 255;
    white.b = 255;
    white.a = 255;
    surf = TTF_RenderUTF8_Solid(font, s, white);
    if (!surf)
        return ;
    tex = SDL_CreateTextureFromSurface(ren, surf);
    SDL_FreeSurface(surf);
    if (!tex)
        return ;
    dst.x = x;
    dst.y = y;
    SDL_QueryTexture(tex, NULL, NULL, &dst.w, &dst.h);
    SDL_RenderCopy(ren, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
}

/* No tci_itoa in libtci -- this reaches for libc nowhere else in this
** function, only a plain digit-by-digit loop the same shape every
** c-tier ft_putnbr already used, reversed into place once the count
** of digits is known. */
static void int_to_str(int n, char *buf)
{
    char    tmp[12];
    int     i;
    int     j;

    i = 0;
    if (n == 0)
        tmp[i++] = '0';
    while (n > 0) {
        tmp[i++] = '0' + n % 10;
        n /= 10;
    }
    j = 0;
    while (i > 0)
        buf[j++] = tmp[--i];
    buf[j] = '\0';
}

static void format_hud_line(char *out, char const *prefix, int n,
        char const *suffix)
{
    char    num[12];
    int     i;
    int     j;

    i = 0;
    while (prefix[i]) {
        out[i] = prefix[i];
        i++;
    }
    int_to_str(n, num);
    j = 0;
    while (num[j])
        out[i++] = num[j++];
    j = 0;
    while (suffix[j])
        out[i++] = suffix[j++];
    out[i] = '\0';
}

/* Lives top-left, the timer the arcade brief actually asked for top
** right -- drawn every frame regardless of game_over, so the last
** thing on screen before a game over is the true final time, not a
** frozen guess. */
void    render_hud(SDL_Renderer *ren, TTF_Font *font, int lives,
        int elapsed_seconds)
{
    char    lives_str[32];
    char    time_str[32];

    format_hud_line(lives_str, "LIVES: ", lives, "");
    format_hud_line(time_str, "TIME: ", elapsed_seconds, "s");
    draw_string(ren, font, 16, 12, lives_str);
    draw_string(ren, font, WINDOW_W - 160, 12, time_str);
}

/* Drawn on top of the frozen world, not instead of it -- the same
** arcade convention as a cabinet holding the last frame on screen
** while it waits for another coin. */
void    render_game_over(SDL_Renderer *ren, TTF_Font *font,
        int elapsed_seconds)
{
    char    survived[32];

    format_hud_line(survived, "SURVIVED ", elapsed_seconds, "s");
    draw_string(ren, font, WINDOW_W / 2 - 90, WINDOW_H / 2 - 40, "GAME OVER");
    draw_string(ren, font, WINDOW_W / 2 - 90, WINDOW_H / 2, survived);
    draw_string(ren, font, WINDOW_W / 2 - 150, WINDOW_H / 2 + 40,
        "PRESS SPACE FOR ANOTHER COIN");
}
