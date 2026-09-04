#ifndef RENDER_H
# define RENDER_H

# include <SDL2/SDL.h>
# include "road.h"
# include "camera.h"

# define SPRITE_COUNT       2
# define MAX_SPRITE_SIZE    160
# define MARKER_SCALE       0.3f
# define PLAYER_SPRITE_W    120
# define PLAYER_SPRITE_H    72
# define HEIGHT_SHIFT_SCALE 0.15f
# define PLAYER_PX_PER_UNIT 30

void    render_backdrop(SDL_Renderer *ren);
void    render_road(t_road const *road, t_camera const *cam,
            float cam_height, SDL_Renderer *ren,
            SDL_Texture *sprites[SPRITE_COUNT]);
void    render_markers(t_camera const *cam, float cam_height,
            SDL_Renderer *ren, SDL_Texture *marker_tex);
void    render_player(SDL_Renderer *ren, SDL_Texture *player_tex,
            float cam_height, float tilt);

#endif
