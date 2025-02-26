#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <ctime>

const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;
const int TILE_SIZE = 64;
const int MAP_WIDTH = SCREEN_WIDTH / TILE_SIZE;
const int MAP_HEIGHT = SCREEN_HEIGHT / TILE_SIZE;
const int PLAYER_SPEED = 5;
const int BULLET_SPEED = 10;

// Structs for game objects
struct Vector2 {
    int x, y;
};

struct Player {
    Vector2 pos;
    SDL_Texture* texture;
    int width = 32, height = 32;
};

struct Bullet {
    Vector2 pos;
    Vector2 velocity;
    SDL_Texture* texture;
    bool active = false;
};

struct Enemy {
    Vector2 pos;
    SDL_Texture* texture;
    int width = 32, height = 32;
};

struct Tile {
    SDL_Texture* texture;
    bool isWall;
};

// Global variables
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
Mix_Chunk* shootSound = nullptr;
Player player;
std::vector<Bullet> bullets;
std::vector<Enemy> enemies;
std::vector<std::vector<Tile>> map(MAP_HEIGHT, std::vector<Tile>(MAP_WIDTH));

bool initSDL() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL Init failed: " << SDL_GetError() << std::endl;
        return false;
    }
    window = SDL_CreateWindow("Gauntlet Clone", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) return false;
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) return false;
    IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG); // BMP support included
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
    return true;
}

SDL_Texture* loadTexture(const char* path) {
    SDL_Surface* surface = IMG_Load(path);
    if (!surface) {
        std::cerr << "Failed to load image: " << IMG_GetError() << std::endl;
        return nullptr;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

void generateMap() {
    srand(time(nullptr));
    SDL_Texture* wallTexture = loadTexture("wall.bmp");
    SDL_Texture* floorTexture = loadTexture("floor.bmp");

    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            // Border walls and random interior walls
            bool isWall = (x == 0 || x == MAP_WIDTH - 1 || y == 0 || y == MAP_HEIGHT - 1 || (rand() % 5 == 0 && x > 1 && x < MAP_WIDTH - 2 && y > 1 && y < MAP_HEIGHT - 2));
            map[y][x].texture = isWall ? wallTexture : floorTexture;
            map[y][x].isWall = isWall;
        }
    }
}

void loadAssets() {
    player.texture = loadTexture("player.bmp"); // Player sprite
    player.pos = {TILE_SIZE * 2, TILE_SIZE * 2}; // Start near top-left

    generateMap();

    Enemy enemy;
    enemy.texture = loadTexture("enemy.bmp"); // Enemy sprite
    enemy.pos = {TILE_SIZE * (MAP_WIDTH - 3), TILE_SIZE * (MAP_HEIGHT - 3)};
    enemies.push_back(enemy);

    bullets.resize(10); // Bullet pool
    for (auto& bullet : bullets) {
        bullet.texture = loadTexture("bullet.bmp"); // Bullet sprite
    }

    shootSound = Mix_LoadWAV("shoot.wav"); // Shooting sound
}

bool checkCollision(int x, int y, int width, int height) {
    int tileX1 = x / TILE_SIZE;
    int tileY1 = y / TILE_SIZE;
    int tileX2 = (x + width - 1) / TILE_SIZE;
    int tileY2 = (y + height - 1) / TILE_SIZE;

    for (int ty = tileY1; ty <= tileY2; ty++) {
        for (int tx = tileX1; tx <= tileX2; tx++) {
            if (ty >= 0 && ty < MAP_HEIGHT && tx >= 0 && tx < MAP_WIDTH && map[ty][tx].isWall) {
                return true;
            }
        }
    }
    return false;
}

void handleInput(bool& running) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) running = false;
    }

    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    Vector2 newPos = player.pos;

    if (keys[SDL_SCANCODE_LEFT]) newPos.x -= PLAYER_SPEED;
    if (keys[SDL_SCANCODE_RIGHT]) newPos.x += PLAYER_SPEED;
    if (keys[SDL_SCANCODE_UP]) newPos.y -= PLAYER_SPEED;
    if (keys[SDL_SCANCODE_DOWN]) newPos.y += PLAYER_SPEED;

    // Check collision before moving
    if (!checkCollision(newPos.x, newPos.y, player.width, player.height)) {
        player.pos = newPos;
    }

    // Shooting (direction based on last movement)
    static Vector2 lastDir = {1, 0}; // Default right
    if (keys[SDL_SCANCODE_LEFT]) lastDir = {-1, 0};
    if (keys[SDL_SCANCODE_RIGHT]) lastDir = {1, 0};
    if (keys[SDL_SCANCODE_UP]) lastDir = {0, -1};
    if (keys[SDL_SCANCODE_DOWN]) lastDir = {0, 1};

    if (keys[SDL_SCANCODE_SPACE]) {
        for (auto& bullet : bullets) {
            if (!bullet.active) {
                bullet.pos = {player.pos.x + player.width / 2, player.pos.y + player.height / 2};
                bullet.velocity = {lastDir.x * BULLET_SPEED, lastDir.y * BULLET_SPEED};
                bullet.active = true;
                Mix_PlayChannel(-1, shootSound, 0);
                break;
            }
        }
    }
}

void update() {
    // Update bullets
    for (auto& bullet : bullets) {
        if (bullet.active) {
            bullet.pos.x += bullet.velocity.x;
            bullet.pos.y += bullet.velocity.y;
            if (bullet.pos.x < 0 || bullet.pos.x > SCREEN_WIDTH || bullet.pos.y < 0 || bullet.pos.y > SCREEN_HEIGHT || checkCollision(bullet.pos.x, bullet.pos.y, 16, 16)) {
                bullet.active = false;
            }

            // Bullet-enemy collision
            for (auto& enemy : enemies) {
                SDL_Rect bulletRect = {bullet.pos.x, bullet.pos.y, 16, 16};
                SDL_Rect enemyRect = {enemy.pos.x, enemy.pos.y, enemy.width, enemy.height};
                if (SDL_HasIntersection(&bulletRect, &enemyRect)) {
                    bullet.active = false;
                    enemy.pos = {-100, -100}; // Move off-screen (simple "kill")
                }
            }
        }
    }

    // Simple enemy AI (move towards player)
    for (auto& enemy : enemies) {
        Vector2 newPos = enemy.pos;
        if (enemy.pos.x > player.pos.x) newPos.x -= 2;
        if (enemy.pos.x < player.pos.x) newPos.x += 2;
        if (enemy.pos.y > player.pos.y) newPos.y -= 2;
        if (enemy.pos.y < player.pos.y) newPos.y += 2;

        if (!checkCollision(newPos.x, newPos.y, enemy.width, enemy.height)) {
            enemy.pos = newPos;
        }

        // Player-enemy collision (simple "damage")
        SDL_Rect playerRect = {player.pos.x, player.pos.y, player.width, player.height};
        SDL_Rect enemyRect = {enemy.pos.x, enemy.pos.y, enemy.width, enemy.height};
        if (SDL_HasIntersection(&playerRect, &enemyRect)) {
            player.pos = {TILE_SIZE * 2, TILE_SIZE * 2}; // Reset to start
        }
    }
}

void render() {
    SDL_RenderClear(renderer);

    // Render map
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            SDL_Rect tileRect = {x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE};
            SDL_RenderCopy(renderer, map[y][x].texture, nullptr, &tileRect);
        }
    }

    // Render enemies
    for (const auto& enemy : enemies) {
        SDL_Rect enemyRect = {enemy.pos.x, enemy.pos.y, enemy.width, enemy.height};
        SDL_RenderCopy(renderer, enemy.texture, nullptr, &enemyRect);
    }

    // Render bullets
    for (const auto& bullet : bullets) {
        if (bullet.active) {
            SDL_Rect bulletRect = {bullet.pos.x, bullet.pos.y, 16, 16};
            SDL_RenderCopy(renderer, bullet.texture, nullptr, &bulletRect);
        }
    }

    // Render player
    SDL_Rect playerRect = {player.pos.x, player.pos.y, player.width, player.height};
    SDL_RenderCopy(renderer, player.texture, nullptr, &playerRect);

    SDL_RenderPresent(renderer);
}

void cleanup() {
    SDL_DestroyTexture(player.texture);
    for (auto& row : map) {
        for (auto& tile : row) {
            if (tile.texture) SDL_DestroyTexture(tile.texture); // Cleanup happens once per unique texture
        }
    }
    for (auto& enemy : enemies) SDL_DestroyTexture(enemy.texture);
    for (auto& bullet : bullets) SDL_DestroyTexture(bullet.texture);
    Mix_FreeChunk(shootSound);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_Quit();
    IMG_Quit();
    SDL_Quit();
}

int main(int argc, char* args[]) {
    if (!initSDL()) return 1;
    loadAssets();

    bool running = true;
    while (running) {
        handleInput(running);
        update();
        render();
        SDL_Delay(16); // ~60 FPS
    }

    cleanup();
    return 0;
}
