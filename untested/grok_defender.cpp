#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <vector>
#include <iostream>

const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;
const int PLAYER_SPEED = 5;
const int BULLET_SPEED = 10;
const int SCROLL_SPEED = 2;

// Structs for game objects
struct Vector2 {
    int x, y;
};

struct Player {
    Vector2 pos;
    SDL_Texture* texture;
    int width = 64, height = 64;
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
    int width = 64, height = 64;
};

struct Background {
    SDL_Texture* texture;
    int x = 0; // Scrolling offset
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
    window = SDL_CreateWindow("Defender Clone", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
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
    SDL_QueryTexture(texture, nullptr, nullptr, &background.width, &background.height); // For background size
    SDL_FreeSurface(surface);
    return texture;
}

void loadAssets() {
    player.texture = loadTexture("player.png"); // Spaceship sprite
    player.pos = {SCREEN_WIDTH / 4, SCREEN_HEIGHT / 2};

    background.texture = loadTexture("background.png"); // Scrolling background (wider than screen)
    background.width = SCREEN_WIDTH * 2; // Adjust based on your image
    background.height = SCREEN_HEIGHT;

    Enemy enemy;
    enemy.texture = loadTexture("enemy.png"); // Enemy sprite
    enemy.pos = {SCREEN_WIDTH - 200, SCREEN_HEIGHT / 2};
    enemies.push_back(enemy);

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
    if (keys[SDL_SCANCODE_LEFT] && player.pos.x > 0) player.pos.x -= PLAYER_SPEED;
    if (keys[SDL_SCANCODE_RIGHT] && player.pos.x < SCREEN_WIDTH - player.width) player.pos.x += PLAYER_SPEED;
    if (keys[SDL_SCANCODE_UP] && player.pos.y > 0) player.pos.y -= PLAYER_SPEED;
    if (keys[SDL_SCANCODE_DOWN] && player.pos.y < SCREEN_HEIGHT - player.height) player.pos.y += PLAYER_SPEED;
    if (keys[SDL_SCANCODE_SPACE]) { // Shoot
        for (auto& bullet : bullets) {
            if (!bullet.active) {
                bullet.pos = {player.pos.x + player.width, player.pos.y + player.height / 2};
                bullet.velocity = {BULLET_SPEED, 0};
                bullet.active = true;
                Mix_PlayChannel(-1, shootSound, 0);
                break;
            }
        }
    }
}

void update() {
    // Scroll background
    background.x -= SCROLL_SPEED;
    if (background.x <= -background.width + SCREEN_WIDTH) background.x = 0;

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
                    enemy.pos.x = -100; // Move off-screen (simple "kill")
                }
            }
        }
    }

    // Simple enemy movement (towards player)
    for (auto& enemy : enemies) {
        if (enemy.pos.x > player.pos.x) enemy.pos.x -= 1;
        if (enemy.pos.x < player.pos.x) enemy.pos.x += 1;
        if (enemy.pos.y > player.pos.y) enemy.pos.y -= 1;
        if (enemy.pos.y < player.pos.y) enemy.pos.y += 1;
    }
}

void render() {
    SDL_RenderClear(renderer);

    // Render scrolling background (two copies for seamless loop)
    SDL_Rect bgRect1 = {background.x, 0, background.width, background.height};
    SDL_Rect bgRect2 = {background.x + background.width, 0, background.width, background.height};
    SDL_RenderCopy(renderer, background.texture, nullptr, &bgRect1);
    SDL_RenderCopy(renderer, background.texture, nullptr, &bgRect2);

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
