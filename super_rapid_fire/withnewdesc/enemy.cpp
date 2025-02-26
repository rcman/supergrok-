#include "enemy.h"
#include <SDL2/SDL_image.h>

Enemy::Enemy(SDL_Renderer* renderer, float x, float y) : x(x), y(y) {
    SDL_Surface* surface = IMG_Load("enemy.png");
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
}

void Enemy::update(float delta_time) {
    y += speed * delta_time;
    if (y > 600) active = false; // Off-screen
}

void Enemy::render(SDL_Renderer* renderer) {
    if (active) {
        SDL_Rect rect = {(int)x, (int)y, w, h};
        SDL_RenderCopy(renderer, texture, nullptr, &rect);
    }
}

EnemyManager::EnemyManager(SDL_Renderer* renderer) : renderer(renderer) {}

void EnemyManager::update(float delta_time, BulletManager& bullet_mgr) {
    Uint32 current_time = SDL_GetTicks();
    if (current_time - last_spawn_time > 1000) {
        enemies.emplace_back(renderer, rand() % (800 - 32), -32);
        last_spawn_time = current_time;
    }
    for (auto it = enemies.begin(); it != enemies.end();) {
        it->update(delta_time);
        if (!it->active) it = enemies.erase(it);
        else ++it;
    }
}

void EnemyManager::render(SDL_Renderer* renderer) {
    for (auto& enemy : enemies) enemy.render(renderer);
}

void EnemyManager::check_collisions(BulletManager& bullet_mgr) {
    for (auto& enemy : enemies) {
        if (!enemy.active) continue;
        SDL_Rect enemy_rect = enemy.get_rect();
        bullet_mgr.check_collision(enemy_rect, [&enemy](Bullet& bullet) {
            enemy.active = false;
            bullet.active = false;
        });
    }
}
