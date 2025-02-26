#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <stdbool.h>
#include <stdio.h>

#define SCREEN_WIDTH 384  // Original arcade resolution scaled (224x288 -> 384x480)
#define SCREEN_HEIGHT 480
#define CAR_WIDTH 16
#define CAR_HEIGHT 16
#define FLAG_SIZE 16
#define ROCK_SIZE 16
#define RADAR_WIDTH 96
#define RADAR_HEIGHT 120
#define MAZE_WIDTH 32   // 32x24 tile map
#define MAZE_HEIGHT 24
#define TILE_SIZE 16

typedef struct {
    float x, y;
    float dx, dy;
    int width, height;
    int direction; // 0: right, 1: up, 2: left, 3: down
} Entity;

typedef struct {
    int x, y;
    bool collected;
} Flag;

typedef struct {
    int x, y;
} Rock;

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
    SDL_Window* window = SDL_CreateWindow("Rally-X Clone",
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
    SDL_Texture* playerTex = IMG_LoadTexture(renderer, "blue_car.png");
    SDL_Texture* enemyTex = IMG_LoadTexture(renderer, "red_car.png");
    SDL_Texture* flagTex = IMG_LoadTexture(renderer, "flag.png");
    SDL_Texture* rockTex = IMG_LoadTexture(renderer, "rock.png");
    SDL_Texture* smokeTex = IMG_LoadTexture(renderer, "smoke.png");
    SDL_Texture* mazeTex = IMG_LoadTexture(renderer, "maze.png");
    SDL_Texture* radarTex = IMG_LoadTexture(renderer, "radar.png");

    // Load sounds (placeholders - replace with actual assets)
    Mix_Chunk* engineSound = Mix_LoadWAV("engine.wav");
    Mix_Chunk* flagSound = Mix_LoadWAV("flag.wav");
    Mix_Chunk* crashSound = Mix_LoadWAV("crash.wav");
    Mix_Chunk* smokeSound = Mix_LoadWAV("smoke.wav");
    Mix_Music* bgMusic = Mix_LoadMUS("rallyx_music.mp3");

    // Player setup
    Entity player = {TILE_SIZE * 2, TILE_SIZE * 2, 2.0f, 0, CAR_WIDTH, CAR_HEIGHT, 0};

    // Enemies (3 red cars for demo)
    Entity enemies[3] = {
        {TILE_SIZE * 28, TILE_SIZE * 20, -2.0f, 0, CAR_WIDTH, CAR_HEIGHT, 2},
        {TILE_SIZE * 28, TILE_SIZE * 4, -2.0f, 0, CAR_WIDTH, CAR_HEIGHT, 2},
        {TILE_SIZE * 4, TILE_SIZE * 20, 2.0f, 0, CAR_WIDTH, CAR_HEIGHT, 0}
    };

    // Flags (10 per level as per original)
    Flag flags[10] = {
        {TILE_SIZE * 5, TILE_SIZE * 5, false}, {TILE_SIZE * 15, TILE_SIZE * 5, false},
        {TILE_SIZE * 25, TILE_SIZE * 5, false}, {TILE_SIZE * 5, TILE_SIZE * 10, false},
        {TILE_SIZE * 15, TILE_SIZE * 10, false}, {TILE_SIZE * 25, TILE_SIZE * 10, false},
        {TILE_SIZE * 5, TILE_SIZE * 15, false}, {TILE_SIZE * 15, TILE_SIZE * 15, false},
        {TILE_SIZE * 25, TILE_SIZE * 15, false}, {TILE_SIZE * 15, TILE_SIZE * 20, false}
    };

    // Rocks (3 for demo)
    Rock rocks[3] = {{TILE_SIZE * 10, TILE_SIZE * 8}, {TILE_SIZE * 20, TILE_SIZE * 12}, {TILE_SIZE * 12, TILE_SIZE * 18}};

    // Simple maze layout (1 = wall, 0 = path)
    int maze[MAZE_HEIGHT][MAZE_WIDTH] = {
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,1,1,1,1,0,1,1,1,1,0,1,1,1,1,0,1,1,1,1,0,1,1,1,1,0,1,1,1,0,1},
        {1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1},
        {1,0,1,0,1,1,1,1,0,1,1,1,1,0,1,1,1,1,0,1,1,1,1,0,1,1,1,1,0,1,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,1,1,1,1,0,1,1,1,1,0,1,1,1,1,0,1,1,1,1,0,1,1,1,1,0,1,1,1,0,1},
        {1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1},
        {1,0,1,0,1,1,1,1,0,1,1,1,1,0,1,1,1,1,0,1,1,1,1,0,1,1,1,1,0,1,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,1,1,1,1,0,1,1,1,1,0,1,1,1,1,0,1,1,1,1,0,1,1,1,1,0,1,1,1,0,1},
        {1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1},
        {1,0,1,0,1,1,1,1,0,1,1,1,1,0,1,1,1,1,0,1,1,1,1,0,1,1,1,1,0,1,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,1,1,1,1,0,1,1,1,1,0,1,1,1,1,0,1,1,1,1,0,1,1,1,1,0,1,1,1,0,1},
        {1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1},
        {1,0,1,0,1,1,1,1,0,1,1,1,1,0,1,1,1,1,0,1,1,1,1,0,1,1,1,1,0,1,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,1,1,1,1,0,1,1,1,1,0,1,1,1,1,0,1,1,1,1,0,1,1,1,1,0,1,1,1,0,1},
        {1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1},
        {1,0,1,0,1,1,1,1,0,1,1,1,1,0,1,1,1,1,0,1,1,1,1,0,1,1,1,1,0,1,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,1,1,1,1,0,1,1,1,1,0,1,1,1,1,0,1,1,1,1,0,1,1,1,1,0,1,1,1,0,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
    };

    int score = 0;
    int lives = 3;
    int fuel = 5000; // Fuel decreases over time
    int smokeScreen = 3; // 3 uses per level
    bool smokeActive = false;
    Uint32 smokeTimer = 0;
    int flagsCollected = 0;
    int currentLevel = 1;
    bool running = true;
    SDL_Event event;

    Mix_PlayMusic(bgMusic, -1);
    Mix_PlayChannel(-1, engineSound, -1); // Looping engine sound

    while (running) {
        // Event handling
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_RIGHT: player.direction = 0; player.dx = 2.0f; player.dy = 0; break;
                    case SDLK_UP: player.direction = 1; player.dx = 0; player.dy = -2.0f; break;
                    case SDLK_LEFT: player.direction = 2; player.dx = -2.0f; player.dy = 0; break;
                    case SDLK_DOWN: player.direction = 3; player.dx = 0; player.dy = 2.0f; break;
                    case SDLK_SPACE:
                        if (smokeScreen > 0) {
                            smokeActive = true;
                            smokeTimer = SDL_GetTicks();
                            smokeScreen--;
                            Mix_PlayChannel(-1, smokeSound, 0);
                        }
                        break;
                }
            }
        }

        // Update player
        float newX = player.x + player.dx;
        float newY = player.y + player.dy;
        int tileX = (int)(newX / TILE_SIZE);
        int tileY = (int)(newY / TILE_SIZE);
        if (tileX >= 0 && tileX < MAZE_WIDTH && tileY >= 0 && tileY < MAZE_HEIGHT && maze[tileY][tileX] == 0) {
            player.x = newX;
            player.y = newY;
        }
        if (player.x < 0) player.x = 0;
        if (player.x + player.width > MAZE_WIDTH * TILE_SIZE) player.x = MAZE_WIDTH * TILE_SIZE - player.width;
        if (player.y < 0) player.y = 0;
        if (player.y + player.height > MAZE_HEIGHT * TILE_SIZE) player.y = MAZE_HEIGHT * TILE_SIZE - player.height;

        // Update enemies (simple chase AI)
        for (int i = 0; i < 3; i++) {
            if (smokeActive && SDL_GetTicks() - smokeTimer < 2000) continue; // Enemies pause during smoke
            float dx = player.x - enemies[i].x;
            float dy = player.y - enemies[i].y;
            if (dx > 0) enemies[i].dx = 2.0f;
            else if (dx < 0) enemies[i].dx = -2.0f;
            else enemies[i].dx = 0;
            if (dy > 0) enemies[i].dy = 2.0f;
            else if (dy < 0) enemies[i].dy = -2.0f;
            else enemies[i].dy = 0;

            newX = enemies[i].x + enemies[i].dx;
            newY = enemies[i].y + enemies[i].dy;
            tileX = (int)(newX / TILE_SIZE);
            tileY = (int)(newY / TILE_SIZE);
            if (tileX >= 0 && tileX < MAZE_WIDTH && tileY >= 0 && tileY < MAZE_HEIGHT && maze[tileY][tileX] == 0) {
                enemies[i].x = newX;
                enemies[i].y = newY;
            }
            if (enemies[i].x < 0) enemies[i].x = 0;
            if (enemies[i].x + enemies[i].width > MAZE_WIDTH * TILE_SIZE) enemies[i].x = MAZE_WIDTH * TILE_SIZE - enemies[i].width;
            if (enemies[i].y < 0) enemies[i].y = 0;
            if (enemies[i].y + enemies[i].height > MAZE_HEIGHT * TILE_SIZE) enemies[i].y = MAZE_HEIGHT * TILE_SIZE - enemies[i].height;

            // Collision with player
            SDL_Rect playerRect = {(int)player.x, (int)player.y, player.width, player.height};
            SDL_Rect enemyRect = {(int)enemies[i].x, (int)enemies[i].y, enemies[i].width, enemies[i].height};
            if (SDL_HasIntersection(&playerRect, &enemyRect)) {
                lives--;
                Mix_PlayChannel(-1, crashSound, 0);
                player.x = TILE_SIZE * 2;
                player.y = TILE_SIZE * 2;
                if (lives <= 0) running = false;
            }
        }

        // Flag collection
        for (int i = 0; i < 10; i++) {
            if (!flags[i].collected) {
                SDL_Rect playerRect = {(int)player.x, (int)player.y, player.width, player.height};
                SDL_Rect flagRect = {flags[i].x, flags[i].y, FLAG_SIZE, FLAG_SIZE};
                if (SDL_HasIntersection(&playerRect, &flagRect)) {
                    flags[i].collected = true;
                    score += 100 * (flagsCollected + 1); // Bonus increases with each flag
                    fuel += 100; // Fuel bonus
                    flagsCollected++;
                    Mix_PlayChannel(-1, flagSound, 0);
                    if (flagsCollected == 10) {
                        currentLevel++;
                        flagsCollected = 0;
                        smokeScreen = 3;
                        for (int j = 0; j < 10; j++) flags[j].collected = false;
                    }
                }
            }
        }

        // Rock collision
        for (int i = 0; i < 3; i++) {
            SDL_Rect playerRect = {(int)player.x, (int)player.y, player.width, player.height};
            SDL_Rect rockRect = {rocks[i].x, rocks[i].y, ROCK_SIZE, ROCK_SIZE};
            if (SDL_HasIntersection(&playerRect, &rockRect)) {
                lives--;
                Mix_PlayChannel(-1, crashSound, 0);
                player.x = TILE_SIZE * 2;
                player.y = TILE_SIZE * 2;
                if (lives <= 0) running = false;
            }
        }

        // Fuel management
        fuel--;
        if (fuel <= 0) {
            lives--;
            fuel = 5000;
            player.x = TILE_SIZE * 2;
            player.y = TILE_SIZE * 2;
            if (lives <= 0) running = false;
        }

        // Rendering
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Draw maze (scrolling view centered on player)
        int viewX = player.x - SCREEN_WIDTH / 2;
        int viewY = player.y - SCREEN_HEIGHT / 2;
        if (viewX < 0) viewX = 0;
        if (viewY < 0) viewY = 0;
        if (viewX > MAZE_WIDTH * TILE_SIZE - SCREEN_WIDTH) viewX = MAZE_WIDTH * TILE_SIZE - SCREEN_WIDTH;
        if (viewY > MAZE_HEIGHT * TILE_SIZE - SCREEN_HEIGHT) viewY = MAZE_HEIGHT * TILE_SIZE - SCREEN_HEIGHT;
        SDL_Rect srcRect = {viewX, viewY, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_Rect dstRect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_RenderCopy(renderer, mazeTex, &srcRect, &dstRect);

        // Draw flags
        for (int i = 0; i < 10; i++) {
            if (!flags[i].collected) {
                SDL_Rect flagRect = {flags[i].x - viewX, flags[i].y - viewY, FLAG_SIZE, FLAG_SIZE};
                SDL_RenderCopy(renderer, flagTex, NULL, &flagRect);
            }
        }

        // Draw rocks
        for (int i = 0; i < 3; i++) {
            SDL_Rect rockRect = {rocks[i].x - viewX, rocks[i].y - viewY, ROCK_SIZE, ROCK_SIZE};
            SDL_RenderCopy(renderer, rockTex, NULL, &rockRect);
        }

        // Draw enemies
        for (int i = 0; i < 3; i++) {
            SDL_Rect enemyRect = {(int)enemies[i].x - viewX, (int)enemies[i].y - viewY, enemies[i].width, enemies[i].height};
            SDL_RenderCopy(renderer, enemyTex, NULL, &enemyRect);
        }

        // Draw player
        SDL_Rect playerRect = {(int)player.x - viewX, (int)player.y - viewY, player.width, player.height};
        SDL_RenderCopy(renderer, playerTex, NULL, &playerRect);

        // Draw smoke screen
        if (smokeActive && SDL_GetTicks() - smokeTimer < 2000) {
            SDL_Rect smokeRect = {(int)player.x - viewX - 16, (int)player.y - viewY, 32, 32};
            SDL_RenderCopy(renderer, smokeTex, NULL, &smokeRect);
        }

        // Draw radar (top-right corner)
        SDL_Rect radarRect = {SCREEN_WIDTH - RADAR_WIDTH, 0, RADAR_WIDTH, RADAR_HEIGHT};
        SDL_RenderCopy(renderer, radarTex, NULL, &radarRect);
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // Player dot (blue)
        SDL_Rect playerRadar = {(int)(SCREEN_WIDTH - RADAR_WIDTH + (player.x / TILE_SIZE) * 3), (int)((player.y / TILE_SIZE) * 5), 3, 3};
        SDL_RenderFillRect(renderer, &playerRadar);
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Enemy dots (red)
        for (int i = 0; i < 3; i++) {
            SDL_Rect enemyRadar = {(int)(SCREEN_WIDTH - RADAR_WIDTH + (enemies[i].x / TILE_SIZE) * 3), (int)((enemies[i].y / TILE_SIZE) * 5), 3, 3};
            SDL_RenderFillRect(renderer, &enemyRadar);
        }
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // Flag dots (yellow)
        for (int i = 0; i < 10; i++) {
            if (!flags[i].collected) {
                SDL_Rect flagRadar = {(int)(SCREEN_WIDTH - RADAR_WIDTH + (flags[i].x / TILE_SIZE) * 3), (int)((flags[i].y / TILE_SIZE) * 5), 3, 3};
                SDL_RenderFillRect(renderer, &flagRadar);
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60 FPS
    }

    // Cleanup
    Mix_FreeChunk(engineSound);
    Mix_FreeChunk(flagSound);
    Mix_FreeChunk(crashSound);
    Mix_FreeChunk(smokeSound);
    Mix_FreeMusic(bgMusic);
    SDL_DestroyTexture(playerTex);
    SDL_DestroyTexture(enemyTex);
    SDL_DestroyTexture(flagTex);
    SDL_DestroyTexture(rockTex);
    SDL_DestroyTexture(smokeTex);
    SDL_DestroyTexture(mazeTex);
    SDL_DestroyTexture(radarTex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_Quit();
    IMG_Quit();
    SDL_Quit();

    printf("Game Over! Final Score: %d\n", score);
    return 0;
}
