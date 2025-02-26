#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <vector>
#include <iostream>
#include <cmath>

const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;
const int PLAYER_SPEED = 5;
const int JUMP_VELOCITY = -15;
const int GRAVITY = 1;
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
    bool onRope = false;
    float swingAngle = 0; // For rope swinging
};

struct Rope {
    Vector2 pos; // Anchor point (top of rope)
    SDL_Texture* texture;
    int length = 100;
    float angle = 0; // Swing angle in radians
    float angularVelocity = 0;
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
Mix_Chunk* swingSound = nullptr;
Player player;
std::vector<Rope> ropes;
std::vector<Enemy> enemies;
Background background;

bool initSDL() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL Init failed: " << SDL_GetError() << std::endl;
        return false;
    }
    window = SDL_CreateWindow("Pitfall Clone", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
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
    player.texture = loadTexture("player.png"); // Player sprite
    player.pos = {100, 500};
    player.velocity = {0, 0};

    background.texture = loadTexture("background.png"); // Scrolling jungle
    background.width = SCREEN_WIDTH * 2; // Wider than screen
    background.height = SCREEN_HEIGHT;

    Rope rope;
    rope.pos = {600, 200}; // Rope anchor point
    rope.texture = loadTexture("rope.png");
    ropes.push_back(rope);

    Enemy enemy;
    enemy.texture = loadTexture("enemy.png"); // Scorpion sprite
    enemy.pos = {800, 600};
    enemies.push_back(enemy);

    jumpSound = Mix_LoadWAV("jump.wav"); // Jump sound
    swingSound = Mix_LoadWAV("swing.wav"); // Rope swing sound
}

void handleInput(bool& running) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) running = false;
    }

    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    player.velocity.x = 0;

    if (!player.onRope) {
        if (keys[SDL_SCANCODE_LEFT] && player.pos.x > 0) player.velocity.x = -PLAYER_SPEED;
        if (keys[SDL_SCANCODE_RIGHT]) player.velocity.x = PLAYER_SPEED;
        if (keys[SDL_SCANCODE_SPACE] && !player.isJumping) {
            player.velocity.y = JUMP_VELOCITY;
            player.isJumping = true;
            Mix_PlayChannel(-1, jumpSound, 0);
        }
    }

    // Rope interaction
    if (keys[SDL_SCANCODE_UP]) {
        for (auto& rope : ropes) {
            SDL_Rect playerRect = {player.pos.x, player.pos.y, player.width, player.height};
            SDL_Rect ropeRect = {rope.pos.x - 10, rope.pos.y, 20, rope.length};
            if (SDL_HasIntersection(&playerRect, &ropeRect) && !player.onRope) {
                player.onRope = true;
                player.pos.x = rope.pos.x;
                player.pos.y = rope.pos.y + rope.length - player.height;
                player.velocity = {0, 0};
                Mix_PlayChannel(-1, swingSound, 0);
                break;
            }
        }
    }
    if (player.onRope && keys[SDL_SCANCODE_DOWN]) {
        player.onRope = false;
        player.isJumping = true;
    }
}

void update() {
    // Scroll background
    background.x -= SCROLL_SPEED;
    if (background.x <= -background.width + SCREEN_WIDTH) background.x = 0;

    if (player.onRope) {
        // Simple pendulum physics
        for (auto& rope : ropes) {
            if (player.pos.x == rope.pos.x) {
                rope.angularVelocity += 0.005 * sin(rope.angle); // Gravity effect
                rope.angularVelocity *= 0.99; // Damping
                rope.angle += rope.angularVelocity;
                player.pos.x = rope.pos.x + sin(rope.angle) * rope.length;
                player.pos.y = rope.pos.y + cos(rope.angle) * rope.length - player.height;
                break;
            }
        }
    } else {
        // Apply gravity and movement
        player.velocity.y += GRAVITY;
        player.pos.x += player.velocity.x;
        player.pos.y += player.velocity.y;

        // Ground collision
        if (player.pos.y > SCREEN_HEIGHT - player.height) {
            player.pos.y = SCREEN_HEIGHT - player.height;
            player.velocity.y = 0;
            player.isJumping = false;
        }
        if (player.pos.x < 0) player.pos.x = 0;
    }

    // Enemy movement (simple left-right patrol)
    for (auto& enemy : enemies) {
        enemy.pos.x += (enemy.pos.x > SCREEN_WIDTH / 2) ? -1 : 1;
        if (enemy.pos.x < 0 || enemy.pos.x > SCREEN_WIDTH - enemy.width) enemy.pos.x = SCREEN_WIDTH - enemy.width;

        // Player-enemy collision (simple "death")
        SDL_Rect playerRect = {player.pos.x, player.pos.y, player.width, player.height};
        SDL_Rect enemyRect = {enemy.pos.x, enemy.pos.y, enemy.width, enemy.height};
        if (SDL_HasIntersection(&playerRect, &enemyRect)) {
            player.pos = {100, 500}; // Reset position
            player.velocity = {0, 0};
        }
    }
}

void render() {
    SDL_RenderClear(renderer);

    // Render scrolling background
    SDL_Rect bgRect1 = {background.x, 0, background.width, background.height};
    SDL_Rect bgRect2 = {background.x + background.width, 0, background.width, background.height};
    SDL_RenderCopy(renderer, background.texture, nullptr, &bgRect1);
    SDL_RenderCopy(renderer, background.texture, nullptr, &bgRect2);

    // Render ropes
    for (const auto& rope : ropes) {
        SDL_Rect ropeRect = {rope.pos.x - 2, rope.pos.y, 4, rope.length};
        SDL_RenderCopyEx(renderer, rope.texture, nullptr, &ropeRect, rope.angle * 180 / M_PI, nullptr, SDL_FLIP_NONE);
    }

    // Render enemies
    for (const auto& enemy : enemies) {
        SDL_Rect enemyRect = {enemy.pos.x, enemy.pos.y, enemy.width, enemy.height};
        SDL_RenderCopy(renderer, enemy.texture, nullptr, &enemyRect);
    }

    // Render player
    SDL_Rect playerRect = {player.pos.x, player.pos.y, player.width, player.height};
    SDL_RenderCopy(renderer, player.texture, nullptr, &playerRect);

    SDL_RenderPresent(renderer);
}

void cleanup() {
    SDL_DestroyTexture(player.texture);
    SDL_DestroyTexture(background.texture);
    for (auto& rope : ropes) SDL_DestroyTexture(rope.texture);
    for (auto& enemy : enemies) SDL_DestroyTexture(enemy.texture);
    Mix_FreeChunk(jumpSound);
    Mix_FreeChunk(swingSound);
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
