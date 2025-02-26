#ifndef LEVEL_H
#define LEVEL_H

#include <SDL2/SDL.h>

class Level {
public:
    Level(SDL_Renderer* renderer);
    void update(float delta_time);
    void render(SDL_Renderer* renderer);
private:
    SDL_Texture* bg1_texture; // Far layer
    SDL_Texture* bg2_texture; // Near layer
    float bg1_y = 0, bg2_y = 0;
    float bg1_speed = 50.0f, bg2_speed = 100.0f;
};

#endif
