#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <stdbool.h>
#include <stdio.h>

#define SCREEN_WIDTH 320  // Amiga resolution
#define SCREEN_HEIGHT 200
#define PLAYER_WIDTH 64   // Large sprites as per original
#define PLAYER_HEIGHT 128
#define ENEMY_WIDTH 64
#define ENEMY_HEIGHT 128
#define POTION_SIZE 32
#define LEVEL_COUNT 11

typedef struct {
    float x, y;
    float dx;
    int width, height;
    bool attacking;
    bool kneeling;
    int health;
} Player;

typedef struct {
    float x, y;
    int width, height;
    bool alive;
    int health;
} Enemy;

typedef struct {
    float x, y;
    int width, height;
    bool active;
    int type; // 0: Extra Life, 1: Strength, 2: Zapper, 3: Shield
} Potion;

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
    SDL_Window* window = SDL_CreateWindow("Sword of Sodan Clone",
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
    SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT); // Maintain Amiga resolution

    // Load textures (placeholders - replace with actual assets)
    SDL_Texture* playerTex = IMG_LoadTexture(renderer, "brodan.png"); // or "shardan.png"
    SDL_Texture* enemyTex = IMG_LoadTexture(renderer, "guard.png");   // Generic enemy for demo
    SDL_Texture* potionTex = IMG_LoadTexture(renderer, "potion.png");
    SDL_Texture* bgTex[LEVEL_COUNT];
    bgTex[0] = IMG_LoadTexture(renderer, "city_gates.png");
    bgTex[1] = IMG_LoadTexture(renderer, "bridge.png");
    bgTex[2] = IMG_LoadTexture(renderer, "city_streets.png");
    bgTex[3] = IMG_LoadTexture(renderer, "forest.png");
    bgTex[4] = IMG_LoadTexture(renderer, "anthill.png");
    bgTex[5] = IMG_LoadTexture(renderer, "dungeon.png");
    bgTex[6] = IMG_LoadTexture(renderer, "catacombs.png");
    bgTex[7] = IMG_LoadTexture(renderer, "caverns.png");
    bgTex[8] = IMG_LoadTexture(renderer, "lava_pits.png");
    bgTex[9] = IMG_LoadTexture(renderer, "castle.png");
    bgTex[10] = IMG_LoadTexture(renderer, "throne_room.png");

    // Load sounds (placeholders - replace with actual assets)
    Mix_Chunk* swordSound = Mix_LoadWAV("sword.wav");
    Mix_Chunk* potionSound = Mix_LoadWAV("potion.wav");
    Mix_Chunk* deathSound = Mix_LoadWAV("death.wav");
    Mix_Chunk* enemyDeathSound = Mix_LoadWAV("enemy_die.wav");
    Mix_Music* introMusic = Mix_LoadMUS("intro.mp3");
    Mix_Music* gameOverMusic = Mix_LoadMUS("game_over.mp3");

    // Player setup
    Player player = {50, SCREEN_HEIGHT - PLAYER_HEIGHT, 2.0f, PLAYER_WIDTH, PLAYER_HEIGHT, false, false, 3};

    // Enemies (one per level for demo, expand as needed)
    Enemy enemies[LEVEL_COUNT] = {
        {200, SCREEN_HEIGHT - ENEMY_HEIGHT, ENEMY_WIDTH, ENEMY_HEIGHT, true, 3}, // City Gates guard
        {200, SCREEN_HEIGHT - ENEMY_HEIGHT, ENEMY_WIDTH, ENEMY_HEIGHT, true, 3}, // Bridge guard
        {200, SCREEN_HEIGHT - ENEMY_HEIGHT, ENEMY_WIDTH, ENEMY_HEIGHT, true, 3}, // City Streets guard
        {200, SCREEN_HEIGHT - ENEMY_HEIGHT, ENEMY_WIDTH, ENEMY_HEIGHT, true, 3}, // Forest creature
        {200, SCREEN_HEIGHT - ENEMY_HEIGHT, ENEMY_WIDTH, ENEMY_HEIGHT, true, 3}, // Anthill ants
        {200, SCREEN_HEIGHT - ENEMY_HEIGHT, ENEMY_WIDTH, ENEMY_HEIGHT, true, 3}, // Dungeon ostrich
        {200, SCREEN_HEIGHT - ENEMY_HEIGHT, ENEMY_WIDTH, ENEMY_HEIGHT, true, 3}, // Catacombs skeleton
        {200, SCREEN_HEIGHT - ENEMY_HEIGHT, ENEMY_WIDTH, ENEMY_HEIGHT, true, 3}, // Caverns beast
        {200, SCREEN_HEIGHT - ENEMY_HEIGHT, ENEMY_WIDTH, ENEMY_HEIGHT, true, 3}, // Lava Pits demon
        {200, SCREEN_HEIGHT - ENEMY_HEIGHT, ENEMY_WIDTH, ENEMY_HEIGHT, true, 3}, // Castle knight
        {200, SCREEN_HEIGHT - ENEMY_HEIGHT, ENEMY_WIDTH, ENEMY_HEIGHT, true, 5}  // Zoras
    };

    // Potions (one per level for demo)
    Potion potions[LEVEL_COUNT] = {
        {150, SCREEN_HEIGHT - POTION_SIZE - 20, POTION_SIZE, POTION_SIZE, true, 0}, // Extra Life
        {150, SCREEN_HEIGHT - POTION_SIZE - 20, POTION_SIZE, POTION_SIZE, true, 1}, // Strength
        {150, SCREEN_HEIGHT - POTION_SIZE - 20, POTION_SIZE, POTION_SIZE, true, 2}, // Zapper
        {150, SCREEN_HEIGHT - POTION_SIZE - 20, POTION_SIZE, POTION_SIZE, true, 3}, // Shield
        {150, SCREEN_HEIGHT - POTION_SIZE - 20, POTION_SIZE, POTION_SIZE, true, 0},
        {150, SCREEN_HEIGHT - POTION_SIZE - 20, POTION_SIZE, POTION_SIZE, true, 1},
        {150, SCREEN_HEIGHT - POTION_SIZE - 20, POTION_SIZE, POTION_SIZE, true, 2},
        {150, SCREEN_HEIGHT - POTION_SIZE - 20, POTION_SIZE, POTION_SIZE, true, 3},
        {150, SCREEN_HEIGHT - POTION_SIZE - 20, POTION_SIZE, POTION_SIZE, true, 0},
        {150, SCREEN_HEIGHT - POTION_SIZE - 20, POTION_SIZE, POTION_SIZE, true, 1},
        {150, SCREEN_HEIGHT - POTION_SIZE - 20, POTION_SIZE, POTION_SIZE, true, 2}
    };

    int currentLevel = 0;
    int score = 0;
    bool running = true;
    bool shieldActive = false;
    Uint32 shieldTimer = 0;
    SDL_Event event;

    Mix_PlayMusic(introMusic, 1);

    while (running) {
        // Event handling
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_RIGHT: player.dx = 2.0f; break;
                    case SDLK_LEFT: player.dx = -2.0f; break; // Walk backwards
                    case SDLK_SPACE: player.attacking = true; Mix_PlayChannel(-1, swordSound, 0); break;
                    case SDLK_DOWN: player.kneeling = true; break;
                    case SDLK_1: // Use potion 1 (metallic bottle)
                        if (potions[currentLevel].active && potions[currentLevel].type == 0) player.health++;
                        else if (potions[currentLevel].type == 1) player.dx += 1.0f; // Strength boost
                        else if (potions[currentLevel].type == 2) enemies[currentLevel].alive = false;
                        else if (potions[currentLevel].type == 3) { shieldActive = true; shieldTimer = SDL_GetTicks(); }
                        potions[currentLevel].active = false;
                        Mix_PlayChannel(-1, potionSound, 0);
                        break;
                }
            }
            if (event.type == SDL_KEYUP) {
                if (event.key.keysym.sym == SDLK_RIGHT || event.key.keysym.sym == SDLK_LEFT) player.dx = 0;
                if (event.key.keysym.sym == SDLK_SPACE) player.attacking = false;
                if (event.key.keysym.sym == SDLK_DOWN) player.kneeling = false;
            }
        }

        // Update player
        player.x += player.dx;
        if (player.x < 0) player.x = 0;
        if (player.x + player.width > SCREEN_WIDTH) {
            currentLevel++;
            if (currentLevel >= LEVEL_COUNT) {
                running = false; // Game won
            } else {
                player.x = 0; // Reset to start of next level
            }
        }

        // Update enemy
        if (enemies[currentLevel].alive) {
            SDL_Rect playerRect = {(int)player.x, (int)player.y, player.width, player.height};
            SDL_Rect enemyRect = {(int)enemies[currentLevel].x, (int)enemies[currentLevel].y, enemies[currentLevel].width, enemies[currentLevel].height};
            if (SDL_HasIntersection(&playerRect, &enemyRect)) {
                if (player.attacking) {
                    enemies[currentLevel].health--;
                    if (enemies[currentLevel].health <= 0) {
                        enemies[currentLevel].alive = false;
                        score += 100;
                        Mix_PlayChannel(-1, enemyDeathSound, 0);
                    }
                } else if (!shieldActive) {
                    player.health--;
                    if (player.health <= 0) {
                        Mix_PlayChannel(-1, deathSound, 0);
                        Mix_PlayMusic(gameOverMusic, 1);
                        running = false;
                    }
                }
            }
        }

        // Potion collection
        if (potions[currentLevel].active) {
            SDL_Rect playerRect = {(int)player.x, (int)player.y, player.width, player.height};
            SDL_Rect potionRect = {(int)potions[currentLevel].x, (int)potions[currentLevel].y, potions[currentLevel].width, potions[currentLevel].height};
            if (SDL_HasIntersection(&playerRect, &potionRect)) {
                potions[currentLevel].active = false;
                Mix_PlayChannel(-1, potionSound, 0);
            }
        }

        // Shield timeout
        if (shieldActive && SDL_GetTicks() - shieldTimer > 30000) shieldActive = false;

        // Rendering
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, bgTex[currentLevel], NULL, NULL);

        // Draw potion
        if (potions[currentLevel].active) {
            SDL_Rect potionRect = {(int)potions[currentLevel].x, (int)potions[currentLevel].y, potions[currentLevel].width, potions[currentLevel].height};
            SDL_RenderCopy(renderer, potionTex, NULL, &potionRect);
        }

        // Draw enemy
        if (enemies[currentLevel].alive) {
            SDL_Rect enemyRect = {(int)enemies[currentLevel].x, (int)enemies[currentLevel].y, enemies[currentLevel].width, enemies[currentLevel].height};
            SDL_RenderCopy(renderer, enemyTex, NULL, &enemyRect);
        }

        // Draw player
        SDL_Rect playerRect = {(int)player.x, (int)player.y, player.width, player.height};
        SDL_RenderCopy(renderer, playerTex, NULL, &playerRect);

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60 FPS
    }

    // Cleanup
    Mix_FreeChunk(swordSound);
    Mix_FreeChunk(potionSound);
    Mix_FreeChunk(deathSound);
    Mix_FreeChunk(enemyDeathSound);
    Mix_FreeMusic(introMusic);
    Mix_FreeMusic(gameOverMusic);
    SDL_DestroyTexture(playerTex);
    SDL_DestroyTexture(enemyTex);
    SDL_DestroyTexture(potionTex);
    for (int i = 0; i < LEVEL_COUNT; i++) SDL_DestroyTexture(bgTex[i]);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_Quit();
    IMG_Quit();
    SDL_Quit();

    printf("Game Over! Final Score: %d\n", score);
    return 0;
}
