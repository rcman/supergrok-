#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <vector>
#include <iostream>
#include <cmath>

const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;
const int PLAYER_SPEED = 5;
const int BULLET_SPEED = 10;
const float PI = 3.1415926535;

// Structs for game objects
struct Vector2 {
    float x, y;
};

struct Player {
    Vector2 pos;
    SDL_Texture* texture;
    float angle = 0; // In degrees
    int width = 48, height = 48;
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
    int width = 48, height = 48;
    bool isBase = false; // Differentiates bases from ships
};

struct Background {
    SDL_Texture* texture;
    int width, height;
};

// Global variables
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
Mix_Chunk* shootSound = nullptr;
Player player;
std::vector<Bullet> bullets;
std::vector<Enemy> enemies;
Background background;

bool initSDL() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL Init failed: " << SDL_GetError() << std::endl;
        return false;
    }
    window = SDL_CreateWindow("Bosconian Clone", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
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
    player.texture = loadTexture("player.png"); // Player spaceship
    player.pos = {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2};

    background.texture = loadTexture("background.png"); // Starfield
    background.width = SCREEN_WIDTH;
    background.height = SCREEN_HEIGHT;

    // Enemy bases
    Enemy base;
    base.texture = loadTexture("base.png");
    base.pos = {200, 200};
    base.isBase = true;
    enemies.push_back(base);
    base.pos = {SCREEN_WIDTH - 200, SCREEN_HEIGHT - 200};
    enemies.push_back(base);

    // Enemy ships
    Enemy ship;
    ship.texture = loadTexture("enemy.png");
    ship.pos = {SCREEN_WIDTH - 300, 300};
    enemies.push_back(ship);

    bullets.resize(10); // Bullet pool
    for (auto& bullet : bullets) {
        bullet.texture = loadTexture("bullet.png"); // Bullet sprite
    }

    shootSound = Mix_LoadWAV("shoot.wav"); // Shooting sound
}

void handleInput(bool& running) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) running = false;
    }

    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    Vector2 velocity = {0, 0};

    // Movement in 8 directions
    if (keys[SDL_SCANCODE_UP]) velocity.y -= PLAYER_SPEED;
    if (keys[SDL_SCANCODE_DOWN]) velocity.y += PLAYER_SPEED;
    if (keys[SDL_SCANCODE_LEFT]) velocity.x -= PLAYER_SPEED;
    if (keys[SDL_SCANCODE_RIGHT]) velocity.x += PLAYER_SPEED;

    // Calculate angle based on movement
    if (velocity.x != 0 || velocity.y != 0) {
        player.angle = atan2(velocity.y, velocity.x) * 180 / PI + 90; // Adjust for sprite orientation
        player.pos.x += velocity.x;
        player.pos.y += velocity.y;
    }

    // Wrap around screen edges
    if (player.pos.x < -player.width) player.pos.x = SCREEN_WIDTH;
    if (player.pos.x > SCREEN_WIDTH) player.pos.x = -player.width;
    if (player.pos.y < -player.height) player.pos.y = SCREEN_HEIGHT;
    if (player.pos.y > SCREEN_HEIGHT) player.pos.y = -player.height;

    // Shooting
    if (keys[SDL_SCANCODE_SPACE]) {
        for (auto& bullet : bullets) {
            if (!bullet.active) {
                bullet.pos = player.pos;
                float rad = (player.angle - 90) * PI / 180; // Convert to radians, adjust for orientation
                bullet.velocity = {cos(rad) * BULLET_SPEED, sin(rad) * BULLET_SPEED};
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
            if (bullet.pos.x < -16 || bullet.pos.x > SCREEN_WIDTH || bullet.pos.y < -16 || bullet.pos.y > SCREEN_HEIGHT) {
                bullet.active = false;
            }

            // Bullet-enemy collision
            for (auto& enemy : enemies) {
                SDL_Rect bulletRect = {(int)bullet.pos.x, (int)bullet.pos.y, 16, 16};
                SDL_Rect enemyRect = {(int)enemy.pos.x, (int)enemy.pos.y, enemy.width, enemy.height};
                if (SDL_HasIntersection(&bulletRect, &enemyRect)) {
                    bullet.active = false;
                    enemy.pos = {-100, -100}; // Move off-screen (simple "destroy")
                }
            }
        }
    }

    // Enemy movement (simple pursuit for ships, bases stay static)
    for (auto& enemy : enemies) {
        if (!enemy.isBase) {
            float dx = player.pos.x - enemy.pos.x;
            float dy = player.pos.y - enemy.pos.y;
            float dist = sqrt(dx * dx + dy * dy);
            if (dist > 0) {
                enemy.pos.x += (dx / dist) * 2; // Move toward player at speed 2
                enemy.pos.y += (dy / dist) * 2;
            }
        }

        // Player-enemy collision (simple "damage")
        SDL_Rect playerRect = {(int)player.pos.x, (int)player.pos.y, player.width, player.height};
        SDL_Rect enemyRect = {(int)enemy.pos.x, (int)enemy.pos.y, enemy.width, enemy.height};
        if (SDL_HasIntersection(&playerRect, &enemyRect)) {
            player.pos = {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2}; // Reset to center
        }
    }
}

void render() {
    SDL_RenderClear(renderer);

    // Render background
    SDL_Rect bgRect = {0, 0, background.width, background.height};
    SDL_RenderCopy(renderer, background.texture, nullptr, &bgRect);

    // Render enemies
    for (const auto& enemy : enemies) {
        SDL_Rect enemyRect = {(int)enemy.pos.x, (int)enemy.pos.y, enemy.width, enemy.height};
        SDL_RenderCopy(renderer, enemy.texture, nullptr, &enemyRect);
    }

    // Render bullets
    for (const auto& bullet : bullets) {
        if (bullet.active) {
            SDL_Rect bulletRect = {(int)bullet.pos.x, (int)bullet.pos.y, 16, 16};
            SDL_RenderCopy(renderer, bullet.texture, nullptr, &bulletRect);
        }
    }

    // Render player with rotation
    SDL_Rect playerRect = {(int)player.pos.x, (int)player.pos.y, player.width, player.height};
    SDL_RenderCopyEx(renderer, player.texture, nullptr, &playerRect, player.angle, nullptr, SDL_FLIP_NONE);

    SDL_RenderPresent(renderer);
}

void cleanup() {
    SDL_DestroyTexture(player.texture);
    SDL_DestroyTexture(background.texture);
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
