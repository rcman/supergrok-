#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define BLOCK_SIZE 64
#define GRAVITY 0.5f
#define JUMP_FORCE -12.0f
#define MOVE_SPEED 4.0f
#define BULLET_SPEED 8.0f
#define MAX_BULLETS 100
#define MAP_WIDTH (SCREEN_WIDTH / BLOCK_SIZE)
#define MAP_HEIGHT (SCREEN_HEIGHT / BLOCK_SIZE)

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
    char tiles[MAP_HEIGHT][MAP_WIDTH];
} Map;

// Load texture helper function with SDL2_image
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

bool loadMap(Map* map, Player* player, Enemy* enemies, int* numEnemies) {
    FILE* file = fopen("map.txt", "r");
    if (!file) {
        printf("Failed to open map.txt\n");
        return false;
    }

    *numEnemies = 0;
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            char c = fgetc(file);
            if (c == EOF) break;
            if (c == '\n') { x--; continue; }
            
            map->tiles[y][x] = c;
            if (c == 'P') {
                player->x = x * BLOCK_SIZE;
                player->y = y * BLOCK_SIZE;
            }
            if (c == 'E' && *numEnemies < 10) {
                enemies[*numEnemies].x = x * BLOCK_SIZE;
                enemies[*numEnemies].y = y * BLOCK_SIZE;
                enemies[*numEnemies].active = true;
                (*numEnemies)++;
            }
        }
    }
    fclose(file);
    return true;
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0 || IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG) {
        printf("SDL/IMG Init Error: %s\n", SDL_GetError());
        return 1;
    }
    
    SDL_Window* window = SDL_CreateWindow("Platform Game",
                                        SDL_WINDOWPOS_UNDEFINED,
                                        SDL_WINDOWPOS_UNDEFINED,
                                        SCREEN_WIDTH, SCREEN_HEIGHT,
                                        SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Window creation error: %s\n", SDL_GetError());
        return 1;
    }
    
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Renderer creation error: %s\n", SDL_GetError());
        return 1;
    }

    // Load textures
    SDL_Texture* blockTexture = loadTexture(renderer, "block.png");
    SDL_Texture* playerTexture = loadTexture(renderer, "player.png");
    SDL_Texture* enemyTexture = loadTexture(renderer, "enemy.png");
    SDL_Texture* bulletTexture = loadTexture(renderer, "bullet.png");
    
    if (!blockTexture || !playerTexture || !enemyTexture || !bulletTexture) {
        SDL_Quit();
        IMG_Quit();
        return 1;
    }

    // Game objects
    Map map = {0};
    Player player = {0};
    Enemy enemies[10] = {0};
    int numEnemies = 0;
    Bullet playerBullets[MAX_BULLETS] = {0};
    Bullet enemyBullets[MAX_BULLETS] = {0};
    
    if (!loadMap(&map, &player, enemies, &numEnemies)) {
        SDL_Quit();
        IMG_Quit();
        return 1;
    }

    bool running = true;
    Uint32 lastShotTime = 0;
    Uint32 lastEnemyShotTime = 0;
    const Uint32 shotDelay = 200;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
        }

        const Uint8* keys = SDL_GetKeyboardState(NULL);
        
        player.dx = 0;
        if (keys[SDL_SCANCODE_LEFT]) player.dx = -MOVE_SPEED;
        if (keys[SDL_SCANCODE_RIGHT]) player.dx = MOVE_SPEED;
        if (keys[SDL_SCANCODE_SPACE] && player.onGround) {
            player.dy = JUMP_FORCE;
            player.onGround = false;
        }

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

        player.dy += GRAVITY;
        player.x += player.dx;
        player.y += player.dy;
        
        player.onGround = false;
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                if (map.tiles[y][x] == '#') {
                    SDL_Rect blockRect = {x * BLOCK_SIZE, y * BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE};
                    SDL_Rect playerRect = {(int)player.x, (int)player.y, BLOCK_SIZE, BLOCK_SIZE};
                    
                    if (SDL_HasIntersection(&playerRect, &blockRect)) {
                        if (player.dy > 0 && player.y + BLOCK_SIZE - player.dy <= blockRect.y) {
                            player.y = blockRect.y - BLOCK_SIZE;
                            player.dy = 0;
                            player.onGround = true;
                        }
                        else if (player.dy < 0 && player.y - player.dy >= blockRect.y + BLOCK_SIZE) {
                            player.y = blockRect.y + BLOCK_SIZE;
                            player.dy = 0;
                        }
                        else if (player.dx > 0) player.x = blockRect.x - BLOCK_SIZE;
                        else if (player.dx < 0) player.x = blockRect.x + BLOCK_SIZE;
                    }
                }
            }
        }

        for (int i = 0; i < MAX_BULLETS; i++) {
            if (playerBullets[i].active) {
                playerBullets[i].x += playerBullets[i].dx;
                if (playerBullets[i].x > SCREEN_WIDTH) playerBullets[i].active = false;
                
                for (int j = 0; j < numEnemies; j++) {
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

        if (currentTime - lastEnemyShotTime > shotDelay * 2) {
            for (int i = 0; i < numEnemies; i++) {
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
        
        // Draw map
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                if (map.tiles[y][x] == '#') {
                    SDL_Rect rect = {x * BLOCK_SIZE, y * BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE};
                    SDL_RenderCopy(renderer, blockTexture, NULL, &rect);
                }
            }
        }
        
        // Draw player
        SDL_Rect playerRect = {(int)player.x, (int)player.y, BLOCK_SIZE, BLOCK_SIZE};
        SDL_RenderCopy(renderer, playerTexture, NULL, &playerRect);
        
        // Draw enemies
        for (int i = 0; i < numEnemies; i++) {
            if (enemies[i].active) {
                SDL_Rect rect = {(int)enemies[i].x, (int)enemies[i].y, BLOCK_SIZE, BLOCK_SIZE};
                SDL_RenderCopy(renderer, enemyTexture, NULL, &rect);
            }
        }
        
        // Draw bullets
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (playerBullets[i].active) {
                SDL_Rect rect = {(int)playerBullets[i].x, (int)playerBullets[i].y, 8, 8};
                SDL_RenderCopy(renderer, bulletTexture, NULL, &rect);
            }
            if (enemyBullets[i].active) {
                SDL_Rect rect = {(int)enemyBullets[i].x, (int)enemyBullets[i].y, 8, 8};
                SDL_RenderCopy(renderer, bulletTexture, NULL, &rect);
            }
        }
        
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    // Cleanup
    SDL_DestroyTexture(blockTexture);
    SDL_DestroyTexture(playerTexture);
    SDL_DestroyTexture(enemyTexture);
    SDL_DestroyTexture(bulletTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
