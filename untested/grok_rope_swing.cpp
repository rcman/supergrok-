#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <cmath>

const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;
const int PLAYER_SPEED = 5;
const int JUMP_VELOCITY = -15;
const int GRAVITY = 1;
const float PI = 3.1415926535;

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
    float swingAngle = 0; // Angle in radians
};

struct Rope {
    Vector2 pos; // Anchor point (top of rope)
    SDL_Texture* texture;
    int length = 200; // Rope length in pixels
    float angle = 0; // Swing angle in radians
    float angularVelocity = 0;
};

// Global variables
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
Player player;
Rope rope;

bool initSDL() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL Init failed: " << SDL_GetError() << std::endl;
        return false;
    }
    window = SDL_CreateWindow("Rope Swing Example", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) return false;
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) return false;
    IMG_Init(IMG_INIT_PNG);
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

    rope.pos = {SCREEN_WIDTH / 2, 100}; // Rope anchor at top center
    rope.texture = loadTexture("rope.png"); // Thin rope image
}

void handleInput(bool& running) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) running = false;
    }

    const Uint8* keys = SDL_GetKeyboardState(nullptr);

    if (!player.onRope) {
        player.velocity.x = 0;
        if (keys[SDL_SCANCODE_LEFT] && player.pos.x > 0) player.velocity.x = -PLAYER_SPEED;
        if (keys[SDL_SCANCODE_RIGHT] && player.pos.x < SCREEN_WIDTH - player.width) player.velocity.x = PLAYER_SPEED;
        if (keys[SDL_SCANCODE_SPACE] && !player.isJumping) {
            player.velocity.y = JUMP_VELOCITY;
            player.isJumping = true;
        }

        // Attach to rope
        if (keys[SDL_SCANCODE_UP]) {
            SDL_Rect playerRect = {player.pos.x, player.pos.y, player.width, player.height};
            SDL_Rect ropeRect = {rope.pos.x - 10, rope.pos.y, 20, rope.length};
            if (SDL_HasIntersection(&playerRect, &ropeRect)) {
                player.onRope = true;
                player.pos.x = rope.pos.x;
                player.pos.y = rope.pos.y + rope.length - player.height;
                player.velocity = {0, 0};
                rope.angle = atan2(player.pos.y - rope.pos.y, player.pos.x - rope.pos.x);
                rope.angularVelocity = 0;
            }
        }
    } else {
        // Detach from rope
        if (keys[SDL_SCANCODE_DOWN]) {
            player.onRope = false;
            player.isJumping = true;
            player.velocity.x = sin(rope.angle) * 10; // Retain some swing momentum
            player.velocity.y = -cos(rope.angle) * 10;
        }
        // Influence swing direction
        if (keys[SDL_SCANCODE_LEFT]) rope.angularVelocity -= 0.005;
        if (keys[SDL_SCANCODE_RIGHT]) rope.angularVelocity += 0.005;
    }
}

void update() {
    if (player.onRope) {
        // Pendulum physics
        rope.angularVelocity += 0.005 * sin(rope.angle); // Gravity effect
        rope.angularVelocity *= 0.99; // Damping
        rope.angle += rope.angularVelocity;
        player.pos.x = rope.pos.x + sin(rope.angle) * rope.length;
        player.pos.y = rope.pos.y + cos(rope.angle) * rope.length - player.height;
    } else {
        // Normal movement with gravity
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
        if (player.pos.x > SCREEN_WIDTH - player.width) player.pos.x = SCREEN_WIDTH - player.width;
    }
}

void render() {
    SDL_RenderClear(renderer);

    // Render rope
    SDL_Rect ropeRect = {rope.pos.x - 2, rope.pos.y, 4, rope.length};
    SDL_Point center = {2, 0}; // Pivot at top
    SDL_RenderCopyEx(renderer, rope.texture, nullptr, &ropeRect, rope.angle * 180 / PI, &center, SDL_FLIP_NONE);

    // Render player
    SDL_Rect playerRect = {player.pos.x, player.pos.y, player.width, player.height};
    SDL_RenderCopy(renderer, player.texture, nullptr, &playerRect);

    SDL_RenderPresent(renderer);
}

void cleanup() {
    SDL_DestroyTexture(player.texture);
    SDL_DestroyTexture(rope.texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
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
