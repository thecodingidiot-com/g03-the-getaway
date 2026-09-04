#ifndef AUDIO_H
# define AUDIO_H

# include <SDL2/SDL_mixer.h>
# include "event.h"

typedef struct s_audio
{
    Mix_Chunk   *sfx_shoot;
    Mix_Chunk   *sfx_hit;
    Mix_Chunk   *sfx_crash;
}   t_audio;

int     audio_init(t_audio *audio);
void    audio_play_sfx(t_audio const *audio, t_event event);
void    audio_free(t_audio *audio);

#endif
