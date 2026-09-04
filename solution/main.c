#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "libtci.h"
#include "camera.h"
#include "scaler.h"
#include "road.h"
#include "render.h"

#define TURN_SPEED  0.035f
#define MAX_SPEED   1.1f
#define MIN_SPEED   -0.4f
#define ACCEL       0.025f
#define BRAKE       0.05f
#define DECEL       0.015f

/* Same steering convention r01/r02 established, unchanged: RIGHT/L is
** the negative angle delta, LEFT/H the positive one. Forward/back are
** no longer a fixed speed -- holding a direction ramps toward it,
** letting go decays back toward a stop instead of snapping there. */
static void handle_input(t_camera *cam, float *speed, Uint8 const *keys)
{
    if (keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_H])
        camera_turn(cam, TURN_SPEED);
    if (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_L])
        camera_turn(cam, -TURN_SPEED);
    if (keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_K]) {
        *speed += ACCEL;
        if (*speed > MAX_SPEED)
            *speed = MAX_SPEED;
    } else if (keys[SDL_SCANCODE_DOWN] || keys[SDL_SCANCODE_J]) {
        *speed -= BRAKE;
        if (*speed < MIN_SPEED)
            *speed = MIN_SPEED;
    } else if (*speed > 0.0f) {
        *speed -= DECEL;
        if (*speed < 0.0f)
            *speed = 0.0f;
    } else if (*speed < 0.0f) {
        *speed += DECEL;
        if (*speed > 0.0f)
            *speed = 0.0f;
    }
    camera_move(cam, *speed);
}

static void handle_terminal_event(t_camera *cam, float *speed,
        t_event event, int *runs)
{
    if (event == EVENT_NONE)
        return ;
    if (event == EVENT_DIED)
        tci_printf("crashed -- run %d over, restarting\n", *runs);
    else
        tci_printf("made it -- run %d won, restarting\n", *runs);
    *runs = *runs + 1;
    camera_init(cam, 0.0f, 0.0f, 0.0f);
    *speed = 0.0f;
}

int main(int argc, char **argv)
{
    SDL_Window      *win;
    SDL_Renderer    *ren;
    SDL_Event       ev;
    SDL_Texture     *sprites[SPRITE_COUNT];
    SDL_Texture     *marker_tex;
    SDL_Texture     *player_tex;
    t_road          road;
    t_camera        cam;
    Uint8 const     *keys;
    int             running;
    int             runs;
    float           speed;

    if (argc < 2) {
        tci_printf("usage: %s <road_file>\n", argv[0]);
        return (1);
    }
    if (!road_load(&road, argv[1])) {
        tci_printf("failed to load road: %s\n", argv[1]);
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
    if (!sprites[0] || !sprites[1] || !marker_tex || !player_tex) {
        tci_printf("failed to load a sprite: %s\n", IMG_GetError());
        tci_printf("did you run 'bash gen_assets.sh' first?\n");
        return (1);
    }
    camera_init(&cam, 0.0f, 0.0f, 0.0f);
    speed = 0.0f;
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
        handle_input(&cam, &speed, keys);
        handle_terminal_event(&cam, &speed,
            road_check_collision(&road, &cam), &runs);
        handle_terminal_event(&cam, &speed,
            road_check_finish(&road, &cam), &runs);
        render_backdrop(ren);
        render_markers(&cam, ren, marker_tex);
        render_road(&road, &cam, ren, sprites);
        render_player(ren, player_tex);
        SDL_RenderPresent(ren);
        SDL_Delay(16);
    }
    SDL_DestroyTexture(sprites[0]);
    SDL_DestroyTexture(sprites[1]);
    SDL_DestroyTexture(marker_tex);
    SDL_DestroyTexture(player_tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    IMG_Quit();
    SDL_Quit();
    return (0);
}
