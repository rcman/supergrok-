#ifndef AUDIO_H
#define AUDIO_H

#include <SDL2/SDL_mixer.h>

class Audio {
public:
    Audio();
    ~Audio();
    void play_shoot();
private:
    Mix_Chunk* shoot_sound;
    Mix_Music* bg_music;
};

#endif
