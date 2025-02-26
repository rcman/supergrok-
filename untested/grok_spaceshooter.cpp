#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
int PLAYER_SPEED = 6;
#define BULLET_SPEED 10
#define ENEMY_SPEED 4
#define ENEMY_BULLET_SPEED 6
#define BG_SPEED 3
#define SPAWN_INTERVAL 1000 // 1 second
#define MAX_ENEMIES 15
#define MAX_BULLETS 50
#define MAX_ENEMY_BULLETS 50
#define MAX_POWERUPS 10

// Player structure
typedef struct {
    int x, y;
    SDL_Texture* texture;
    int bullets;      // Number of bullets fired at once
    Uint32 fireRate;  // Milliseconds between shots
    bool shield;      // Shield active
    Uint32 lastShot;  // Time of last shot
} Player;

// Enemy structure
typedef struct {
    int x, y;
    SDL_Texture* texture;
    bool active;
    Uint32 lastShot;  // Time of last shot
} Enemy;

// Bullet structure
typedef struct {
    int x, y;
    bool active;
} Bullet;

// Power-up structure
typedef struct {
    int x, y;
    SDL_Texture* texture;
    int type; // 0: bullets, 1: firerate, 2: shield, 3: nuke, 4: speed
    bool active;
} PowerUp;

// Load texture helper
SDL_Texture* loadTexture(SDL_Renderer* renderer, const char* file) {
    SDL_Surface* surface = IMG_Load(file);
    if (!surface) {
        printf("Failed to load %s! Error: %s\n", file, IMG_GetError());
        return NULL;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

int main(int argc, char* argv[]) {
    srand(time(NULL)); // Seed random

    // Initialize SDL and SDL_image
    if (SDL_Init(SDL_INIT_VIDEO) < 0 || IMG_Init(IMG_INIT_PNG) == 0) {
        printf("SDL failed! Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Vertical Space Shooter", SDL_WINDOWPOS_UNDEFINED, 
                                          SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, 
                                          SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!window || !renderer) {
        printf("Window/Renderer failed! Error: %s\n", SDL_GetError());
        return 1;
    }

    // Load textures
    SDL_Texture* bgTexture = loadTexture(renderer, "background.png");
    SDL_Texture* playerTexture = loadTexture(renderer, "player_ship.png");
    SDL_Texture* enemyTexture = loadTexture(renderer, "enemy_ship.png");
    SDL_Texture* powerTextures[5] = {
        loadTexture(renderer, "powerup_bullets.png"),  // Extra bullets
        loadTexture(renderer, "powerup_firerate.png"), // Faster fire rate
        loadTexture(renderer, "powerup_shield.png"),   // Shield
        loadTexture(renderer, "powerup_nuke.png"),     // Nuke
        loadTexture(renderer, "powerup_speed.png")     // Speed boost
    };
    if (!bgTexture || !playerTexture || !enemyTexture || !powerTextures[0] || 
        !powerTextures[1] || !powerTextures[2] || !powerTextures[3] || !powerTextures[4]) {
        return 1;
    }

    // Game objects
    Player player = {SCREEN_WIDTH / 2 - 32, SCREEN_HEIGHT - 100, playerTexture, 1, 500, false, 0};
    Enemy enemies[MAX_ENEMIES] = {0};
    Bullet bullets[MAX_BULLETS] = {0};
    Bullet enemyBullets[MAX_ENEMY_BULLETS] = {0};
    PowerUp powerUps[MAX_POWERUPS] = {0};
    int bulletCount = 0, enemyBulletCount = 0, enemyCount = 0, powerUpCount = 0;
    int bgY = 0;
    Uint32 lastSpawn = 0;
    bool running = true;
    SDL_Event e;

    // Game loop
    while (running) {
        // Events
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) running = false;
        }

        // Input
        const Uint8* keys = SDL_GetKeyboardState(NULL);
        if (keys[SDL_SCANCODE_W] && player.y > 0) player.y -= PLAYER_SPEED;
        if (keys[SDL_SCANCODE_S] && player.y < SCREEN_HEIGHT - 64) player.y += PLAYER_SPEED;
        if (keys[SDL_SCANCODE_A] && player.x > 0) player.x -= PLAYER_SPEED;
        if (keys[SDL_SCANCODE_D] && player.x < SCREEN_WIDTH - 64) player.x += PLAYER_SPEED;

        // Player shooting
        Uint32 now = SDL_GetTicks();
        if (keys[SDL_SCANCODE_SPACE] && now - player.lastShot >= player.fireRate && bulletCount < MAX_BULLETS) {
            for (int i = 0; i < player.bullets && bulletCount < MAX_BULLETS; i++) {
                bullets[bulletCount] = (Bullet){player.x + 32 - (player.bullets - 1) * 10 + i * 20, player.y, true};
                bulletCount++;
            }
            player.lastShot = now;
        }

        // Update bullets
        for (int i = 0; i < bulletCount; i++) {
            if (bullets[i].active) {
                bullets[i].y -= BULLET_SPEED;
                if (bullets[i].y < -10) bullets[i].active = false;
            }
        }

        // Enemy spawning
        if (now - lastSpawn >= SPAWN_INTERVAL && enemyCount < MAX_ENEMIES) {
            for (int i = 0; i < MAX_ENEMIES; i++) {
                if (!enemies[i].active) {
                    enemies[i] = (Enemy){rand() % (SCREEN_WIDTH - 64), -64, enemyTexture, true, now};
                    enemyCount++;
                    lastSpawn = now;
                    break;
                }
            }
        }

        // Update enemies and enemy bullets
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (enemies[i].active) {
                enemies[i].y += ENEMY_SPEED;
                if (enemies[i].y > SCREEN_HEIGHT) {
                    enemies[i].active = false;
                    enemyCount--;
                }
                // Enemy shooting
                if (now - enemies[i].lastShot >= 2000 && enemyBulletCount < MAX_ENEMY_BULLETS) { // Every 2s
                    enemyBullets[enemyBulletCount] = (Bullet){enemies[i].x + 32, enemies[i].y + 64, true};
                    enemyBulletCount++;
                    enemies[i].lastShot = now;
                }
            }
        }
        for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
            if (enemyBullets[i].active) {
                enemyBullets[i].y += ENEMY_BULLET_SPEED;
                if (enemyBullets[i].y > SCREEN_HEIGHT) enemyBullets[i].active = false;
            }
        }

        // Collision detection
        SDL_Rect playerRect = {player.x, player.y, 64, 64};
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (enemies[i].active) {
                SDL_Rect enemyRect = {enemies[i].x, enemies[i].y, 64, 64};
                for (int j = 0; j < bulletCount; j++) {
                    if (bullets[j].active) {
                        SDL_Rect bulletRect = {bullets[j].x, bullets[j].y, 10, 5};
                        if (SDL_HasIntersection(&enemyRect, &bulletRect)) {
                            enemies[i].active = false;
                            bullets[j].active = false;
                            enemyCount--;
                            // Random power-up drop (20% chance)
                            if (rand() % 5 == 0 && powerUpCount < MAX_POWERUPS) {
                                powerUps[powerUpCount] = (PowerUp){enemies[i].x, enemies[i].y, 
                                    powerTextures[rand() % 5], rand() % 5, true};
                                powerUpCount++;
                            }
                        }
                    }
                }
            }
        }
        for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
            if (enemyBullets[i].active) {
                SDL_Rect bulletRect = {enemyBullets[i].x, enemyBullets[i].y, 10, 5};
                if (SDL_HasIntersection(&playerRect, &bulletRect) && !player.shield) {
                    running = false; // Game over (no lives system yet)
                }
                if (player.shield) enemyBullets[i].active = false;
            }
        }

        // Power-up collection
        for (int i = 0; i < MAX_POWERUPS; i++) {
            if (powerUps[i].active) {
                powerUps[i].y += 2; // Fall slowly
                SDL_Rect powerRect = {powerUps[i].x, powerUps[i].y, 32, 32};
                if (SDL_HasIntersection(&playerRect, &powerRect)) {
                    switch (powerUps[i].type) {
                        case 0: player.bullets = (player.bullets < 5) ? player.bullets + 1 : 5; break; // Max 5 bullets
                        case 1: player.fireRate = (player.fireRate > 100) ? player.fireRate - 100 : 100; break; // Min 100ms
                        case 2: player.shield = true; break;
                        case 3: // Nuke
                            for (int j = 0; j < MAX_ENEMIES; j++) {
                                if (enemies[j].active) {
                                    enemies[j].active = false;
                                    enemyCount--;
                                }
                            }
                            break;
                        case 4: PLAYER_SPEED = (PLAYER_SPEED < 10) ? PLAYER_SPEED + 2 : 10; break; // Max 10 speed
                    }
                    powerUps[i].active = false;
                    powerUpCount--;
                }
                if (powerUps[i].y > SCREEN_HEIGHT) {
                    powerUps[i].active = false;
                    powerUpCount--;
                }
            }
        }

        // Scroll background
        bgY += BG_SPEED;
        if (bgY >= SCREEN_HEIGHT) bgY = 0;

        // Render
        SDL_RenderClear(renderer);
        SDL_Rect bgRect1 = {0, bgY - SCREEN_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_Rect bgRect2 = {0, bgY, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_RenderCopy(renderer, bgTexture, NULL, &bgRect1);
        SDL_RenderCopy(renderer, bgTexture, NULL, &bgRect2);

        SDL_Rect playerRectRender = {player.x, player.y, 128, 128};
        SDL_RenderCopy(renderer, player.texture, NULL, &playerRectRender);

        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (enemies[i].active) {
                SDL_Rect enemyRect = {enemies[i].x, enemies[i].y, 128, 128};
                SDL_RenderCopy(renderer, enemyTexture, NULL, &enemyRect);
            }
        }

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White bullets
        for (int i = 0; i < bulletCount; i++) {
            if (bullets[i].active) {
                SDL_Rect bulletRect = {bullets[i].x, bullets[i].y, 10, 5};
                SDL_RenderFillRect(renderer, &bulletRect);
            }
        }
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red enemy bullets
        for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
            if (enemyBullets[i].active) {
                SDL_Rect bulletRect = {enemyBullets[i].x, enemyBullets[i].y, 10, 5};
                SDL_RenderFillRect(renderer, &bulletRect);
            }
        }

        for (int i = 0; i < MAX_POWERUPS; i++) {
            if (powerUps[i].active) {
                SDL_Rect powerRect = {powerUps[i].x, powerUps[i].y, 63, 64};
                SDL_RenderCopy(renderer, powerUps[i].texture, NULL, &powerRect);
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60 FPS
    }

    // Cleanup
    SDL_DestroyTexture(bgTexture);
    SDL_DestroyTexture(playerTexture);
    SDL_DestroyTexture(enemyTexture);
    for (int i = 0; i < 5; i++) SDL_DestroyTexture(powerTextures[i]);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
