#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdbool.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define TILE_SIZE 32
#define MAP_WIDTH 25
#define MAP_HEIGHT 19
#define GRAVITY 0.5f
#define JUMP_FORCE -10.0f
#define MOVE_SPEED 3.0f
#define BULLET_SPEED 5.0f

typedef enum {
    IDLE,
    WALKING,
    JUMPING,
    CROUCHING,
    CLIMBING
} PlayerState;

typedef struct {
    float x, y;
    float velX, velY;
    PlayerState state;
    bool facingRight;
    int health;
    SDL_Texture* texture;
} Player;

typedef struct {
    float x, y;
    bool active;
    int health;
    SDL_Texture* texture;
} Enemy;

typedef struct {
    float x, y;
    float velX;
    bool active;
    SDL_Texture* texture;
} Bullet;

typedef struct {
    int tiles[MAP_HEIGHT][MAP_WIDTH];
    SDL_Texture* blockTexture;
    SDL_Texture* ladderTexture;
} Map;

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
Player player;
Enemy enemies[10];
Bullet bullets[20];
Map gameMap;

SDL_Texture* loadTexture(const char* path) {
    SDL_Surface* surface = SDL_LoadBMP(path);
    if (!surface) {
        printf("Error loading BMP: %s\n", SDL_GetError());
        return NULL;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

bool initialize() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return false;
    
    window = SDL_CreateWindow("2D Platformer", 
                            SDL_WINDOWPOS_UNDEFINED, 
                            SDL_WINDOWPOS_UNDEFINED, 
                            SCREEN_WIDTH, 
                            SCREEN_HEIGHT, 
                            SDL_WINDOW_SHOWN);
    if (!window) return false;

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) return false;

    // Load textures
    player.texture = loadTexture("player.bmp");
    gameMap.blockTexture = loadTexture("block.bmp");
    gameMap.ladderTexture = loadTexture("ladder.bmp");
    
    for (int i = 0; i < 10; i++) {
        enemies[i].texture = loadTexture("enemy.bmp");
    }
    
    for (int i = 0; i < 20; i++) {
        bullets[i].texture = loadTexture("bullet.bmp");
    }

    return true;
}

void loadMap(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Could not load map file!\n");
        return;
    }

    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            fscanf(file, "%d", &gameMap.tiles[y][x]);
            if (gameMap.tiles[y][x] == 3) { // 3 spawns enemies
                enemies[0].x = x * TILE_SIZE;
                enemies[0].y = y * TILE_SIZE;
                enemies[0].active = true;
                enemies[0].health = 100;
                gameMap.tiles[y][x] = 0;
            }
        }
    }
    fclose(file);
}

bool checkCollision(float x, float y) {
    int tileX = x / TILE_SIZE;
    int tileY = y / TILE_SIZE;
    
    if (tileX < 0 || tileX >= MAP_WIDTH || tileY < 0 || tileY >= MAP_HEIGHT)
        return true;
    
    return gameMap.tiles[tileY][tileX] == 1;
}

void shootBullet() {
    for (int i = 0; i < 20; i++) {
        if (!bullets[i].active) {
            bullets[i].x = player.x;
            bullets[i].y = player.y;
            bullets[i].velX = player.facingRight ? BULLET_SPEED : -BULLET_SPEED;
            bullets[i].active = true;
            break;
        }
    }
}

void update() {
    // Player physics
    if (player.state != CLIMBING) {
        player.velY += GRAVITY;
    }
    player.x += player.velX;
    player.y += player.velY;

    // Player collision
    if (checkCollision(player.x, player.y + TILE_SIZE) && player.velY > 0) {
        player.y = ((int)(player.y + TILE_SIZE) / TILE_SIZE) * TILE_SIZE - TILE_SIZE;
        player.velY = 0;
        if (player.state == JUMPING) player.state = IDLE;
    }

    // Ladder collision
    int tileX = player.x / TILE_SIZE;
    int tileY = player.y / TILE_SIZE;
    if (gameMap.tiles[tileY][tileX] == 2) {
        player.state = CLIMBING;
    }

    // Update bullets
    for (int i = 0; i < 20; i++) {
        if (bullets[i].active) {
            bullets[i].x += bullets[i].velX;
            if (bullets[i].x < 0 || bullets[i].x > SCREEN_WIDTH) {
                bullets[i].active = false;
            }
        }
    }
}

void render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Render map
    SDL_Rect rect = {0, 0, TILE_SIZE, TILE_SIZE};
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            rect.x = x * TILE_SIZE;
            rect.y = y * TILE_SIZE;
            if (gameMap.tiles[y][x] == 1) {
                SDL_RenderCopy(renderer, gameMap.blockTexture, NULL, &rect);
            }
            else if (gameMap.tiles[y][x] == 2) {
                SDL_RenderCopy(renderer, gameMap.ladderTexture, NULL, &rect);
            }
        }
    }

    // Render player
    SDL_Rect playerRect = {(int)player.x, (int)player.y, TILE_SIZE, TILE_SIZE};
    SDL_RenderCopy(renderer, player.texture, NULL, &playerRect);

    // Render enemies
    for (int i = 0; i < 10; i++) {
        if (enemies[i].active) {
            SDL_Rect enemyRect = {(int)enemies[i].x, (int)enemies[i].y, TILE_SIZE, TILE_SIZE};
            SDL_RenderCopy(renderer, enemies[i].texture, NULL, &enemyRect);
        }
    }

    // Render bullets
    for (int i = 0; i < 20; i++) {
        if (bullets[i].active) {
            SDL_Rect bulletRect = {(int)bullets[i].x, (int)bullets[i].y, TILE_SIZE, TILE_SIZE};
            SDL_RenderCopy(renderer, bullets[i].texture, NULL, &bulletRect);
        }
    }

    SDL_RenderPresent(renderer);
}

int main() {
    if (!initialize()) return 1;

    // Initialize player
    player.x = 100;
    player.y = 100;
    player.health = 100;

    loadMap("map.txt");

    bool quit = false;
    SDL_Event e;

    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) quit = true;

            const Uint8* keyboard = SDL_GetKeyboardState(NULL);
            
            player.velX = 0;
            if (keyboard[SDL_SCANCODE_LEFT]) {
                player.velX = -MOVE_SPEED;
                player.facingRight = false;
                if (player.state != JUMPING && player.state != CLIMBING)
                    player.state = WALKING;
            }
            if (keyboard[SDL_SCANCODE_RIGHT]) {
                player.velX = MOVE_SPEED;
                player.facingRight = true;
                if (player.state != JUMPING && player.state != CLIMBING)
                    player.state = WALKING;
            }
            if (keyboard[SDL_SCANCODE_UP] && player.state == CLIMBING) {
                player.velY = -MOVE_SPEED;
            }
            if (keyboard[SDL_SCANCODE_DOWN] && player.state == CLIMBING) {
                player.velY = MOVE_SPEED;
            }
            if (keyboard[SDL_SCANCODE_C]) {
                if (player.state != JUMPING && player.state != CLIMBING)
                    player.state = CROUCHING;
            }
            if (keyboard[SDL_SCANCODE_SPACE] && player.state != JUMPING && player.state != CLIMBING) {
                player.velY = JUMP_FORCE;
                player.state = JUMPING;
            }
            if (keyboard[SDL_SCANCODE_F] && e.type == SDL_KEYDOWN) {
                shootBullet();
            }
        }

        update();
        render();
        SDL_Delay(16);
    }

    // Cleanup
    SDL_DestroyTexture(player.texture);
    SDL_DestroyTexture(gameMap.blockTexture);
    SDL_DestroyTexture(gameMap.ladderTexture);
    for (int i = 0; i < 10; i++) SDL_DestroyTexture(enemies[i].texture);
    for (int i = 0; i < 20; i++) SDL_DestroyTexture(bullets[i].texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​