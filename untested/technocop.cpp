#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <stdbool.h>
#include <stdio.h>

#define SCREEN_WIDTH 320   // Amiga resolution
#define SCREEN_HEIGHT 200
#define PLAYER_WIDTH 16
#define PLAYER_HEIGHT 24
#define ENEMY_WIDTH 16
#define ENEMY_HEIGHT 16
#define GRAVITY 0.2f
#define JUMP_FORCE -5.0f
#define MOVE_SPEED 2.0f
#define TIME_LIMIT 60  // Seconds per level, adjustable per original

typedef struct {
    float x, y;
    float dx, dy;
    int width, height;
    bool isJumping;
    int health;    // 5 hits max
    bool usingNet; // Toggle between gun and net
} Player;

typedef struct {
    float x, y;
    int width, height;
    bool alive;
    bool isBoss;  // Boss requires net
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
    SDL_Window* window = SDL_CreateWindow("Techno Cop Clone (Side-Scrolling)",
                                        SDL_WINDOWPOS_CENTERED,
                                        SDL_WINDOWPOS_CENTERED,
                                        SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2,
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

    // Load textures (placeholders)
    SDL_Texture* playerTex = IMG_LoadTexture(renderer, "technocop.png"); // Player sprite
    SDL_Texture* enemyTex = IMG_LoadTexture(renderer, "enemy.png");      // Regular enemy
    SDL_Texture* bossTex = IMG_LoadTexture(renderer, "boss.png");        // Boss enemy
    SDL_Texture* goreTex = IMG_LoadTexture(renderer, "gore.png");        // Gore puddle
    SDL_Texture* bgTex = IMG_LoadTexture(renderer, "building_bg.png");   // Background
    SDL_Texture* platformTex = IMG_LoadTexture(renderer, "platform.png");
    SDL_Texture* hudTex = IMG_LoadTexture(renderer, "wrist_hud.png");    // Wrist console

    // Load sounds (placeholders)
    Mix_Chunk* gunSound = Mix_LoadWAV("gun.wav");    // AutoMag shot
    Mix_Chunk* netSound = Mix_LoadWAV("net.wav");    // Snare gun
    Mix_Chunk* hurtSound = Mix_LoadWAV("hurt.wav");  // Damage taken
    Mix_Music* bgMusic = Mix_LoadMUS("level_music.mp3");

    // Player setup
    Player player = {50, SCREEN_HEIGHT - PLAYER_HEIGHT - 8, 0, 0, PLAYER_WIDTH, PLAYER_HEIGHT, false, 5, false};

    // Enemies (demo: 2 regular + 1 boss)
    Enemy enemies[3] = {
        {200, SCREEN_HEIGHT - ENEMY_HEIGHT - 8, ENEMY_WIDTH, ENEMY_HEIGHT, true, false}, // Thug 1
        {300, SCREEN_HEIGHT - ENEMY_HEIGHT - 8, ENEMY_WIDTH, ENEMY_HEIGHT, true, false}, // Thug 2
        {450, SCREEN_HEIGHT - ENEMY_HEIGHT - 8, ENEMY_WIDTH, ENEMY_HEIGHT, true, true}   // Boss
    };

    // Platforms (demo: ground and one platform)
    Platform platforms[2] = {
        {0, SCREEN_HEIGHT - 8, SCREEN_WIDTH, 8},  // Ground
        {150, SCREEN_HEIGHT - 40, 100, 8}         // Elevated platform
    };

    int score = 0;
    int lives = 1; // One life, extra earned per boss
    int rank = 1;  // Start as Grunt (1-12 ranks)
    Uint32 startTime = SDL_GetTicks();
    bool running = true;
    SDL_Event event;

    Mix_PlayMusic(bgMusic, -1);

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
                    case SDLK_SPACE: // Shoot gun
                        player.usingNet = false;
                        Mix_PlayChannel(-1, gunSound, 0);
                        break;
                    case SDLK_n: // Use net
                        player.usingNet = true;
                        Mix_PlayChannel(-1, netSound, 0);
                        break;
                }
            }
            if (event.type == SDL_KEYUP) {
                if (event.key.keysym.sym == SDLK_RIGHT || event.key.keysym.sym == SDLK_LEFT) player.dx = 0;
            }
        }

        // Time limit check
        Uint32 elapsedTime = (SDL_GetTicks() - startTime) / 1000;
        if (elapsedTime >= TIME_LIMIT) {
            lives--;
            player.x = 50;
            player.y = SCREEN_HEIGHT - PLAYER_HEIGHT - 8;
            startTime = SDL_GetTicks();
            if (lives <= 0) running = false;
        }

        // Update player
        player.x += player.dx;
        player.y += player.dy;
        player.dy += GRAVITY;

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
        for (int i = 0; i < 3; i++) {
            if (enemies[i].alive) {
                enemies[i].x -= 1; // Simple leftward patrol
                if (enemies[i].x < -ENEMY_WIDTH) enemies[i].x = SCREEN_WIDTH;

                SDL_Rect enemyRect = {(int)enemies[i].x, (int)enemies[i].y, enemies[i].width, enemies[i].height};
                SDL_Rect playerRect = {(int)player.x, (int)player.y, player.width, player.height};
                if (SDL_HasIntersection(&playerRect, &enemyRect)) {
                    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE && !player.usingNet) {
                        enemies[i].alive = false; // Gun kills with gore
                        score += 10;
                    } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_n && player.usingNet && enemies[i].isBoss) {
                        enemies[i].alive = false; // Net captures boss
                        score += 50;
                        lives++;
                        rank = (rank < 12) ? rank + 1 : 12; // Promotion
                        startTime = SDL_GetTicks(); // Reset time
                    } else {
                        player.health--;
                        Mix_PlayChannel(-1, hurtSound, 0);
                        if (player.health <= 0) {
                            lives--;
                            player.x = 50;
                            player.y = SCREEN_HEIGHT - PLAYER_HEIGHT - 8;
                            player.health = 5;
                            startTime = SDL_GetTicks();
                            if (lives <= 0) running = false;
                        }
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

        // Draw enemies and gore
        for (int i = 0; i < 3; i++) {
            if (enemies[i].alive) {
                SDL_Rect enemyRect = {(int)enemies[i].x, (int)enemies[i].y, enemies[i].width, enemies[i].height};
                SDL_RenderCopy(renderer, enemies[i].isBoss ? bossTex : enemyTex, NULL, &enemyRect);
            } else if (!enemies[i].isBoss) { // Gore for regular enemies
                SDL_Rect goreRect = {(int)enemies[i].x, (int)enemies[i].y + enemies[i].height - 8, 16, 8};
                SDL_RenderCopy(renderer, goreTex, NULL, &goreRect);
            }
        }

        // Draw player
        SDL_Rect playerRect = {(int)player.x, (int)player.y, player.width, player.height};
        SDL_RenderCopy(renderer, playerTex, NULL, &playerRect);

        // Draw wrist HUD (simplified)
        SDL_Rect hudRect = {0, 0, SCREEN_WIDTH, 32};
        SDL_RenderCopy(renderer, hudTex, NULL, &hudRect);

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60 FPS
    }

    // Cleanup
    Mix_FreeChunk(gunSound);
    Mix_FreeChunk(netSound);
    Mix_FreeChunk(hurtSound);
    Mix_FreeMusic(bgMusic);
    SDL_DestroyTexture(playerTex);
    SDL_DestroyTexture(enemyTex);
    SDL_DestroyTexture(bossTex);
    SDL_DestroyTexture(goreTex);
    SDL_DestroyTexture(bgTex);
    SDL_DestroyTexture(platformTex);
    SDL_DestroyTexture(hudTex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_Quit();
    IMG_Quit();
    SDL_Quit();

    printf("Game Over! Final Score: %d, Rank: %d\n", score, rank);
    return 0;
}
