#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <stdbool.h>
#include <stdio.h>

#define SCREEN_WIDTH 320   // Amiga resolution
#define SCREEN_HEIGHT 200
#define PLAYER_WIDTH 16
#define PLAYER_HEIGHT 16
#define ENEMY_WIDTH 16
#define ENEMY_HEIGHT 16
#define GRAVITY 0.2f
#define JUMP_FORCE -5.0f
#define MOVE_SPEED 2.0f
#define LEVELS 10

typedef struct {
    float x, y;
    float dx, dy;
    int width, height;
    bool isJumping;
    bool active;
} Player;

typedef struct {
    float x, y;
    int width, height;
    bool alive;
} Enemy;

typedef struct {
    int x, y;
    int width, height;
} Platform;

int main(int argc, char* argv[]) {
    // Initialize SDL2
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }

    if (IMG_Init(IMG_INIT_PNG) == 0) {
        printf("IMG initialization failed: %s\n", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("Mixer initialization failed: %s\n", Mix_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Create window and renderer
    SDL_Window* window = SDL_CreateWindow("Lupo Alberto Clone",
                                        SDL_WINDOWPOS_CENTERED,
                                        SDL_WINDOWPOS_CENTERED,
                                        SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2, // Scaled for modern displays
                                        SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        Mix_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        Mix_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);

    // Load textures (placeholders - replace with actual assets)
    SDL_Texture* lupoTex = IMG_LoadTexture(renderer, "lupo.png");     // Lupo Alberto sprite
    SDL_Texture* martaTex = IMG_LoadTexture(renderer, "marta.png");  // Marta sprite
    SDL_Texture* enemyTex = IMG_LoadTexture(renderer, "enemy.png");  // Moses/enemy sprite
    SDL_Texture* bgTex = IMG_LoadTexture(renderer, "farm_bg.png");   // Level background
    SDL_Texture* platformTex = IMG_LoadTexture(renderer, "platform.png");

    // Load sounds (placeholders - replace with actual assets)
    Mix_Chunk* jumpSound = Mix_LoadWAV("jump.wav");
    Mix_Chunk* hitSound = Mix_LoadWAV("hit.wav");
    Mix_Music* bgMusic = Mix_LoadMUS("bg_music.mp3");

    // Player setup (Lupo and Marta)
    Player lupo = {50, SCREEN_HEIGHT - PLAYER_HEIGHT - TILE_SIZE, 0, 0, PLAYER_WIDTH, PLAYER_HEIGHT, false, true};
    Player marta = {100, SCREEN_HEIGHT - PLAYER_HEIGHT - TILE_SIZE, 0, 0, PLAYER_WIDTH, PLAYER_HEIGHT, false, false}; // Inactive in single-player

    // Enemies (demo: 2 enemies)
    Enemy enemies[2] = {
        {200, SCREEN_HEIGHT - ENEMY_HEIGHT - TILE_SIZE, ENEMY_WIDTH, ENEMY_HEIGHT, true},
        {300, SCREEN_HEIGHT - ENEMY_HEIGHT - TILE_SIZE, ENEMY_WIDTH, ENEMY_HEIGHT, true}
    };

    // Platforms (demo: ground and one platform)
    Platform platforms[2] = {
        {0, SCREEN_HEIGHT - TILE_SIZE, SCREEN_WIDTH, TILE_SIZE}, // Ground
        {150, SCREEN_HEIGHT - TILE_SIZE - 50, 100, TILE_SIZE}    // Elevated platform
    };

    int score = 0;
    int lives = 3;
    int currentLevel = 0;
    bool twoPlayer = false; // Toggle for two-player mode
    bool running = true;
    SDL_Event event;

    Mix_PlayMusic(bgMusic, -1); // Loop background music

    while (running) {
        // Event handling
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    // Lupo controls
                    case SDLK_RIGHT: lupo.dx = MOVE_SPEED; break;
                    case SDLK_LEFT: lupo.dx = -MOVE_SPEED; break;
                    case SDLK_UP:
                        if (!lupo.isJumping) {
                            lupo.dy = JUMP_FORCE;
                            lupo.isJumping = true;
                            Mix_PlayChannel(-1, jumpSound, 0);
                        }
                        break;
                    // Marta controls (two-player mode)
                    case SDLK_d: if (twoPlayer) marta.dx = MOVE_SPEED; break;
                    case SDLK_a: if (twoPlayer) marta.dx = -MOVE_SPEED; break;
                    case SDLK_w:
                        if (twoPlayer && !marta.isJumping) {
                            marta.dy = JUMP_FORCE * 1.2f; // Marta jumps higher
                            marta.isJumping = true;
                            Mix_PlayChannel(-1, jumpSound, 0);
                        }
                        break;
                    case SDLK_t: twoPlayer = !twoPlayer; marta.active = twoPlayer; break; // Toggle two-player mode
                }
            }
            if (event.type == SDL_KEYUP) {
                if (event.key.keysym.sym == SDLK_RIGHT || event.key.keysym.sym == SDLK_LEFT) lupo.dx = 0;
                if (twoPlayer && (event.key.keysym.sym == SDLK_d || event.key.keysym.sym == SDLK_a)) marta.dx = 0;
            }
        }

        // Update players
        for (Player* p = &lupo; p <= (twoPlayer ? &marta : &lupo); p++) {
            if (!p->active) continue;
            p->x += p->dx;
            p->y += p->dy;
            p->dy += GRAVITY;

            // Platform collision
            for (int i = 0; i < 2; i++) {
                SDL_Rect playerRect = {(int)p->x, (int)p->y, p->width, p->height};
                SDL_Rect platRect = {platforms[i].x, platforms[i].y, platforms[i].width, platforms[i].height};
                if (SDL_HasIntersection(&playerRect, &platRect) && p->dy > 0) {
                    p->y = platforms[i].y - p->height;
                    p->dy = 0;
                    p->isJumping = false;
                }
            }

            // Boundary checks
            if (p->x < 0) p->x = 0;
            if (p->x + p->width > SCREEN_WIDTH) {
                p->x = 0; // Reset to start of next level
                currentLevel++;
                if (currentLevel >= LEVELS) running = false; // Game won
            }
            if (p->y + p->height > SCREEN_HEIGHT) {
                p->y = SCREEN_HEIGHT - p->height;
                p->dy = 0;
                p->isJumping = false;
            }
        }

        // Update enemies
        for (int i = 0; i < 2; i++) {
            if (enemies[i].alive) {
                enemies[i].x += (i % 2 == 0) ? 1 : -1;
                if (enemies[i].x < 0 || enemies[i].x + enemies[i].width > SCREEN_WIDTH)
                    enemies[i].x = (enemies[i].x < 0) ? 0 : SCREEN_WIDTH - enemies[i].width;

                // Collision with players
                SDL_Rect enemyRect = {(int)enemies[i].x, (int)enemies[i].y, enemies[i].width, enemies[i].height};
                SDL_Rect lupoRect = {(int)lupo.x, (int)lupo.y, lupo.width, lupo.height};
                if (SDL_HasIntersection(&lupoRect, &enemyRect)) {
                    lives--;
                    lupo.x = 50;
                    lupo.y = SCREEN_HEIGHT - PLAYER_HEIGHT - TILE_SIZE;
                    Mix_PlayChannel(-1, hitSound, 0);
                    if (lives <= 0) running = false;
                }
                if (twoPlayer) {
                    SDL_Rect martaRect = {(int)marta.x, (int)marta.y, marta.width, marta.height};
                    if (SDL_HasIntersection(&martaRect, &enemyRect)) {
                        lives--;
                        marta.x = 100;
                        marta.y = SCREEN_HEIGHT - PLAYER_HEIGHT - TILE_SIZE;
                        Mix_PlayChannel(-1, hitSound, 0);
                        if (lives <= 0) running = false;
                    }
                }
            }
        }

        // Rendering
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, bgTex, NULL, NULL); // Background

        // Draw platforms
        for (int i = 0; i < 2; i++) {
            SDL_Rect platRect = {platforms[i].x, platforms[i].y, platforms[i].width, platforms[i].height};
            SDL_RenderCopy(renderer, platformTex, NULL, &platRect);
        }

        // Draw enemies
        for (int i = 0; i < 2; i++) {
            if (enemies[i].alive) {
                SDL_Rect enemyRect = {(int)enemies[i].x, (int)enemies[i].y, enemies[i].width, enemies[i].height};
                SDL_RenderCopy(renderer, enemyTex, NULL, &enemyRect);
            }
        }

        // Draw players
        SDL_Rect lupoRect = {(int)lupo.x, (int)lupo.y, lupo.width, lupo.height};
        SDL_RenderCopy(renderer, lupoTex, NULL, &lupoRect);
        if (twoPlayer) {
            SDL_Rect martaRect = {(int)marta.x, (int)marta.y, marta.width, marta.height};
            SDL_RenderCopy(renderer, martaTex, NULL, &martaRect);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60 FPS
    }

    // Cleanup
    Mix_FreeChunk(jumpSound);
    Mix_FreeChunk(hitSound);
    Mix_FreeMusic(bgMusic);
    SDL_DestroyTexture(lupoTex);
    SDL_DestroyTexture(martaTex);
    SDL_DestroyTexture(enemyTex);
    SDL_DestroyTexture(bgTex);
    SDL_DestroyTexture(platformTex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_Quit();
    IMG_Quit();
    SDL_Quit();

    printf("Game Over! Final Score: %d\n", score);
    return 0;
}
