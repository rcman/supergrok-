#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define SPRITE_SIZE 64
#define BULLET_SIZE 8
#define PLAYER_SPEED 5.0f
#define BULLET_SPEED 8.0f
#define MAX_BULLETS 50
#define MAX_ENEMIES 20

typedef struct {
    float x, y;
    float dx;
    bool active;
    SDL_Texture* texture;
} Player;

typedef struct {
    float x, y;
    bool active;
    SDL_Texture* texture;
} Enemy;

typedef struct {
    float x, y;
    float dy;
    bool active;
    SDL_Texture* texture;
} Bullet;

// Load texture helper function
SDL_Texture* loadTexture(SDL_Renderer* renderer, const char* filename) {
    SDL_Surface* surface = IMG_Load(filename);
    if (!surface) {
        printf("Error loading %s: %s\n", filename, IMG_GetError());
        return NULL;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

// Initialize enemies in a formation
void initEnemies(Enemy* enemies, SDL_Texture* texture) {
    int rows = 4;
    int cols = 5;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        int row = i / cols;
        int col = i % cols;
        enemies[i].x = 200 + col * (SPRITE_SIZE + 10);
        enemies[i].y = 50 + row * (SPRITE_SIZE + 10);
        enemies[i].active = true;
        enemies[i].texture = texture;
    }
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0 || IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG) {
        printf("SDL/IMG Init Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Galaga Clone",
                                        SDL_WINDOWPOS_UNDEFINED,
                                        SDL_WINDOWPOS_UNDEFINED,
                                        SCREEN_WIDTH, SCREEN_HEIGHT,
                                        SDL_WINDOW_SHOWN);
    if (!window) return 1;

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) return 1;

    // Load textures
    SDL_Texture* backgroundTexture = loadTexture(renderer, "background.png");
    SDL_Texture* playerTexture = loadTexture(renderer, "player.png");
    SDL_Texture* enemyTexture = loadTexture(renderer, "enemy.png");
    SDL_Texture* bulletTexture = loadTexture(renderer, "bullet.png");

    if (!backgroundTexture || !playerTexture || !enemyTexture || !bulletTexture) {
        printf("Failed to load textures\n");
        SDL_Quit();
        IMG_Quit();
        return 1;
    }

    // Game objects
    Player player = {SCREEN_WIDTH / 2 - SPRITE_SIZE / 2, SCREEN_HEIGHT - SPRITE_SIZE - 20, 0, true, playerTexture};
    Enemy enemies[MAX_ENEMIES];
    Bullet playerBullets[MAX_BULLETS] = {0};
    initEnemies(enemies, enemyTexture);

    bool running = true;
    Uint32 lastShotTime = 0;
    const Uint32 shotDelay = 200;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
        }

        // Input handling
        const Uint8* keys = SDL_GetKeyboardState(NULL);
        player.dx = 0;
        if (keys[SDL_SCANCODE_LEFT] && player.x > 0) player.dx = -PLAYER_SPEED;
        if (keys[SDL_SCANCODE_RIGHT] && player.x < SCREEN_WIDTH - SPRITE_SIZE) player.dx = PLAYER_SPEED;

        Uint32 currentTime = SDL_GetTicks();
        if (keys[SDL_SCANCODE_SPACE] && (currentTime - lastShotTime) > shotDelay) {
            for (int i = 0; i < MAX_BULLETS; i++) {
                if (!playerBullets[i].active) {
                    playerBullets[i] = (Bullet){
                        player.x + SPRITE_SIZE / 2 - BULLET_SIZE / 2,
                        player.y - BULLET_SIZE,
                        -BULLET_SPEED,
                        true,
                        bulletTexture
                    };
                    lastShotTime = currentTime;
                    break;
                }
            }
        }

        // Update player
        player.x += player.dx;

        // Update bullets
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (playerBullets[i].active) {
                playerBullets[i].y += playerBullets[i].dy;
                if (playerBullets[i].y < 0) playerBullets[i].active = false;

                // Bullet-enemy collision
                for (int j = 0; j < MAX_ENEMIES; j++) {
                    if (enemies[j].active && playerBullets[i].active) {
                        SDL_Rect bulletRect = {(int)playerBullets[i].x, (int)playerBullets[i].y, BULLET_SIZE, BULLET_SIZE};
                        SDL_Rect enemyRect = {(int)enemies[j].x, (int)enemies[j].y, SPRITE_SIZE, SPRITE_SIZE};
                        if (SDL_HasIntersection(&bulletRect, &enemyRect)) {
                            playerBullets[i].active = false;
                            enemies[j].active = false;
                        }
                    }
                }
            }
        }

        // Rendering
        SDL_RenderClear(renderer);

        // Draw background
        SDL_RenderCopy(renderer, backgroundTexture, NULL, NULL);

        // Draw player
        if (player.active) {
            SDL_Rect playerRect = {(int)player.x, (int)player.y, SPRITE_SIZE, SPRITE_SIZE};
            SDL_RenderCopy(renderer, player.texture, NULL, &playerRect);
        }

        // Draw enemies
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (enemies[i].active) {
                SDL_Rect enemyRect = {(int)enemies[i].x, (int)enemies[i].y, SPRITE_SIZE, SPRITE_SIZE};
                SDL_RenderCopy(renderer, enemies[i].texture, NULL, &enemyRect);
            }
        }

        // Draw bullets
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (playerBullets[i].active) {
                SDL_Rect bulletRect = {(int)playerBullets[i].x, (int)playerBullets[i].y, BULLET_SIZE, BULLET_SIZE};
                SDL_RenderCopy(renderer, playerBullets[i].texture, NULL, &bulletRect);
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60 FPS
    }

    // Cleanup
    SDL_DestroyTexture(backgroundTexture);
    SDL_DestroyTexture(playerTexture);
    SDL_DestroyTexture(enemyTexture);
    SDL_DestroyTexture(bulletTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
