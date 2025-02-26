#ifndef ENEMY_H
#define ENEMY_H

#include <SDL2/SDL.h>
#include <vector>
#include "bullet.h"

class Enemy {
public:
    Enemy(SDL_Renderer* renderer, float x, float y);
    void update(float delta_time);
    void render(SDL_Renderer* renderer);
    SDL_Rect get_rect() const { return {(int)x, (int)y, w, h}; }
    bool active = true;
private:
    float x, y;
    float speed = 100.0f;
    int w = 32, h = 32;
    SDL_Texture* texture;
};

class EnemyManager {
public:
    EnemyManager(SDL_Renderer* renderer);
    void update(float delta_time, BulletManager& bullet_mgr);
    void render(SDL_Renderer* renderer);
    void check_collisions(BulletManager& bullet_mgr);
private:
    std::vector<Enemy> enemies;
    SDL_Renderer* renderer;
    Uint32 last_spawn_time = 0;
};

#endif
