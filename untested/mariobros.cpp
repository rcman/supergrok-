#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <stdbool.h>
#include <stdio.h>

#define SCREEN_WIDTH 672   // Original arcade resolution scaled (224x256 -> 672x768)
#define SCREEN_HEIGHT 768
#define PLAYER_WIDTH 32
#define PLAYER_HEIGHT 48
#define ENEMY_WIDTH 32
#define ENEMY_HEIGHT 32
#define COIN_SIZE 16
#define PLATFORM_HEIGHT 16
#define GRAVITY 0.5
#define JUMP_FORCE -12
#define PLAYER_SPEED 4

typedef struct {
    float x, y;
    float dx, dy;
    int width, height;
    bool isJumping;
    bool alive;
} Entity;

typedef struct {
    int x, y;
    int width, height;
    bool active;
} Coin;

typedef struct {
    int x, y;
    int width, height;
} Platform;

int main(int argc, char* argv[]) {
    // Initialize SDL
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
    SDL_Window* window = SDL_CreateWindow("Mario Bros. Clone",
                                        SDL_WINDOWPOS_CENTERED,
                                        SDL_WINDOWPOS_CENTERED,
                                        SCREEN_WIDTH, SCREEN_HEIGHT,
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

    // Load textures (placeholders - replace with actual assets)
    SDL_Texture* marioTex = IMG_LoadTexture(renderer, "mario.png");
    SDL_Texture* enemyTex = IMG_LoadTexture(renderer, "shellcreeper.png");
    SDL_Texture* coinTex = IMG_LoadTexture(renderer, "coin.png");
    SDL_Texture* platformTex = IMG_LoadTexture(renderer, "platform.png");
    SDL_Texture* pipeTex = IMG_LoadTexture(renderer, "pipe.png");
    SDL_Texture* powTex = IMG_LoadTexture(renderer, "pow_block.png");
    SDL_Texture* bgTex = IMG_LoadTexture(renderer, "background.png");

    // Load sounds (placeholders - replace with actual assets)
    Mix_Chunk* jumpSound = Mix_LoadWAV("jump.wav");
    Mix_Chunk* coinSound = Mix_LoadWAV("coin.wav");
    Mix_Chunk* bumpSound = Mix_LoadWAV("bump.wav");
    Mix_Chunk* powSound = Mix_LoadWAV("pow.wav");
    Mix_Music* bgMusic = Mix_LoadMUS("stage_music.mp3");

    // Mario setup
    Entity mario = {SCREEN_WIDTH / 2 - PLAYER_WIDTH / 2, SCREEN_HEIGHT - 100, 0, 0, PLAYER_WIDTH, PLAYER_HEIGHT, false, true};

    // Enemies (3 Shellcreepers for demo)
    Entity enemies[3] = {
        {100, 200, 1, 0, ENEMY_WIDTH, ENEMY_HEIGHT, false, true},
        {300, 200, -1, 0, ENEMY_WIDTH, ENEMY_HEIGHT, false, true},
        {500, 200, 1, 0, ENEMY_WIDTH, ENEMY_HEIGHT, false, true}
    };

    // Coins (5 for demo)
    Coin coins[5] = {
        {150, 300, COIN_SIZE, COIN_SIZE, true},
        {250, 300, COIN_SIZE, COIN_SIZE, true},
        {350, 300, COIN_SIZE, COIN_SIZE, true},
        {450, 300, COIN_SIZE, COIN_SIZE, true},
        {550, 300, COIN_SIZE, COIN_SIZE, true}
    };

    // Platforms (3 levels + ground)
    Platform platforms[4] = {
        {0, SCREEN_HEIGHT - 64, SCREEN_WIDTH, PLATFORM_HEIGHT}, // Ground
        {100, 500, SCREEN_WIDTH - 200, PLATFORM_HEIGHT},        // Bottom platform
        {150, 350, SCREEN_WIDTH - 300, PLATFORM_HEIGHT},        // Middle platform
        {200, 200, SCREEN_WIDTH - 400, PLATFORM_HEIGHT}         // Top platform
    };

    // Pipes (left and right)
    Platform pipes[2] = {
        {0, 0, 64, SCREEN_HEIGHT - 64},           // Left pipe
        {SCREEN_WIDTH - 64, 0, 64, SCREEN_HEIGHT - 64} // Right pipe
    };

    // POW block
    Platform powBlock = {SCREEN_WIDTH / 2 - 32, SCREEN_HEIGHT - 96, 64, 32};

    int score = 0;
    int lives = 3;
    int powHits = 3;
    bool running = true;
    SDL_Event event;

    Mix_PlayMusic(bgMusic, -1);

    while (running) {
        // Event handling
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_LEFT: mario.dx = -PLAYER_SPEED; break;
                    case SDLK_RIGHT: mario.dx = PLAYER_SPEED; break;
                    case SDLK_SPACE:
                        if (!mario.isJumping) {
                            mario.dy = JUMP_FORCE;
                            mario.isJumping = true;
                            Mix_PlayChannel(-1, jumpSound, 0);
                        }
                        break;
                }
            }
            if (event.type == SDL_KEYUP) {
                if (event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_RIGHT) mario.dx = 0;
            }
        }

        // Update Mario
        mario.x += mario.dx;
        mario.y += mario.dy;
        mario.dy += GRAVITY;

        // Platform collision for Mario
        for (int i = 0; i < 4; i++) {
            SDL_Rect marioRect = {(int)mario.x, (int)mario.y, mario.width, mario.height};
            SDL_Rect platRect = {platforms[i].x, platforms[i].y, platforms[i].width, platforms[i].height};
            if (SDL_HasIntersection(&marioRect, &platRect) && mario.dy > 0) {
                mario.y = platforms[i].y - mario.height;
                mario.dy = 0;
                mario.isJumping = false;
            }
        }

        // Pipe collision
        for (int i = 0; i < 2; i++) {
            SDL_Rect marioRect = {(int)mario.x, (int)mario.y, mario.width, mario.height};
            SDL_Rect pipeRect = {pipes[i].x, pipes[i].y, pipes[i].width, pipes[i].height};
            if (SDL_HasIntersection(&marioRect, &pipeRect)) {
                if (mario.dx > 0) mario.x = pipes[i].x - mario.width;
                else if (mario.dx < 0) mario.x = pipes[i].x + pipes[i].width;
            }
        }

        // Update enemies
        for (int i = 0; i < 3; i++) {
            if (enemies[i].alive) {
                enemies[i].x += enemies[i].dx;
                enemies[i].y += enemies[i].dy;
                enemies[i].dy += GRAVITY;

                // Platform collision for enemies
                for (int j = 0; j < 4; j++) {
                    SDL_Rect enemyRect = {(int)enemies[i].x, (int)enemies[i].y, enemies[i].width, enemies[i].height};
                    SDL_Rect platRect = {platforms[j].x, platforms[j].y, platforms[j].width, platforms[j].height};
                    if (SDL_HasIntersection(&enemyRect, &platRect) && enemies[i].dy > 0) {
                        enemies[i].y = platforms[j].y - enemies[i].height;
                        enemies[i].dy = 0;
                    }
                }

                // Reverse direction at pipes
                if (enemies[i].x <= pipes[0].x + pipes[0].width || enemies[i].x + enemies[i].width >= pipes[1].x)
                    enemies[i].dx = -enemies[i].dx;

                // Mario bumps enemy from below
                SDL_Rect marioRect = {(int)mario.x, (int)mario.y, mario.width, mario.height};
                SDL_Rect enemyRect = {(int)enemies[i].x, (int)enemies[i].y, enemies[i].width, enemies[i].height};
                if (SDL_HasIntersection(&marioRect, &enemyRect) && mario.y + mario.height <= enemies[i].y + 10) {
                    enemies[i].alive = false;
                    score += 800; // Shellcreeper points
                    Mix_PlayChannel(-1, bumpSound, 0);
                }
            }
        }

        // Coin collection
        for (int i = 0; i < 5; i++) {
            if (coins[i].active) {
                SDL_Rect marioRect = {(int)mario.x, (int)mario.y, mario.width, mario.height};
                SDL_Rect coinRect = {coins[i].x, coins[i].y, coins[i].width, coins[i].height};
                if (SDL_HasIntersection(&marioRect, &coinRect)) {
                    coins[i].active = false;
                    score += 800; // Coin points
                    Mix_PlayChannel(-1, coinSound, 0);
                }
            }
        }

        // POW block interaction
        SDL_Rect marioRect = {(int)mario.x, (int)mario.y, mario.width, mario.height};
        SDL_Rect powRect = {powBlock.x, powBlock.y, powBlock.width, powBlock.height};
        if (SDL_HasIntersection(&marioRect, &powRect) && powHits > 0 && mario.dy > 0) {
            powHits--;
            for (int i = 0; i < 3; i++) {
                if (enemies[i].alive) {
                    enemies[i].alive = false;
                    score += 800;
                }
            }
            Mix_PlayChannel(-1, powSound, 0);
        }

        // Rendering
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, bgTex, NULL, NULL); // Background

        // Draw platforms
        for (int i = 0; i < 4; i++) {
            SDL_Rect platRect = {platforms[i].x, platforms[i].y, platforms[i].width, platforms[i].height};
            SDL_RenderCopy(renderer, platformTex, NULL, &platRect);
        }

        // Draw pipes
        for (int i = 0; i < 2; i++) {
            SDL_Rect pipeRect = {pipes[i].x, pipes[i].y, pipes[i].width, pipes[i].height};
            SDL_RenderCopy(renderer, pipeTex, NULL, &pipeRect);
        }

        // Draw POW block
        if (powHits > 0) {
            SDL_Rect powRectRender = {powBlock.x, powBlock.y, powBlock.width, powBlock.height};
            SDL_RenderCopy(renderer, powTex, NULL, &powRectRender);
        }

        // Draw coins
        for (int i = 0; i < 5; i++) {
            if (coins[i].active) {
                SDL_Rect coinRect = {coins[i].x, coins[i].y, coins[i].width, coins[i].height};
                SDL_RenderCopy(renderer, coinTex, NULL, &coinRect);
            }
        }

        // Draw enemies
        for (int i = 0; i < 3; i++) {
            if (enemies[i].alive) {
                SDL_Rect enemyRect = {(int)enemies[i].x, (int)enemies[i].y, enemies[i].width, enemies[i].height};
                SDL_RenderCopy(renderer, enemyTex, NULL, &enemyRect);
            }
        }

        // Draw Mario
        if (mario.alive) {
            SDL_Rect marioRectRender = {(int)mario.x, (int)mario.y, mario.width, mario.height};
            SDL_RenderCopy(renderer, marioTex, NULL, &marioRectRender);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60 FPS
    }

    // Cleanup
    Mix_FreeChunk(jumpSound);
    Mix_FreeChunk(coinSound);
    Mix_FreeChunk(bumpSound);
    Mix_FreeChunk(powSound);
    Mix_FreeMusic(bgMusic);
    SDL_DestroyTexture(marioTex);
    SDL_DestroyTexture(enemyTex);
    SDL_DestroyTexture(coinTex);
    SDL_DestroyTexture(platformTex);
    SDL_DestroyTexture(pipeTex);
    SDL_DestroyTexture(powTex);
    SDL_DestroyTexture(bgTex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_Quit();
    IMG_Quit();
    SDL_Quit();

    printf("Game Over! Final Score: %d\n", score);
    return 0;
}
