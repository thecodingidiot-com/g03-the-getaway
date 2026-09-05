#include <math.h>
#include <stdlib.h>
#include "render.h"
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

/* One row of the chequerboard, top to bottom edge to edge -- no gaps
** either side of a cell, unlike a thin stripe drawn at one fixed
** thickness. `size` sets both how wide each cell is here (bigger,
** the same WINDOW_H / depth growth every billboard already gets
** scaled by) and how many are needed to reach the screen edge: few,
** wide cells up close, many, thin ones near the horizon, with no
** separate case for either -- the same division just keeps dividing.
** `row_parity` alternates which colour starts each row, so columns
** and rows both check like an actual board, not only one axis. */
static void render_checker_row(SDL_Renderer *ren, int top, int bottom,
        float size, int row_parity)
{
    SDL_Rect    cell;
    int         k;
    int         x0;
    int         x1;

    if (bottom <= top || size <= 0.0f)
        return ;
    cell.y = top;
    cell.h = bottom - top;
    k = -CHECKER_MAX_COLUMNS;
    while (k < CHECKER_MAX_COLUMNS) {
        x0 = WINDOW_W / 2 + (int)(size * (float)k * CHECKER_CELL_S);
        if (x0 >= WINDOW_W)
            break;
        x1 = WINDOW_W / 2 + (int)(size * (float)(k + 1) * CHECKER_CELL_S);
        if (x1 > 0) {
            cell.x = (x0 < 0) ? 0 : x0;
            cell.w = ((x1 > WINDOW_W) ? WINDOW_W : x1) - cell.x;
            if (cell.w > 0) {
                if ((row_parity + k) & 1)
                    SDL_SetRenderDrawColor(ren, 0x1c, 0x15, 0x26, 0xff);
                else
                    SDL_SetRenderDrawColor(ren, 0x7c, 0x3a, 0xed, 0xff);
                SDL_RenderFillRect(ren, &cell);
            }
        }
        k++;
    }
}

/* Space Harrier's own real floor is a moving scanline surface -- a
** different, larger technique this chapter still doesn't attempt.
** This reuses the one already here instead: scaler_project()'s own
** `size` field, the same WINDOW_H / depth growth every billboard is
** scaled by, applied downward from HORIZON_Y instead of upward --
** ground recedes into the horizon exactly as fast as a billboard at
** the same depth grows, because it's the same division.
**
** STRIPE_SPACING world units apart, STRIPE_COUNT of them, each one's
** depth wrapping through cam->pos.x with fmodf() -- the same "how far
** through the current cycle" idea a clock face uses, so a row slides
** continuously toward the camera and off the near plane instead of
** jumping there. The next one behind it is already sliding in from
** the horizon; nothing is ever reset or respawned.
**
** Rows are walked farthest first specifically so each one's own y can
** become the *top* of the next, nearer row's band -- a chequerboard
** tiles edge to edge with no gap between rows, unlike a thin stripe
** drawn at one fixed thickness regardless of how far the next row is.
**
** CHECKER_MAX_SIZE caps `size` the same way cap_projection() already
** caps a billboard's -- without it, the nearest row's cells would
** grow without bound and swallow the whole screen in one or two giant
** squares instead of reading as perspective at all. The real cabinet
** doesn't chequer all the way to the bottom edge either: once the cap
** stops the pattern advancing, whatever's left between there and
** WINDOW_H is one plain fill, not more (unreadably huge) cells. */
void    render_ground_stripes(t_camera const *cam, SDL_Renderer *ren)
{
    t_projection    proj;
    t_vec2          far_point;
    SDL_Rect        solid;
    float           size;
    int             prev_y;
    int             row_y;
    int             i;

    prev_y = HORIZON_Y;
    i = STRIPE_COUNT - 1;
    while (i >= 0) {
        far_point.x = cam->pos.x + STRIPE_SPACING
            - fmodf(cam->pos.x, STRIPE_SPACING) + (float)i * STRIPE_SPACING;
        far_point.y = cam->pos.y;
        proj = scaler_project(cam, far_point);
        if (proj.visible) {
            size = (float)proj.size;
            if (size > CHECKER_MAX_SIZE)
                size = CHECKER_MAX_SIZE;
            row_y = HORIZON_Y + (int)(size * STRIPE_Y_SCALE);
            if (row_y > WINDOW_H)
                row_y = WINDOW_H;
            render_checker_row(ren, prev_y, row_y, size, i);
            prev_y = row_y;
        }
        i--;
    }
    if (prev_y < WINDOW_H) {
        solid.x = 0;
        solid.y = prev_y;
        solid.w = WINDOW_W;
        solid.h = WINDOW_H - prev_y;
        SDL_SetRenderDrawColor(ren, 0x7c, 0x3a, 0xed, 0xff);
        SDL_RenderFillRect(ren, &solid);
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
        proj->screen_y = HORIZON_Y - proj->size;
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
        SDL_Renderer *ren, SDL_Texture *shot_tex)
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
void    render_player(SDL_Renderer *ren, SDL_Texture *player_tex,
        float cam_height, float tilt)
{
    SDL_Rect    dst;

    dst.w = PLAYER_SPRITE_W;
    dst.h = PLAYER_SPRITE_H;
    dst.x = WINDOW_W / 2 - dst.w / 2;
    dst.y = WINDOW_H - dst.h - 24 - (int)(cam_height * PLAYER_PX_PER_UNIT);
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
