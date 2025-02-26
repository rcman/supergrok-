#ifndef PLAYER_H
#define PLAYER_H

#include <SDL2/SDL.h>
#include <vector>
#include "bullet.h"

class Player {
public:
    Player(SDL_Renderer* renderer, float x, float y);
    void handle_input(SDL_Event& event);
    void update(float delta_time, BulletManager& bullet_mgr);
    void render(SDL_Renderer* renderer);
private:
    float x, y;
    float speed = 300.0f;
    int w = 32, h = 32;
    SDL_Texture* texture;
    Uint32 last_shot_time = 0;
    const Uint32 shoot_cooldown = 150; // ms
};

#endif
