#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>
#include <stdbool.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define PLAYER_SPEED 5
#define BULLET_SPEED 10
#define MAX_BULLETS 50
#define MAX_ENEMIES 20
#define SCROLL_SPEED 2

// Player structure
typedef struct {
    float x, y;
    int w, h;
    float dx, dy; // Velocity
} Player;

// Bullet structure
typedef struct {
    float x, y;
    bool active;
} Bullet;

// Enemy structure
typedef struct {
    float x, y;
    int w, h;
    bool active;
} Enemy;

int main(int argc, char* args[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }
    if (IMG_Init(IMG_INIT_PNG) == 0) {
        printf("SDL_image could not initialize! IMG_Error: %s\n", IMG_GetError());
        return 1;
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("SDL_mixer could not initialize! Mix_Error: %s\n", Mix_GetError());
        return 1;
    }

    // Create window and renderer
    SDL_Window* window = SDL_CreateWindow("Seek & Destroy Clone", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!window || !renderer) {
        printf("Window or renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    // Load assets (replace with your file paths)
    SDL_Texture* playerTex = IMG_LoadTexture(renderer, "helicopter.png");
    SDL_Texture* bulletTex = IMG_LoadTexture(renderer, "bullet.png");
    SDL_Texture* enemyTex = IMG_LoadTexture(renderer, "enemy.png");
    SDL_Texture* bgTex = IMG_LoadTexture(renderer, "background.png");
    Mix_Chunk* shootSound = Mix_LoadWAV("shoot.wav");

    // Player setup
    Player player = {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 64, 64, 0, 0};
    Bullet bullets[MAX_BULLETS] = {0};
    Enemy enemies[MAX_ENEMIES] = {0};
    float bgScrollX = 0, bgScrollY = 0;

    // Spawn some enemies
    for (int i = 0; i < 5; i++) {
        enemies[i].x = rand() % SCREEN_WIDTH;
        enemies[i].y = rand() % (SCREEN_HEIGHT / 2);
        enemies[i].w = 48;
        enemies[i].h = 48;
        enemies[i].active = true;
    }

    bool quit = false;
    SDL_Event e;

    while (!quit) {
        // Handle input
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) quit = true;
            if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_UP: player.dy = -PLAYER_SPEED; break;
                    case SDLK_DOWN: player.dy = PLAYER_SPEED; break;
                    case SDLK_LEFT: player.dx = -PLAYER_SPEED; break;
                    case SDLK_RIGHT: player.dx = PLAYER_SPEED; break;
                    case SDLK_SPACE:
                        for (int i = 0; i < MAX_BULLETS; i++) {
                            if (!bullets[i].active) {
                                bullets[i].x = player.x + player.w;
                                bullets[i].y = player.y + player.h / 2;
                                bullets[i].active = true;
                                Mix_PlayChannel(-1, shootSound, 0);
                                break;
                            }
                        }
                        break;
                }
            }
            if (e.type == SDL_KEYUP) {
                if (e.key.keysym.sym == SDLK_UP || e.key.keysym.sym == SDLK_DOWN) player.dy = 0;
                if (e.key.keysym.sym == SDLK_LEFT || e.key.keysym.sym == SDLK_RIGHT) player.dx = 0;
            }
        }

        // Update player position
        player.x += player.dx;
        player.y += player.dy;
        if (player.x < 0) player.x = 0;
        if (player.x + player.w > SCREEN_WIDTH) player.x = SCREEN_WIDTH - player.w;
        if (player.y < 0) player.y = 0;
        if (player.y + player.h > SCREEN_HEIGHT) player.y = SCREEN_HEIGHT - player.h;

        // Update bullets
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                bullets[i].x += BULLET_SPEED;
                if (bullets[i].x > SCREEN_WIDTH) bullets[i].active = false;
            }
        }

        // Update enemies (simple movement)
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (enemies[i].active) {
                enemies[i].x -= 2; // Move left
                if (enemies[i].x < -enemies[i].w) enemies[i].active = false; // Despawn
            }
        }

        // Collision detection (bullets vs enemies)
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (!bullets[i].active) continue;
            for (int j = 0; j < MAX_ENEMIES; j++) {
                if (!enemies[j].active) continue;
                SDL_Rect bulletRect = {(int)bullets[i].x, (int)bullets[i].y, 16, 8};
                SDL_Rect enemyRect = {(int)enemies[j].x, (int)enemies[j].y, enemies[j].w, enemies[j].h};
                if (SDL_HasIntersection(&bulletRect, &enemyRect)) {
                    bullets[i].active = false;
                    enemies[j].active = false;
                }
            }
        }

        // Scrolling background
        bgScrollX -= SCROLL_SPEED;
        if (bgScrollX <= -SCREEN_WIDTH) bgScrollX = 0;

        // Render
        SDL_RenderClear(renderer);

        // Draw background (tiled scrolling)
        SDL_Rect bgRect1 = {(int)bgScrollX, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_Rect bgRect2 = {(int)bgScrollX + SCREEN_WIDTH, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_RenderCopy(renderer, bgTex, NULL, &bgRect1);
        SDL_RenderCopy(renderer, bgTex, NULL, &bgRect2);

        // Draw player
        SDL_Rect playerRect = {(int)player.x, (int)player.y, player.w, player.h};
        SDL_RenderCopy(renderer, playerTex, NULL, &playerRect);

        // Draw bullets
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                SDL_Rect bulletRect = {(int)bullets[i].x, (int)bullets[i].y, 16, 8};
                SDL_RenderCopy(renderer, bulletTex, NULL, &bulletRect);
            }
        }

        // Draw enemies
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (enemies[i].active) {
                SDL_Rect enemyRect = {(int)enemies[i].x, (int)enemies[i].y, enemies[i].w, enemies[i].h};
                SDL_RenderCopy(renderer, enemyTex, NULL, &enemyRect);
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60 FPS
    }

    // Cleanup
    SDL_DestroyTexture(playerTex);
    SDL_DestroyTexture(bulletTex);
    SDL_DestroyTexture(enemyTex);
    SDL_DestroyTexture(bgTex);
    Mix_FreeChunk(shootSound);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_Quit();
    IMG_Quit();
    SDL_Quit();

    return 0;
}
