#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <stdbool.h>
#include <stdio.h>

#define SCREEN_WIDTH 320   // Amiga resolution
#define SCREEN_HEIGHT 200
#define PLAYER_WIDTH 32
#define PLAYER_HEIGHT 32
#define ENEMY_WIDTH 32
#define ENEMY_HEIGHT 32
#define GRAVITY 0.5f
#define JUMP_FORCE -8.0f
#define MOVE_SPEED 2.0f

typedef struct {
    float x, y;
    float dx, dy;
    int width, height;
    bool isJumping;
    int health;
} Player;

typedef struct {
    float x, y;
    int width, height;
    bool alive;
} Enemy;

typedef struct {
    float x;
    SDL_Texture* texture;
    float speed; // Parallax scroll speed
} BackgroundLayer;

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
    SDL_Window* window = SDL_CreateWindow("Shadow of the Beast Clone",
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
    SDL_Texture* playerTex = IMG_LoadTexture(renderer, "aarbron.png"); // Player sprite
    SDL_Texture* enemyTex = IMG_LoadTexture(renderer, "enemy.png");   // Generic enemy
    BackgroundLayer bgLayers[4] = {
        {0, IMG_LoadTexture(renderer, "sky.png"), 0.1f},      // Distant sky
        {0, IMG_LoadTexture(renderer, "mountains.png"), 0.3f}, // Mountains
        {0, IMG_LoadTexture(renderer, "grass.png"), 0.6f},     // Grass
        {0, IMG_LoadTexture(renderer, "foreground.png"), 1.0f}  // Foreground
    };
    SDL_Texture* platformTex = IMG_LoadTexture(renderer, "platform.png");

    // Load sounds (placeholders - replace with actual assets)
    Mix_Chunk* punchSound = Mix_LoadWAV("punch.wav");
    Mix_Chunk* hurtSound = Mix_LoadWAV("hurt.wav");
    Mix_Music* bgMusic = Mix_LoadMUS("beast_music.mp3");

    // Player setup
    Player player = {100, SCREEN_HEIGHT - PLAYER_HEIGHT - TILE_SIZE, 0, 0, PLAYER_WIDTH, PLAYER_HEIGHT, false, 12}; // 12 health as per original

    // Enemies (demo: 2 enemies)
    Enemy enemies[2] = {
        {300, SCREEN_HEIGHT - ENEMY_HEIGHT - TILE_SIZE, ENEMY_WIDTH, ENEMY_HEIGHT, true},
        {450, SCREEN_HEIGHT - ENEMY_HEIGHT - TILE_SIZE, ENEMY_WIDTH, ENEMY_HEIGHT, true}
    };

    // Platforms (demo: ground and one platform)
    Platform platforms[2] = {
        {0, SCREEN_HEIGHT - TILE_SIZE, SCREEN_WIDTH, TILE_SIZE}, // Ground
        {200, SCREEN_HEIGHT - TILE_SIZE - 50, 100, TILE_SIZE}   // Elevated platform
    };

    int score = 0;
    int lives = 1; // Single life as per original
    bool running = true;
    SDL_Event event;

    Mix_PlayMusic(bgMusic, -1); // Loop background music

    while (running) {
        // Event handling
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_RIGHT: player.dx = MOVE_SPEED; break;
                    case SDLK_LEFT: player.dx = -MOVE_SPEED; break;
                    case SDLK_UP:
                        if (!player.isJumping) {
                            player.dy = JUMP_FORCE;
                            player.isJumping = true;
                        }
                        break;
                    case SDLK_SPACE:
                        // Punch action
                        Mix_PlayChannel(-1, punchSound, 0);
                        break;
                }
            }
            if (event.type == SDL_KEYUP) {
                if (event.key.keysym.sym == SDLK_RIGHT || event.key.keysym.sym == SDLK_LEFT) player.dx = 0;
            }
        }

        // Update player
        player.x += player.dx;
        player.y += player.dy;
        player.dy += GRAVITY;

        // Update background scrolling
        for (int i = 0; i < 4; i++) {
            bgLayers[i].x -= player.dx * bgLayers[i].speed;
            if (bgLayers[i].x < -SCREEN_WIDTH) bgLayers[i].x += SCREEN_WIDTH;
            if (bgLayers[i].x > 0) bgLayers[i].x -= SCREEN_WIDTH;
        }

        // Platform collision
        for (int i = 0; i < 2; i++) {
            SDL_Rect playerRect = {(int)player.x, (int)player.y, player.width, player.height};
            SDL_Rect platRect = {platforms[i].x, platforms[i].y, platforms[i].width, platforms[i].height};
            if (SDL_HasIntersection(&playerRect, &platRect) && player.dy > 0) {
                player.y = platforms[i].y - player.height;
                player.dy = 0;
                player.isJumping = false;
            }
        }

        // Boundary checks
        if (player.x < 0) player.x = 0;
        if (player.x + player.width > SCREEN_WIDTH) player.x = SCREEN_WIDTH - player.width;
        if (player.y + player.height > SCREEN_HEIGHT) {
            player.y = SCREEN_HEIGHT - player.height;
            player.dy = 0;
            player.isJumping = false;
        }

        // Update enemies and combat
        for (int i = 0; i < 2; i++) {
            if (enemies[i].alive) {
                enemies[i].x -= 1; // Simple leftward movement
                if (enemies[i].x < -ENEMY_WIDTH) enemies[i].x = SCREEN_WIDTH; // Respawn on right

                SDL_Rect enemyRect = {(int)enemies[i].x, (int)enemies[i].y, enemies[i].width, enemies[i].height};
                SDL_Rect playerRect = {(int)player.x, (int)player.y, player.width, player.height};
                if (SDL_HasIntersection(&playerRect, &enemyRect)) {
                    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE) {
                        enemies[i].alive = false; // Punch kills enemy
                        score += 10;
                    } else {
                        player.health--;
                        Mix_PlayChannel(-1, hurtSound, 0);
                        if (player.health <= 0) {
                            lives--;
                            player.x = 100;
                            player.y = SCREEN_HEIGHT - PLAYER_HEIGHT - TILE_SIZE;
                            player.health = 12;
                            if (lives <= 0) running = false;
                        }
                    }
                }
            }
        }

        // Rendering
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Draw parallax backgrounds
        for (int i = 0; i < 4; i++) {
            SDL_Rect bgRect1 = {(int)bgLayers[i].x, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
            SDL_Rect bgRect2 = {(int)bgLayers[i].x + SCREEN_WIDTH, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
            SDL_RenderCopy(renderer, bgLayers[i].texture, NULL, &bgRect1);
            SDL_RenderCopy(renderer, bgLayers[i].texture, NULL, &bgRect2);
        }

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

        // Draw player
        SDL_Rect playerRect = {(int)player.x, (int)player.y, player.width, player.height};
        SDL_RenderCopy(renderer, playerTex, NULL, &playerRect);

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60 FPS
    }

    // Cleanup
    Mix_FreeChunk(punchSound);
    Mix_FreeChunk(hurtSound);
    Mix_FreeMusic(bgMusic);
    SDL_DestroyTexture(playerTex);
    SDL_DestroyTexture(enemyTex);
    for (int i = 0; i < 4; i++) SDL_DestroyTexture(bgLayers[i].texture);
    SDL_DestroyTexture(platformTex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_Quit();
    IMG_Quit();
    SDL_Quit();

    printf("Game Over! Final Score: %d\n", score);
    return 0;
}
