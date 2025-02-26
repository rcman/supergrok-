#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <vector>
#include <iostream>

const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;
const int GRAVITY = 1;
const int JUMP_VELOCITY = -20;
const int PLAYER_SPEED = 5;
const int BULLET_SPEED = 10;

// Structs for game objects
struct Vector2 {
    int x, y;
};

struct Player {
    Vector2 pos;
    Vector2 velocity;
    SDL_Texture* texture;
    int width = 64, height = 64;
    bool isJumping = false;
};

struct Enemy {
    Vector2 pos;
    SDL_Texture* texture;
    int width = 64, height = 64;
};

struct Platform {
    SDL_Rect rect;
    SDL_Texture* texture;
};

struct Bullet {
    Vector2 pos;
    Vector2 velocity;
    SDL_Texture* texture;
    bool active = false;
};

// Global variables
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
Mix_Chunk* jumpSound = nullptr;
std::vector<Platform> platforms;
std::vector<Enemy> enemies;
std::vector<Bullet> bullets;
Player player;

bool initSDL() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) return false;
    window = SDL_CreateWindow("Rick Dangerous Clone", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) return false;
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) return false;
    IMG_Init(IMG_INIT_PNG);
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

void loadAssets() {
    player.texture = loadTexture("player.png"); // Replace with your player sprite
    player.pos = {100, 500};
    player.velocity = {0, 0};

    Platform platform;
    platform.rect = {0, 600, SCREEN_WIDTH, 120};
    platform.texture = loadTexture("platform.png"); // Replace with your platform image
    platforms.push_back(platform);

    Enemy enemy;
    enemy.pos = {800, 536};
    enemy.texture = loadTexture("enemy.png"); // Replace with your enemy sprite
    enemies.push_back(enemy);

    bullets.resize(10); // Pool of bullets
    for (auto& bullet : bullets) {
        bullet.texture = loadTexture("bullet.png"); // Replace with your bullet sprite
    }

    jumpSound = Mix_LoadWAV("jump.wav"); // Replace with your jump sound
}

void handleInput(bool& running) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) running = false;
    }

    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    player.velocity.x = 0;

    if (keys[SDL_SCANCODE_LEFT]) player.velocity.x = -PLAYER_SPEED;
    if (keys[SDL_SCANCODE_RIGHT]) player.velocity.x = PLAYER_SPEED;
    if (keys[SDL_SCANCODE_SPACE] && !player.isJumping) {
        player.velocity.y = JUMP_VELOCITY;
        player.isJumping = true;
        Mix_PlayChannel(-1, jumpSound, 0);
    }
    if (keys[SDL_SCANCODE_F]) { // Shoot
        for (auto& bullet : bullets) {
            if (!bullet.active) {
                bullet.pos = {player.pos.x + player.width, player.pos.y + player.height / 2};
                bullet.velocity = {BULLET_SPEED, 0};
                bullet.active = true;
                break;
            }
        }
    }
}

void update() {
    // Apply gravity
    player.velocity.y += GRAVITY;
    player.pos.x += player.velocity.x;
    player.pos.y += player.velocity.y;

    // Platform collision
    for (const auto& platform : platforms) {
        SDL_Rect playerRect = {player.pos.x, player.pos.y, player.width, player.height};
        if (SDL_HasIntersection(&playerRect, &platform.rect)) {
            if (player.velocity.y > 0) { // Falling
                player.pos.y = platform.rect.y - player.height;
                player.velocity.y = 0;
                player.isJumping = false;
            }
        }
    }

    // Keep player in bounds
    if (player.pos.y > SCREEN_HEIGHT - player.height) {
        player.pos.y = SCREEN_HEIGHT - player.height;
        player.velocity.y = 0;
        player.isJumping = false;
    }
    if (player.pos.x < 0) player.pos.x = 0;
    if (player.pos.x > SCREEN_WIDTH - player.width) player.pos.x = SCREEN_WIDTH - player.width;

    // Update bullets
    for (auto& bullet : bullets) {
        if (bullet.active) {
            bullet.pos.x += bullet.velocity.x;
            if (bullet.pos.x > SCREEN_WIDTH) bullet.active = false;

            // Bullet-enemy collision
            for (auto& enemy : enemies) {
                SDL_Rect bulletRect = {bullet.pos.x, bullet.pos.y, 16, 16};
                SDL_Rect enemyRect = {enemy.pos.x, enemy.pos.y, enemy.width, enemy.height};
                if (SDL_HasIntersection(&bulletRect, &enemyRect)) {
                    bullet.active = false;
                    enemy.pos.x = -100; // Simple "kill" by moving off-screen
                }
            }
        }
    }
}

void render() {
    SDL_RenderClear(renderer);

    // Render platforms
    for (const auto& platform : platforms) {
        SDL_RenderCopy(renderer, platform.texture, nullptr, &platform.rect);
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
    for (auto& platform : platforms) SDL_DestroyTexture(platform.texture);
    for (auto& enemy : enemies) SDL_DestroyTexture(enemy.texture);
    for (auto& bullet : bullets) SDL_DestroyTexture(bullet.texture);
    Mix_FreeChunk(jumpSound);
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
