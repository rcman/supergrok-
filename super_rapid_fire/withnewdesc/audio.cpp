#include "audio.h"

Audio::Audio() {
    shoot_sound = Mix_LoadWAV("shoot.wav");
    bg_music = Mix_LoadMUS("bgm.mp3");
    Mix_PlayMusic(bg_music, -1); // Loop BGM
}

Audio::~Audio() {
    Mix_FreeChunk(shoot_sound);
    Mix_FreeMusic(bg_music);
}

void Audio::play_shoot() {
    Mix_PlayChannel(-1, shoot_sound, 0);
}
