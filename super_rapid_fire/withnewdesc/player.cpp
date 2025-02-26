#include "player.h"
#include <SDL2/SDL_image.h>

Player::Player(SDL_Renderer* renderer, float x, float y) : x(x), y(y) {
    SDL_Surface* surface = IMG_Load("player.png");
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
}

void Player::handle_input(SDL_Event& event) {
    const Uint8* keys = SDL_GetKeyboardState(NULL);
    if (keys[SDL_SCANCODE_LEFT]) x -= speed * 0.016f; // Assuming ~60 FPS
    if (keys[SDL_SCANCODE_RIGHT]) x += speed * 0.016f;
    if (keys[SDL_SCANCODE_UP]) y -= speed * 0.016f;
    if (keys[SDL_SCANCODE_DOWN]) y += speed * 0.016f;
    x = SDL_clamp(x, 0, SCREEN_WIDTH - w);
    y = SDL_clamp(y, 0, SCREEN_HEIGHT - h);
}

void Player::update(float delta_time, BulletManager& bullet_mgr) {
    Uint32 current_time = SDL_GetTicks();
    const Uint8* keys = SDL_GetKeyboardState(NULL);
    if (keys[SDL_SCANCODE_SPACE] && (current_time - last_shot_time >= shoot_cooldown)) {
        bullet_mgr.spawn_bullet(x + w / 2 - 4, y, true); // Player bullet
        last_shot_time = current_time;
    }
}

void Player::render(SDL_Renderer* renderer) {
    SDL_Rect rect = {(int)x, (int)y, w, h};
    SDL_RenderCopy(renderer, texture, nullptr, &rect);
}
