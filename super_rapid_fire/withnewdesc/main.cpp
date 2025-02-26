#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include "player.h"
#include "enemy.h"
#include "bullet.h"
#include "level.h"
#include "audio.h"

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0 || IMG_Init(IMG_INIT_PNG) == 0 || 
        Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0 || TTF_Init() < 0) {
        std::cerr << "Initialization failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Super Rapid Fire", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    Player player(renderer, SCREEN_WIDTH / 2 - 16, SCREEN_HEIGHT - 48);
    EnemyManager enemy_mgr(renderer);
    BulletManager bullet_mgr(renderer);
    Level level(renderer);
    Audio audio;

    bool running = true;
    Uint32 last_time = SDL_GetTicks();

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            player.handle_input(event);
        }

        Uint32 current_time = SDL_GetTicks();
        float delta_time = (current_time - last_time) / 1000.0f;
        last_time = current_time;

        player.update(delta_time, bullet_mgr);
        enemy_mgr.update(delta_time, bullet_mgr);
        bullet_mgr.update(delta_time);
        level.update(delta_time);

        // Simple collision check
        enemy_mgr.check_collisions(bullet_mgr);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        level.render(renderer);
        player.render(renderer);
        bullet_mgr.render(renderer);
        enemy_mgr.render(renderer);
        SDL_RenderPresent(renderer);
    }

    Mix_CloseAudio();
    TTF_Quit();
    IMG_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
