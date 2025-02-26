#ifndef BULLET_H
#define BULLET_H

#include <SDL2/SDL.h>
#include <vector>
#include <functional>

class Bullet {
public:
    Bullet(SDL_Renderer* renderer, float x, float y, bool is_player);
    void update(float delta_time);
    void render(SDL_Renderer* renderer);
    SDL_Rect get_rect() const { return {(int)x, (int)y, w, h}; }
    bool active = true;
private:
    float x, y;
    float speed;
    int w = 8, h = 16;
    SDL_Texture* texture;
};

class BulletManager {
public:
    BulletManager(SDL_Renderer* renderer);
    void spawn_bullet(float x, float y, bool is_player);
    void update(float delta_time);
    void render(SDL_Renderer* renderer);
    void check_collision(const SDL_Rect& target, std::function<void(Bullet&)> on_hit);
private:
    std::vector<Bullet> bullets;
    SDL_Renderer* renderer;
};

#endif
