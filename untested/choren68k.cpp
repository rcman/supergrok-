#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

#define SCREEN_WIDTH 256   // X68000 resolution
#define SCREEN_HEIGHT 256
#define PLAYER_WIDTH 16
#define PLAYER_HEIGHT 16
#define ENEMY_WIDTH 16
#define ENEMY_HEIGHT 16
#define BULLET_SIZE 4
#define MAX_BULLETS 200
#define MAX_ENEMIES 50
#define TRIANGLE_SIZE 24
#define LOOP_COUNT 8   // Up to 8 loops as per original

typedef struct {
    float x, y;
    float dx, dy;
    int width, height;
    int powerLevel; // 0-4
    int bombs;      // Max 5
    bool shield;    // One-hit protection
} Player;

typedef struct {
    float x, y;
    int width, height;
    bool active;
    bool isBoss;
    int health;
} Enemy;

typedef struct {
    float x, y;
    bool active;
} Bullet;

typedef struct {
    float x, y;
    bool active;
    int type; // 0: Power-up, 1: Bomb, 2: Shield
} PowerUp;

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
    SDL_Window* window = SDL_CreateWindow("Cho Ren Sha 68K Clone",
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
    SDL_Texture* playerTex = IMG_LoadTexture(renderer, "player.png");     // Ship sprite
    SDL_Texture* enemyTex = IMG_LoadTexture(renderer, "enemy.png");       // Basic enemy
    SDL_Texture* bossTex = IMG_LoadTexture(renderer, "boss.png");         // Boss sprite
    SDL_Texture* bulletTex = IMG_LoadTexture(renderer, "bullet.png");     // Player bullet
    SDL_Texture* enemyBulletTex = IMG_LoadTexture(renderer, "enemy_bullet.png"); // Enemy bullet
    SDL_Texture* powerUpTex = IMG_LoadTexture(renderer, "powerup.png");   // Power-up triangle
    SDL_Texture* bgTex = IMG_LoadTexture(renderer, "ring_bg.png");        // Repeating ring background

    // Load sounds (placeholders)
    Mix_Chunk* shotSound = Mix_LoadWAV("shot.wav");
    Mix_Chunk* bombSound = Mix_LoadWAV("bomb.wav");
    Mix_Chunk* shieldSound = Mix_LoadWAV("shield.wav");
    Mix_Chunk* hitSound = Mix_LoadWAV("hit.wav");
    Mix_Music* bgMusic = Mix_LoadMUS("stage_music.mp3");

    // Game state
    Player player = {SCREEN_WIDTH / 2 - PLAYER_WIDTH / 2, SCREEN_HEIGHT - PLAYER_HEIGHT - 20, 0, 0, PLAYER_WIDTH, PLAYER_HEIGHT, 0, 3, false};
    Enemy enemies[MAX_ENEMIES];
    Bullet bullets[MAX_BULLETS];
    Bullet enemyBullets[MAX_BULLETS];
    PowerUp powerUps[3]; // Triangle: Power-up, Bomb, Shield
    int score = 0;
    int lives = 3;
    int loop = 1;
    int stage = 1;
    Uint32 lastShotTime = 0;
    Uint32 enemySpawnTimer = 0;
    float bgOffset = 0;
    bool running = true;
    SDL_Event event;

    // Initialize arrays
    for (int i = 0; i < MAX_ENEMIES; i++) enemies[i].active = false;
    for (int i = 0; i < MAX_BULLETS; i++) { bullets[i].active = false; enemyBullets[i].active = false; }
    for (int i = 0; i < 3; i++) powerUps[i].active = false;

    Mix_PlayMusic(bgMusic, -1);

    while (running) {
        // Event handling
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_RIGHT: player.dx = MOVE_SPEED; break;
                    case SDLK_LEFT: player.dx = -MOVE_SPEED; break;
                    case SDLK_UP: player.dy = -MOVE_SPEED; break;
                    case SDLK_DOWN: player.dy = MOVE_SPEED; break;
                    case SDLK_z: // Fire (semi-auto)
                        if (SDL_GetTicks() - lastShotTime > 100) { // ~10 shots/sec
                            for (int i = 0; i < MAX_BULLETS; i++) {
                                if (!bullets[i].active) {
                                    bullets[i].x = player.x + player.width / 2 - BULLET_SIZE / 2;
                                    bullets[i].y = player.y;
                                    bullets[i].active = true;
                                    Mix_PlayChannel(-1, shotSound, 0);
                                    lastShotTime = SDL_GetTicks();
                                    break;
                                }
                            }
                        }
                        break;
                    case SDLK_x: // Bomb
                        if (player.bombs > 0) {
                            player.bombs--;
                            Mix_PlayChannel(-1, bombSound, 0);
                            for (int i = 0; i < MAX_ENEMIES; i++) if (enemies[i].active) enemies[i].active = false; // Clear screen
                            for (int i = 0; i < MAX_BULLETS; i++) enemyBullets[i].active = false; // Clear bullets
                        }
                        break;
                }
            }
            if (event.type == SDL_KEYUP) {
                if (event.key.keysym.sym == SDLK_RIGHT || event.key.keysym.sym == SDLK_LEFT) player.dx = 0;
                if (event.key.keysym.sym == SDLK_UP || event.key.keysym.sym == SDLK_DOWN) player.dy = 0;
            }
        }

        // Update player
        player.x += player.dx;
        player.y += player.dy;
        if (player.x < 0) player.x = 0;
        if (player.x + player.width > SCREEN_WIDTH) player.x = SCREEN_WIDTH - player.width;
        if (player.y < 0) player.y = 0;
        if (player.y + player.height > SCREEN_HEIGHT) player.y = SCREEN_HEIGHT - player.height;

        // Update background (scrolling ring)
        bgOffset += 1.0f;
        if (bgOffset >= SCREEN_HEIGHT) bgOffset -= SCREEN_HEIGHT;

        // Spawn enemies
        if (SDL_GetTicks() - enemySpawnTimer > 1000) {
            for (int i = 0; i < MAX_ENEMIES; i++) {
                if (!enemies[i].active) {
                    enemies[i].x = rand() % (SCREEN_WIDTH - ENEMY_WIDTH);
                    enemies[i].y = -ENEMY_HEIGHT;
                    enemies[i].width = ENEMY_WIDTH;
                    enemies[i].height = ENEMY_HEIGHT;
                    enemies[i].active = true;
                    enemies[i].isBoss = (stage == 7 && i == 0); // Stage 7 boss
                    enemies[i].health = enemies[i].isBoss ? 20 : 1;
                    enemySpawnTimer = SDL_GetTicks();
                    break;
                }
            }
        }

        // Update enemies
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (enemies[i].active) {
                enemies[i].y += 1 + (loop - 1) * 0.5f; // Faster per loop
                if (enemies[i].y > SCREEN_HEIGHT) enemies[i].active = false;

                // Enemy shooting (simple straight bullets)
                if (rand() % 100 < 5 + loop) {
                    for (int j = 0; j < MAX_BULLETS; j++) {
                        if (!enemyBullets[j].active) {
                            enemyBullets[j].x = enemies[i].x + enemies[i].width / 2 - BULLET_SIZE / 2;
                            enemyBullets[j].y = enemies[i].y + enemies[i].height;
                            enemyBullets[j].active = true;
                            break;
                        }
                    }
                }

                // Collision with player
                SDL_Rect playerRect = {(int)player.x, (int)player.y, player.width, player.height};
                SDL_Rect enemyRect = {(int)enemies[i].x, (int)enemies[i].y, enemies[i].width, enemies[i].height};
                if (SDL_HasIntersection(&playerRect, &enemyRect)) {
                    if (player.shield) {
                        player.shield = false;
                        Mix_PlayChannel(-1, shieldSound, 0);
                        for (int j = 0; j < MAX_BULLETS; j++) enemyBullets[j].active = false; // Shockwave
                    } else {
                        lives--;
                        player.x = SCREEN_WIDTH / 2 - PLAYER_WIDTH / 2;
                        player.y = SCREEN_HEIGHT - PLAYER_HEIGHT - 20;
                        player.powerLevel = 0;
                        player.bombs = 3;
                        player.shield = false;
                        if (lives <= 0) running = false;
                    }
                    enemies[i].active = false;
                }
            }
        }

        // Update bullets
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                bullets[i].y -= 5;
                if (bullets[i].y < -BULLET_SIZE) bullets[i].active = false;

                SDL_Rect bulletRect = {(int)bullets[i].x, (int)bullets[i].y, BULLET_SIZE, BULLET_SIZE};
                for (int j = 0; j < MAX_ENEMIES; j++) {
                    if (enemies[j].active && SDL_HasIntersection(&bulletRect, &(SDL_Rect){(int)enemies[j].x, (int)enemies[j].y, enemies[j].width, enemies[j].height})) {
                        bullets[i].active = false;
                        enemies[j].health--;
                        if (enemies[j].health <= 0) {
                            enemies[j].active = false;
                            score += 10;
                            if (loop > 1) { // Revenge bullets from loop 2
                                for (int k = 0; k < MAX_BULLETS; k++) {
                                    if (!enemyBullets[k].active) {
                                        enemyBullets[k].x = enemies[j].x + enemies[j].width / 2 - BULLET_SIZE / 2;
                                        enemyBullets[k].y = enemies[j].y + enemies[j].height / 2;
                                        enemyBullets[k].active = true;
                                        break;
                                    }
                                }
                            }
                            if (enemies[j].isBoss && stage == 7) {
                                stage = 0; // Stage 0 after loop 1
                                loop++;
                                if (loop > LOOP_COUNT) running = false; // True ending after loop 8
                            } else if (!enemies[j].isBoss) {
                                // Spawn power-up triangle
                                for (int k = 0; k < 3; k++) {
                                    if (!powerUps[k].active) {
                                        powerUps[k].x = enemies[j].x + enemies[j].width / 2 - TRIANGLE_SIZE / 2;
                                        powerUps[k].y = enemies[j].y + enemies[j].height / 2 - TRIANGLE_SIZE / 2;
                                        powerUps[k].active = true;
                                        powerUps[k].type = k; // 0: Power, 1: Bomb, 2: Shield
                                    }
                                }
                            }
                        }
                        Mix_PlayChannel(-1, hitSound, 0);
                        break;
                    }
                }
            }
            if (enemyBullets[i].active) {
                enemyBullets[i].y += 3 + (loop - 1); // Faster per loop
                if (enemyBullets[i].y > SCREEN_HEIGHT) enemyBullets[i].active = false;

                SDL_Rect enemyBulletRect = {(int)enemyBullets[i].x, (int)enemyBullets[i].y, BULLET_SIZE, BULLET_SIZE};
                if (SDL_HasIntersection(&enemyBulletRect, &(SDL_Rect){(int)player.x, (int)player.y, player.width, player.height})) {
                    enemyBullets[i].active = false;
                    if (player.shield) {
                        player.shield = false;
                        Mix_PlayChannel(-1, shieldSound, 0);
                        for (int j = 0; j < MAX_BULLETS; j++) enemyBullets[j].active = false;
                    } else {
                        lives--;
                        player.x = SCREEN_WIDTH / 2 - PLAYER_WIDTH / 2;
                        player.y = SCREEN_HEIGHT - PLAYER_HEIGHT - 20;
                        player.powerLevel = 0;
                        player.bombs = 3;
                        player.shield = false;
                        if (lives <= 0) running = false;
                    }
                }
            }
        }

        // Update power-ups
        for (int i = 0; i < 3; i++) {
            if (powerUps[i].active) {
                SDL_Rect powerUpRect = {(int)powerUps[i].x, (int)powerUps[i].y, TRIANGLE_SIZE, TRIANGLE_SIZE};
                SDL_Rect playerRect = {(int)player.x, (int)player.y, player.width, player.height};
                if (SDL_HasIntersection(&playerRect, &powerUpRect)) {
                    switch (powerUps[i].type) {
                        case 0: if (player.powerLevel < 4) player.powerLevel++; break; // Power-up
                        case 1: if (player.bombs < 5) player.bombs++; break;         // Bomb
                        case 2: if (!player.shield) player.shield = true; break;    // Shield
                    }
                    powerUps[i].active = false;
                    if (i == 0 && powerUps[1].active && powerUps[2].active) { // All three collected
                        player.powerLevel = (player.powerLevel < 4) ? player.powerLevel + 1 : 4;
                        player.bombs = (player.bombs < 5) ? player.bombs + 1 : 5;
                        player.shield = true;
                        powerUps[1].active = false;
                        powerUps[2].active = false;
                    }
                }
                // Despawn if off-screen
                if (powerUps[i].y > SCREEN_HEIGHT) powerUps[i].active = false;
            }
        }

        // Rendering
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Draw background (repeating ring)
        SDL_Rect bgRect1 = {0, (int)bgOffset - SCREEN_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_Rect bgRect2 = {0, (int)bgOffset, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_RenderCopy(renderer, bgTex, NULL, &bgRect1);
        SDL_RenderCopy(renderer, bgTex, NULL, &bgRect2);

        // Draw power-ups (triangle formation)
        for (int i = 0; i < 3; i++) {
            if (powerUps[i].active) {
                float angle = 120 * i * M_PI / 180;
                SDL_Rect powerUpRect = {(int)(powerUps[i].x + cos(angle) * TRIANGLE_SIZE / 2), (int)(powerUps[i].y + sin(angle) * TRIANGLE_SIZE / 2), 8, 8};
                SDL_RenderCopy(renderer, powerUpTex, NULL, &powerUpRect);
            }
        }

        // Draw enemies
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (enemies[i].active) {
                SDL_Rect enemyRect = {(int)enemies[i].x, (int)enemies[i].y, enemies[i].width, enemies[i].height};
                SDL_RenderCopy(renderer, enemies[i].isBoss ? bossTex : enemyTex, NULL, &enemyRect);
            }
        }

        // Draw bullets
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                SDL_Rect bulletRect = {(int)bullets[i].x, (int)bullets[i].y, BULLET_SIZE, BULLET_SIZE};
                SDL_RenderCopy(renderer, bulletTex, NULL, &bulletRect);
            }
            if (enemyBullets[i].active) {
                SDL_Rect enemyBulletRect = {(int)enemyBullets[i].x, (int)enemyBullets[i].y, BULLET_SIZE, BULLET_SIZE};
                SDL_RenderCopy(renderer, enemyBulletTex, NULL, &enemyBulletRect);
            }
        }

        // Draw player
        SDL_Rect playerRect = {(int)player.x, (int)player.y, player.width, player.height};
        SDL_RenderCopy(renderer, playerTex, NULL, &playerRect);

        SDL_RenderPresent(renderer);
        SDL_Delay(18); // ~55 FPS for X68000 authenticity
    }

    // Cleanup
    Mix_FreeChunk(shotSound);
    Mix_FreeChunk(bombSound);
    Mix_FreeChunk(shieldSound);
    Mix_FreeChunk(hitSound);
    Mix_FreeMusic(bgMusic);
    SDL_DestroyTexture(playerTex);
    SDL_DestroyTexture(enemyTex);
    SDL_DestroyTexture(bossTex);
    SDL_DestroyTexture(bulletTex);
    SDL_DestroyTexture(enemyBulletTex);
    SDL_DestroyTexture(powerUpTex);
    SDL_DestroyTexture(bgTex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_Quit();
    IMG_Quit();
    SDL_Quit();

    printf("Game Over! Final Score: %d, Loop: %d, Stage: %d\n", score, loop, stage);
    return 0;
}
