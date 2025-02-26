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
const int ROAD_SPEED = 3;

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

struct RoadSegment {
    Vector2 pos;
    SDL_Texture* texture;
    int width = SCREEN_WIDTH, height = SCREEN_HEIGHT;
    bool hasBreak = false; // Indicates a gap
};

struct Warning {
    Vector2 pos;
    SDL_Texture* texture;
    bool visible = false;
};

// Global variables
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
Mix_Chunk* jumpSound = nullptr;
Player player;
std::vector<RoadSegment> roadSegments;
Warning warning;
int roadOffset = 0; // For seamless scrolling

bool initSDL() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL Init failed: " << SDL_GetError() << std::endl;
        return false;
    }
    window = SDL_CreateWindow("Bump 'n' Jump Clone", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
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
    player.texture = loadTexture("player.png"); // Car sprite
    player.pos = {SCREEN_WIDTH / 2 - 32, SCREEN_HEIGHT - 128}; // Start near bottom
    player.velocity = {0, 0};

    // Initial road segments (two for seamless scrolling)
    RoadSegment road1;
    road1.pos = {0, 0};
    road1.texture = loadTexture("road.png"); // Road background
    roadSegments.push_back(road1);

    RoadSegment road2;
    road2.pos = {0, -SCREEN_HEIGHT};
    road2.texture = road1.texture;
    roadSegments.push_back(road2);

    warning.texture = loadTexture("warning.png"); // Warning symbol
    warning.pos = {SCREEN_WIDTH / 2 - 32, SCREEN_HEIGHT / 2}; // Center screen

    jumpSound = Mix_LoadWAV("jump.wav"); // Jump sound
}

void handleInput(bool& running) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) running = false;
    }

    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    player.velocity.x = 0;

    if (keys[SDL_SCANCODE_LEFT] && player.pos.x > 0) player.velocity.x = -PLAYER_SPEED;
    if (keys[SDL_SCANCODE_RIGHT] && player.pos.x < SCREEN_WIDTH - player.width) player.velocity.x = PLAYER_SPEED;
    if (keys[SDL_SCANCODE_SPACE] && !player.isJumping) {
        player.velocity.y = JUMP_VELOCITY;
        player.isJumping = true;
        Mix_PlayChannel(-1, jumpSound, 0);
    }
}

void update() {
    // Scroll road segments
    for (auto& segment : roadSegments) {
        segment.pos.y += ROAD_SPEED;
    }

    // Reset road segments and add breaks
    static int breakTimer = 0;
    breakTimer++;
    for (auto& segment : roadSegments) {
        if (segment.pos.y >= SCREEN_HEIGHT) {
            segment.pos.y -= SCREEN_HEIGHT * 2; // Move to top
            segment.hasBreak = (breakTimer > 200 && rand() % 3 == 0); // Random break every ~200 frames
            if (segment.hasBreak) breakTimer = 0; // Reset timer after adding a break
        }
    }

    // Warning for upcoming break
    warning.visible = false;
    for (const auto& segment : roadSegments) {
        if (segment.hasBreak && segment.pos.y > -SCREEN_HEIGHT && segment.pos.y < SCREEN_HEIGHT / 2) {
            warning.visible = true;
            break;
        }
    }

    // Player movement and gravity
    player.velocity.y += GRAVITY;
    player.pos.x += player.velocity.x;
    player.pos.y += player.velocity.y;

    // Ground collision and break detection
    bool onGround = false;
    for (const auto& segment : roadSegments) {
        SDL_Rect playerRect = {player.pos.x, player.pos.y, player.width, player.height};
        SDL_Rect roadRect = {segment.pos.x, segment.pos.y, segment.width, segment.height};
        if (!segment.hasBreak && SDL_HasIntersection(&playerRect, &roadRect)) {
            if (player.velocity.y > 0) { // Falling
                player.pos.y = segment.pos.y - player.height;
                player.velocity.y = 0;
                player.isJumping = false;
                onGround = true;
            }
        }
    }

    // Fall through break (game over condition)
    if (!onGround && player.pos.y > SCREEN_HEIGHT) {
        player.pos = {SCREEN_WIDTH / 2 - 32, SCREEN_HEIGHT - 128}; // Reset to start
        player.velocity = {0, 0};
        player.isJumping = false;
    }

    // Keep player in horizontal bounds
    if (player.pos.x < 0) player.pos.x = 0;
    if (player.pos.x > SCREEN_WIDTH - player.width) player.pos.x = SCREEN_WIDTH - player.width;
}

void render() {
    SDL_RenderClear(renderer);

    // Render road segments
    for (const auto& segment : roadSegments) {
        SDL_Rect roadRect = {segment.pos.x, segment.pos.y, segment.width, segment.height};
        if (!segment.hasBreak) {
            SDL_RenderCopy(renderer, segment.texture, nullptr, &roadRect);
        } else {
            // Simulate break by drawing partial road
            SDL_Rect topPart = {segment.pos.x, segment.pos.y, segment.width, segment.height / 2};
            SDL_Rect bottomPart = {segment.pos.x, segment.pos.y + segment.height / 2 + 100, segment.width, segment.height / 2 - 100};
            SDL_RenderCopy(renderer, segment.texture, nullptr, &topPart);
            SDL_RenderCopy(renderer, segment.texture, nullptr, &bottomPart);
        }
    }

    // Render warning symbol
    if (warning.visible) {
        SDL_Rect warningRect = {warning.pos.x, warning.pos.y, 64, 64};
        SDL_RenderCopy(renderer, warning.texture, nullptr, &warningRect);
    }

    // Render player
    SDL_Rect playerRect = {player.pos.x, player.pos.y, player.width, player.height};
    SDL_RenderCopy(renderer, player.texture, nullptr, &playerRect);

    SDL_RenderPresent(renderer);
}

void cleanup() {
    SDL_DestroyTexture(player.texture);
    SDL_DestroyTexture(roadSegments[0].texture); // Shared texture
    SDL_DestroyTexture(warning.texture);
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
