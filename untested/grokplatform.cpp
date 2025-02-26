#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdio.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define BLOCK_SIZE 64
#define GRAVITY 0.5f
#define JUMP_FORCE -12.0f
#define MOVE_SPEED 4.0f
#define BULLET_SPEED 8.0f
#define MAX_BULLETS 100

typedef struct {
    float x, y;
    float dx, dy;
    bool onGround;
} Player;

typedef struct {
    float x, y;
    bool active;
} Enemy;

typedef struct {
    float x, y;
    float dx, dy;
    bool active;
} Bullet;

typedef struct {
    int x, y;
} Block;

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    
    SDL_Window* window = SDL_CreateWindow("Platform Game",
                                        SDL_WINDOWPOS_UNDEFINED,
                                        SDL_WINDOWPOS_UNDEFINED,
                                        SCREEN_WIDTH, SCREEN_HEIGHT,
                                        SDL_WINDOW_SHOWN);
    
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // Player initialization
    Player player = {100, 100, 0, 0, false};
    
    // Enemies
    Enemy enemies[3] = {
        {300, 100, true},
        {400, 100, true},
        {500, 100, true}
    };
    
    // Bullets
    Bullet playerBullets[MAX_BULLETS] = {0};
    Bullet enemyBullets[MAX_BULLETS] = {0};
    
    // Blocks (simple level design)
    Block blocks[] = {
        {0, 500}, {64, 500}, {128, 500}, {192, 500}, {256, 500}, // Floor
        {320, 436}, {384, 436}, // Platform
        {448, 372} // Higher platform
    };
    int numBlocks = sizeof(blocks) / sizeof(blocks[0]);
    
    bool running = true;
    Uint32 lastShotTime = 0;
    Uint32 lastEnemyShotTime = 0;
    const Uint32 shotDelay = 200; // milliseconds between shots
    
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
        }

        // Input handling
        const Uint8* keys = SDL_GetKeyboardState(NULL);
        
        // Player movement
        player.dx = 0;
        if (keys[SDL_SCANCODE_LEFT]) player.dx = -MOVE_SPEED;
        if (keys[SDL_SCANCODE_RIGHT]) player.dx = MOVE_SPEED;
        if (keys[SDL_SCANCODE_SPACE] && player.onGround) {
            player.dy = JUMP_FORCE;
            player.onGround = false;
        }
        
        // Shooting
        Uint32 currentTime = SDL_GetTicks();
        if (keys[SDL_SCANCODE_Z] && (currentTime - lastShotTime) > shotDelay) {
            for (int i = 0; i < MAX_BULLETS; i++) {
                if (!playerBullets[i].active) {
                    playerBullets[i] = (Bullet){player.x + BLOCK_SIZE, player.y, BULLET_SPEED, 0, true};
                    lastShotTime = currentTime;
                    break;
                }
            }
        }

        // Apply gravity
        player.dy += GRAVITY;
        
        // Update player position
        player.x += player.dx;
        player.y += player.dy;
        
        // Collision detection with blocks
        player.onGround = false;
        for (int i = 0; i < numBlocks; i++) {
            SDL_Rect blockRect = {blocks[i].x, blocks[i].y, BLOCK_SIZE, BLOCK_SIZE};
            SDL_Rect playerRect = {(int)player.x, (int)player.y, BLOCK_SIZE, BLOCK_SIZE};
            
            if (SDL_HasIntersection(&playerRect, &blockRect)) {
                // Vertical collision
                if (player.dy > 0 && player.y + BLOCK_SIZE - player.dy <= blockRect.y) {
                    player.y = blockRect.y - BLOCK_SIZE;
                    player.dy = 0;
                    player.onGround = true;
                }
                else if (player.dy < 0 && player.y - player.dy >= blockRect.y + BLOCK_SIZE) {
                    player.y = blockRect.y + BLOCK_SIZE;
                    player.dy = 0;
                }
                // Horizontal collision
                else if (player.dx > 0) player.x = blockRect.x - BLOCK_SIZE;
                else if (player.dx < 0) player.x = blockRect.x + BLOCK_SIZE;
            }
        }

        // Update bullets
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (playerBullets[i].active) {
                playerBullets[i].x += playerBullets[i].dx;
                if (playerBullets[i].x > SCREEN_WIDTH) playerBullets[i].active = false;
                
                // Bullet-enemy collision
                for (int j = 0; j < 3; j++) {
                    if (enemies[j].active) {
                        SDL_Rect enemyRect = {(int)enemies[j].x, (int)enemies[j].y, BLOCK_SIZE, BLOCK_SIZE};
                        SDL_Rect bulletRect = {(int)playerBullets[i].x, (int)playerBullets[i].y, 8, 8};
                        if (SDL_HasIntersection(&enemyRect, &bulletRect)) {
                            playerBullets[i].active = false;
                            enemies[j].active = false;
                        }
                    }
                }
            }
            
            if (enemyBullets[i].active) {
                enemyBullets[i].x += enemyBullets[i].dx;
                if (enemyBullets[i].x < 0 || enemyBullets[i].x > SCREEN_WIDTH)
                    enemyBullets[i].active = false;
            }
        }

        // Enemy shooting
        if (currentTime - lastEnemyShotTime > shotDelay * 2) {
            for (int i = 0; i < 3; i++) {
                if (enemies[i].active) {
                    for (int j = 0; j < MAX_BULLETS; j++) {
                        if (!enemyBullets[j].active) {
                            enemyBullets[j] = (Bullet){enemies[i].x, enemies[i].y, -BULLET_SPEED, 0, true};
                            lastEnemyShotTime = currentTime;
                            break;
                        }
                    }
                }
            }
        }

        // Rendering
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        // Draw blocks
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        for (int i = 0; i < numBlocks; i++) {
            SDL_Rect rect = {blocks[i].x, blocks[i].y, BLOCK_SIZE, BLOCK_SIZE};
            SDL_RenderFillRect(renderer, &rect);
        }
        
        // Draw player
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_Rect playerRect = {(int)player.x, (int)player.y, BLOCK_SIZE, BLOCK_SIZE};
        SDL_RenderFillRect(renderer, &playerRect);
        
        // Draw enemies
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        for (int i = 0; i < 3; i++) {
            if (enemies[i].active) {
                SDL_Rect rect = {(int)enemies[i].x, (int)enemies[i].y, BLOCK_SIZE, BLOCK_SIZE};
                SDL_RenderFillRect(renderer, &rect);
            }
        }
        
        // Draw bullets
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (playerBullets[i].active) {
                SDL_Rect rect = {(int)playerBullets[i].x, (int)playerBullets[i].y, 8, 8};
                SDL_RenderFillRect(renderer, &rect);
            }
            if (enemyBullets[i].active) {
                SDL_Rect rect = {(int)enemyBullets[i].x, (int)enemyBullets[i].y, 8, 8};
                SDL_RenderFillRect(renderer, &rect);
            }
        }
        
        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60 FPS
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
