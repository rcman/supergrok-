#include "bullet.h"
#include <SDL2/SDL_image.h>

Bullet::Bullet(SDL_Renderer* renderer, float x, float y, bool is_player) : x(x), y(y) {
    SDL_Surface* surface = IMG_Load("bullet.png");
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    speed = is_player ? -500.0f : 300.0f; // Up for player, down for enemy
}

void Bullet::update(float delta_time) {
    y += speed * delta_time;
    if (y < -h || y > 600) active = false;
}

void Bullet::render(SDL_Renderer* renderer) {
    if (active) {
        SDL_Rect rect = {(int)x, (int)y, w, h};
        SDL_RenderCopy(renderer, texture, nullptr, &rect);
    }
}

BulletManager::BulletManager(SDL_Renderer* renderer) : renderer(renderer) {}

void BulletManager::spawn_bullet(float x, float y, bool is_player) {
    bullets.emplace_back(renderer, x, y, is_player);
}

void BulletManager::update(float delta_time) {
    for (auto it = bullets.begin(); it != bullets.end();) {
        it->update(delta_time);
        if (!it->active) it = bullets.erase(it);
        else ++it;
    }
}

void BulletManager::render(SDL_Renderer* renderer) {
    for (auto& bullet : bullets) bullet.render(renderer);
}

void BulletManager::check_collision(const SDL_Rect& target, std::function<void(Bullet&)> on_hit) {
    for (auto& bullet : bullets) {
        if (!bullet.active) continue;
        SDL_Rect bullet_rect = bullet.get_rect();
        if (SDL_HasIntersection(&bullet_rect, &target)) on_hit(bullet);
    }
}
