#include "audio.h"

/* Same boundary render.c keeps for SDL2's drawing calls -- audio.c is
** the only file that ever calls an actual SDL2_mixer function.
** Mix_AllocateChannels(8) is what makes burst fire from a held key
** layer instead of cutting itself off: each Mix_PlayChannel(-1, ...)
** below grabs any free channel rather than a fixed one, so a shot
** fired mid-decay of the last one gets its own channel instead of
** stopping it short. Keeping sfx_shoot itself short (well under
** FIRE_COOLDOWN's gap between shots) is what keeps a held-down burst
** sounding like a burst instead of a wash of overlapping tails. */
int audio_init(t_audio *audio)
{
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) != 0)
        return (0);
    Mix_AllocateChannels(8);
    audio->sfx_shoot = Mix_LoadWAV("assets/sfx_shoot.wav");
    audio->sfx_hit = Mix_LoadWAV("assets/sfx_hit.wav");
    audio->sfx_crash = Mix_LoadWAV("assets/sfx_crash.wav");
    if (!audio->sfx_shoot || !audio->sfx_hit || !audio->sfx_crash)
        return (0);
    return (1);
}

void audio_play_sfx(t_audio const *audio, t_event event)
{
    if (event == EVENT_FIRED)
        Mix_PlayChannel(-1, audio->sfx_shoot, 0);
    else if (event == EVENT_HIT)
        Mix_PlayChannel(-1, audio->sfx_hit, 0);
    else if (event == EVENT_DIED)
        Mix_PlayChannel(-1, audio->sfx_crash, 0);
}

void audio_free(t_audio *audio)
{
    Mix_FreeChunk(audio->sfx_shoot);
    Mix_FreeChunk(audio->sfx_hit);
    Mix_FreeChunk(audio->sfx_crash);
    Mix_CloseAudio();
}
