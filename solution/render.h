#ifndef RENDER_H
# define RENDER_H

# include <SDL2/SDL.h>
# include <SDL2/SDL_ttf.h>
# include "map.h"
# include "camera.h"

# define SPRITE_COUNT       2
# define MAX_SPRITE_SIZE    160
# define MARKER_SCALE       0.3f
# define PLAYER_SPRITE_W    120
# define PLAYER_SPRITE_H    72
# define HEIGHT_SHIFT_SCALE 0.3f
# define PLAYER_PX_PER_UNIT 30
# define SHOT_SCALE         0.2f
# define STRIPE_SPACING     6.0f
# define STRIPE_COUNT       14
# define STRIPE_Y_SCALE     0.5f
# define STRIPE_THICKNESS_SCALE 0.05f
# define STRIPE_MAX_THICKNESS   36
# define CHECKER_HALF_S     3.0f
# define CHECKER_COLUMNS    6

void    render_backdrop(SDL_Renderer *ren);
void    render_ground_stripes(t_camera const *cam, SDL_Renderer *ren);
void    render_map(t_map const *map, t_camera const *cam,
            float cam_height, SDL_Renderer *ren,
            SDL_Texture *sprites[SPRITE_COUNT]);
void    render_markers(t_camera const *cam, float cam_height,
            SDL_Renderer *ren, SDL_Texture *marker_tex);
void    render_shots(t_shot const shots[MAX_SHOTS], t_camera const *cam,
            SDL_Renderer *ren, SDL_Texture *shot_tex);
void    render_player(SDL_Renderer *ren, SDL_Texture *player_tex,
            float cam_height, float tilt);
void    render_hud(SDL_Renderer *ren, TTF_Font *font, int lives,
            int elapsed_seconds);
void    render_game_over(SDL_Renderer *ren, TTF_Font *font,
            int elapsed_seconds);

#endif
