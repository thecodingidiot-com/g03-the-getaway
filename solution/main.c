#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "libtci.h"
#include "camera.h"
#include "scaler.h"
#include "shot.h"
#include "map.h"
#include "render.h"
#include "audio.h"

#define FORWARD_SPEED   0.6f
#define STRAFE_SPEED    0.3f
#define VERTICAL_SPEED  0.07f
#define MAX_TILT_DEG    25.0f
#define TILT_EASE       0.2f

/* Hang-On/Out Run/Space Harrier never rotate the camera -- steering
** shifts world position directly, so the ground slides sideways under
** a craft that keeps facing forward. The first version of this file
** called camera_turn() for left/right, which is the raycaster's
** steering model (r03 rotates and moves forward), not this one --
** caught by actually playing it, not by reading the code. `cam->right`
** never changes once `camera_turn()` is never called, so this is a
** plain vector add, not a rotation.
**
** MIN_SIDE/MAX_SIDE fence the play area, the same way the real game
** does -- clamped directly on `cam->pos.y`, which is safe here only
** because `forward` never rotates away from (1, 0): `y` genuinely is
** the whole lateral coordinate, not an approximation of it.
**
** `*tilt` is purely cosmetic on top of that -- it never touches
** `cam->pos`, only how the player's own sprite gets drawn. It eases
** toward a target instead of snapping to it, the same reason a real
** plane's bank angle lags the stick instead of matching it instantly. */
static void handle_steering(t_camera *cam, float *tilt, Uint8 const *keys)
{
    float   target;

    target = 0.0f;
    if (keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_H]) {
        cam->pos = vec2_add(cam->pos, vec2_scale(cam->right, -STRAFE_SPEED));
        target = MAX_TILT_DEG;
    }
    if (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_L]) {
        cam->pos = vec2_add(cam->pos, vec2_scale(cam->right, STRAFE_SPEED));
        target = -MAX_TILT_DEG;
    }
    if (cam->pos.y > MAX_SIDE)
        cam->pos.y = MAX_SIDE;
    if (cam->pos.y < MIN_SIDE)
        cam->pos.y = MIN_SIDE;
    *tilt += (target - *tilt) * TILT_EASE;
}

/* Space Harrier's other axis -- up/down never touched the camera at
** all before this. A direct step, same as steering, clamped to a
** fixed band rather than ramped: this is positioning, not throttle. */
static void handle_altitude(float *cam_height, Uint8 const *keys)
{
    if (keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_K]) {
        *cam_height += VERTICAL_SPEED;
        if (*cam_height > MAX_HEIGHT)
            *cam_height = MAX_HEIGHT;
    }
    if (keys[SDL_SCANCODE_DOWN] || keys[SDL_SCANCODE_J]) {
        *cam_height -= VERTICAL_SPEED;
        if (*cam_height < MIN_HEIGHT)
            *cam_height = MIN_HEIGHT;
    }
}

/* Ctrl or D fires, on its own cooldown -- not tied to the frame rate
** the way a single SDL_KEYDOWN event would be, so holding it down
** fires repeatedly at a fixed pace instead of once. D matches the
** real Space Harrier emulator default; Ctrl stays as the more common
** PC-game fire key. shot_fire() itself doesn't know or care what's
** about to fire on it; it just claims the first free slot in the
** pool. Returning EVENT_FIRED only on an actual fire -- not on every
** frame the key is held -- is what lets audio_play_sfx() below treat
** it exactly like map_check_collision()/map_check_finish() already
** are: a t_event, checked once, played once. */
static t_event handle_fire(t_shot shots[MAX_SHOTS], int *cooldown,
        t_camera const *cam, float cam_height, Uint8 const *keys)
{
    if (*cooldown > 0)
        *cooldown = *cooldown - 1;
    if ((keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL]
            || keys[SDL_SCANCODE_D]) && *cooldown == 0) {
        shot_fire(shots, cam->pos, cam_height);
        *cooldown = FIRE_COOLDOWN;
        return (EVENT_FIRED);
    }
    return (EVENT_NONE);
}

static void handle_terminal_event(t_camera *cam, float *cam_height,
        float *tilt, t_map *map, t_shot shots[MAX_SHOTS], t_audio const *audio,
        t_event event, int *runs)
{
    int i;

    audio_play_sfx(audio, event);
    if (event == EVENT_NONE)
        return ;
    if (event == EVENT_DIED)
        tci_printf("crashed -- run %d over, restarting\n", *runs);
    else
        tci_printf("made it -- run %d won, restarting\n", *runs);
    *runs = *runs + 1;
    camera_init(cam, 0.0f, 0.0f, 0.0f);
    *cam_height = 0.0f;
    *tilt = 0.0f;
    map_reset(map);
    i = 0;
    while (i < MAX_SHOTS) {
        shots[i].active = 0;
        i++;
    }
}

int main(int argc, char **argv)
{
    SDL_Window      *win;
    SDL_Renderer    *ren;
    SDL_Event       ev;
    SDL_Texture     *sprites[SPRITE_COUNT];
    SDL_Texture     *marker_tex;
    SDL_Texture     *player_tex;
    SDL_Texture     *shot_tex;
    t_map           map;
    t_camera        cam;
    t_shot          shots[MAX_SHOTS];
    t_audio         audio;
    Uint8 const     *keys;
    int             running;
    int             runs;
    int             fire_cooldown;
    int             i;
    float           cam_height;
    float           tilt;

    if (argc < 2) {
        tci_printf("usage: %s <map_file>\n", argv[0]);
        return (1);
    }
    if (!map_load(&map, argv[1])) {
        tci_printf("failed to load map: %s\n", argv[1]);
        return (1);
    }
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("SDL_Init: %s", SDL_GetError());
        return (1);
    }
    win = SDL_CreateWindow("g03", SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED, WINDOW_W, WINDOW_H, 0);
    ren = SDL_CreateRenderer(win, -1, 0);
    IMG_Init(IMG_INIT_PNG);
    sprites[0] = IMG_LoadTexture(ren, "assets/rock.png");
    sprites[1] = IMG_LoadTexture(ren, "assets/pillar.png");
    marker_tex = IMG_LoadTexture(ren, "assets/marker.png");
    player_tex = IMG_LoadTexture(ren, "assets/player.png");
    shot_tex = IMG_LoadTexture(ren, "assets/shot.png");
    if (!sprites[0] || !sprites[1] || !marker_tex || !player_tex
            || !shot_tex) {
        tci_printf("failed to load a sprite: %s\n", IMG_GetError());
        tci_printf("did you run 'bash gen_assets.sh' first?\n");
        return (1);
    }
    if (!audio_init(&audio)) {
        tci_printf("audio_init: %s\n", Mix_GetError());
        tci_printf("did you run 'bash gen_audio.sh' first?\n");
        return (1);
    }
    camera_init(&cam, 0.0f, 0.0f, 0.0f);
    cam_height = 0.0f;
    tilt = 0.0f;
    fire_cooldown = 0;
    i = 0;
    while (i < MAX_SHOTS) {
        shots[i].active = 0;
        i++;
    }
    runs = 1;
    running = 1;
    while (running) {
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT)
                running = 0;
            if (ev.type == SDL_KEYDOWN && (ev.key.keysym.sym == SDLK_ESCAPE
                    || ev.key.keysym.sym == SDLK_q))
                running = 0;
        }
        keys = SDL_GetKeyboardState(NULL);
        handle_steering(&cam, &tilt, keys);
        handle_altitude(&cam_height, keys);
        audio_play_sfx(&audio,
            handle_fire(shots, &fire_cooldown, &cam, cam_height, keys));
        camera_move(&cam, FORWARD_SPEED);
        shot_update(shots);
        map_check_shots(&map, shots);
        handle_terminal_event(&cam, &cam_height, &tilt, &map, shots, &audio,
            map_check_collision(&map, &cam, cam_height), &runs);
        handle_terminal_event(&cam, &cam_height, &tilt, &map, shots, &audio,
            map_check_finish(&map, &cam), &runs);
        render_backdrop(ren);
        render_markers(&cam, cam_height, ren, marker_tex);
        render_map(&map, &cam, cam_height, ren, sprites);
        render_shots(shots, &cam, ren, shot_tex);
        render_player(ren, player_tex, cam_height, tilt);
        SDL_RenderPresent(ren);
        SDL_Delay(16);
    }
    audio_free(&audio);
    SDL_DestroyTexture(sprites[0]);
    SDL_DestroyTexture(sprites[1]);
    SDL_DestroyTexture(marker_tex);
    SDL_DestroyTexture(player_tex);
    SDL_DestroyTexture(shot_tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    IMG_Quit();
    SDL_Quit();
    return (0);
}
