#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <vector>
#include <iostream>

const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;
const int PLAYER_SPEED = 5;
const int JUMP_VELOCITY = -15;
const int GRAVITY = 1;
const int BULLET_SPEED = 10;
const int SCROLL_SPEED = 2;

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
    int frame = 0; // Animation frame
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
Mix_Chunk* jumpSound = nullptr;
Mix_Chunk* shootSound = nullptr;
Player player;
std::vector<Bullet> bullets;
std::vector<Enemy> enemies;
Background background;
std::vector<SDL_Rect> platforms; // Simple ground platforms

bool initSDL() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL Init failed: " << SDL_GetError() << std::endl;
        return false;
    }
    window = SDL_CreateWindow("Metal Slug Clone", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
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
    player.texture = loadTexture("player.png"); // Player sprite sheet (e.g., 4 frames: 64x64 each)
    player.pos = {100, 500};
    player.velocity = {0, 0};

    background.texture = loadTexture("background.png"); // Wide scrolling background
    background.width = SCREEN_WIDTH * 2; // Adjust based on your image
    background.height = SCREEN_HEIGHT;

    // Simple ground platform
    platforms.push_back({0, SCREEN_HEIGHT - 128, SCREEN_WIDTH * 2, 128});

    Enemy enemy;
    enemy.texture = loadTexture("enemy.png"); // Enemy sprite
    enemy.pos = {800, SCREEN_HEIGHT - 192}; // On ground
    enemies.push_back(enemy);

    bullets.resize(10); // Bullet pool
    for (auto& bullet : bullets) {
        bullet.texture = loadTexture("bullet.png"); // Bullet sprite
    }

    jumpSound = Mix_LoadWAV("jump.wav"); // Jump sound
    shootSound = Mix_LoadWAV("shoot.wav"); // Shoot sound
}

void handleInput(bool& running) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) running = false;
    }

    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    player.velocity.x = 0;

    if (keys[SDL_SCANCODE_LEFT] && player.pos.x > 0) player.velocity.x = -PLAYER_SPEED;
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

    // Player movement and gravity
    player.velocity.y += GRAVITY;
    player.pos.x += player.velocity.x;
    player.pos.y += player.velocity.y;

    // Platform collision
    for (const auto& platform : platforms) {
        SDL_Rect playerRect = {player.pos.x, player.pos.y, player.width, player.height};
        if (SDL_HasIntersection(&playerRect, &platform)) {
            if (player.velocity.y > 0) { // Falling
                player.pos.y = platform.y - player.height;
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

    // Enemy movement (simple patrol)
    for (auto& enemy : enemies) {
        enemy.pos.x -= 2; // Move left
        if (enemy.pos.x < -enemy.width) enemy.pos.x = SCREEN_WIDTH; // Respawn on right

        // Player-enemy collision (simple "damage")
        SDL_Rect playerRect = {player.pos.x, player.pos.y, player.width, player.height};
        SDL_Rect enemyRect = {enemy.pos.x, enemy.pos.y, enemy.width, enemy.height};
        if (SDL_HasIntersection(&playerRect, &enemyRect)) {
            player.pos = {100, 500}; // Reset position
        }
    }

    // Simple animation (walking)
    static int frameCounter = 0;
    if (player.velocity.x != 0) {
        frameCounter++;
        if (frameCounter % 10 == 0) player.frame = (player.frame + 1) % 4; // 4 frames
    } else {
        player.frame = 0; // Idle frame
    }
}

void render() {
    SDL_RenderClear(renderer);

    // Render scrolling background
    SDL_Rect bgRect1 = {background.x, 0, background.width, background.height};
    SDL_Rect bgRect2 = {background.x + background.width, 0, background.width, background.height};
    SDL_RenderCopy(renderer, background.texture, nullptr, &bgRect1);
    SDL_RenderCopy(renderer, background.texture, nullptr, &bgRect2);

    // Render platforms (using background for simplicity)
    for (const auto& platform : platforms) {
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Green for visibility
        SDL_RenderFillRect(renderer, &platform);
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

    // Render player with animation
    SDL_Rect srcRect = {player.frame * 64, 0, 64, 64}; // Assuming 4 frames in a row
    SDL_Rect dstRect = {player.pos.x, player.pos.y, player.width, player.height};
    SDL_RendererFlip flip = (player.velocity.x < 0) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
    SDL_RenderCopyEx(renderer, player.texture, &srcRect, &dstRect, 0, nullptr, flip);

    SDL_RenderPresent(renderer);
}

void cleanup() {
    SDL_DestroyTexture(player.texture);
    SDL_DestroyTexture(background.texture);
    for (auto& enemy : enemies) SDL_DestroyTexture(enemy.texture);
    for (auto& bullet : bullets) SDL_DestroyTexture(bullet.texture);
    Mix_FreeChunk(jumpSound);
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
