#include "level.h"
#include <SDL2/SDL_image.h>

Level::Level(SDL_Renderer* renderer) {
    SDL_Surface* bg1_surface = IMG_Load("bg1.png"); // Far background
    bg1_texture = SDL_CreateTextureFromSurface(renderer, bg1_surface);
    SDL_Surface* bg2_surface = IMG_Load("bg2.png"); // Near background
    bg2_texture = SDL_CreateTextureFromSurface(renderer, bg2_surface);
    SDL_FreeSurface(bg1_surface);
    SDL_FreeSurface(bg2_surface);
}

void Level::update(float delta_time) {
    bg1_y += bg1_speed * delta_time;
    bg2_y += bg2_speed * delta_time;
    if (bg1_y >= 600) bg1_y -= 600;
    if (bg2_y >= 600) bg2_y -= 600;
}

void Level::render(SDL_Renderer* renderer) {
    SDL_Rect bg1_rect = {0, (int)bg1_y - 600, 800, 600};
    SDL_Rect bg1_rect2 = {0, (int)bg1_y, 800, 600};
    SDL_Rect bg2_rect = {0, (int)bg2_y - 600, 800, 600};
    SDL_Rect bg2_rect2 = {0, (int)bg2_y, 800, 600};
    SDL_RenderCopy(renderer, bg1_texture, nullptr, &bg1_rect);
    SDL_RenderCopy(renderer, bg1_texture, nullptr, &bg1_rect2);
    SDL_RenderCopy(renderer, bg2_texture, nullptr, &bg2_rect);
    SDL_RenderCopy(renderer, bg2_texture, nullptr, &bg2_rect2);
}
