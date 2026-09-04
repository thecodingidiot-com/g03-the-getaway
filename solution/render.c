#include <stdlib.h>
#include "render.h"
#include "scaler.h"

typedef struct s_draw_item
{
    t_projection    proj;
    int             sprite_id;
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
** collision. Scattered wider than any obstacle in road1.txt so they
** read as background, not as something to dodge. */
static t_vec2 const g_markers[] = {
    {10.0f, 8.0f}, {10.0f, -9.0f}, {22.0f, 10.0f}, {22.0f, -7.0f},
    {35.0f, 9.0f}, {35.0f, -11.0f}, {48.0f, 7.0f}, {48.0f, -9.0f},
    {58.0f, 11.0f}, {58.0f, -8.0f}, {68.0f, 9.0f}, {68.0f, -10.0f},
    {78.0f, 8.0f}, {78.0f, -9.0f},
};
# define MARKER_COUNT (int)(sizeof(g_markers) / sizeof(g_markers[0]))

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
    SDL_SetRenderDrawColor(ren, 0x5c, 0x9d, 0xe8, 0xff);
    SDL_RenderFillRect(ren, &sky);
    SDL_SetRenderDrawColor(ren, 0x4a, 0x8a, 0x4a, 0xff);
    SDL_RenderFillRect(ren, &ground);
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

void    render_road(t_road const *road, t_camera const *cam,
        float cam_height, SDL_Renderer *ren,
        SDL_Texture *sprites[SPRITE_COUNT])
{
    t_draw_item items[MAX_OBSTACLES];
    int         visible;
    int         i;
    SDL_Rect    dst;

    visible = 0;
    i = 0;
    while (i < road->count) {
        items[visible].proj = scaler_project(cam, road->obstacles[i].pos);
        items[visible].sprite_id = road->obstacles[i].sprite_id;
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
        dst.x = items[i].proj.screen_x;
        dst.y = items[i].proj.screen_y;
        dst.w = items[i].proj.size;
        dst.h = items[i].proj.size;
        SDL_RenderCopy(ren, sprites[items[i].sprite_id], NULL, &dst);
        i++;
    }
}

/* Same scaler_project() pipeline r01 built, applied to fixed decoration
** instead of gameplay obstacles -- shrunk after projecting, not a
** second technique. Drawn before render_road() calls, so a real
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

/* No projection at all -- the player's own craft sits at a fixed
** horizontal point on screen every frame, the same way the dashboard
** in an Out Run cabinet never moves while the road does. Altitude is
** the one exception: it moves the craft itself, in real pixels, not
** anything projected -- Space Harrier's own on-screen character does
** exactly this, no depth involved at all. */
void    render_player(SDL_Renderer *ren, SDL_Texture *player_tex,
        float cam_height)
{
    SDL_Rect    dst;

    dst.w = PLAYER_SPRITE_W;
    dst.h = PLAYER_SPRITE_H;
    dst.x = WINDOW_W / 2 - dst.w / 2;
    dst.y = WINDOW_H - dst.h - 24 - (int)(cam_height * PLAYER_PX_PER_UNIT);
    SDL_RenderCopy(ren, player_tex, NULL, &dst);
}
