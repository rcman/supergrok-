#include <SDL2/SDL.h>
#include <vector>
#include <iostream>

const int SCREEN_WIDTH = 896;  // Arcade resolution (224x256 scaled 4x)
const int SCREEN_HEIGHT = 1024;
const float GRAVITY = 500.0f;
const float MOVE_SPEED = 150.0f;
const float JUMP_SPEED = -300.0f;

struct Vec2 {
    float x, y;
};

struct Platform {
    int x, y, width;
};

struct Ladder {
    int x, y, height;
};

struct Barrel {
    Vec2 pos;
    Vec2 vel;
    bool active;
};

struct Player {
    Vec2 pos = { 50, 900 };  // Starting position
    Vec2 vel = { 0, 0 };
    bool onGround = false;
    bool climbing = false;
    int ladderIndex = -1;
};

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
bool quit = false;

std::vector<Platform> platforms;
std::vector<Ladder> ladders;
std::vector<Barrel> barrels;
Player mario;

void initSDL() {
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("Donkey Kong Clone", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
}

void generateLevel() {
    // Simplified 25m level
    platforms.push_back({ 0, 900, 800 });    // Bottom girder
    platforms.push_back({ 100, 750, 600 });  // Second girder (slanted slightly)
    platforms.push_back({ 0, 600, 700 });    // Third girder
    platforms.push_back({ 100, 450, 600 });  // Fourth girder
    platforms.push_back({ 0, 300, 400 });    // Top platform (DK's level)

    ladders.push_back({ 200, 750, 150 });  // Ladder from bottom to second
    ladders.push_back({ 500, 600, 150 });  // Ladder from second to third
    ladders.push_back({ 300, 450, 150 });  // Ladder from third to fourth
}

void spawnBarrel() {
    Barrel barrel;
    barrel.pos = { 50, 250 };  // Spawn near DK
    barrel.vel = { 100, 0 };
    barrel.active = true;
    barrels.push_back(barrel);
}

bool checkCollision(float x, float y, const Platform& p) {
    return (x + 16 > p.x && x < p.x + p.width && y + 32 > p.y && y < p.y + 10);
}

bool onLadder(float x, float y, int& ladderIdx) {
    for (int i = 0; i < ladders.size(); i++) {
        if (x + 16 > ladders[i].x && x < ladders[i].x + 16 &&
            y + 32 > ladders[i].y && y < ladders[i].y + ladders[i].height) {
            ladderIdx = i;
            return true;
        }
    }
    ladderIdx = -1;
    return false;
}

void updatePlayer(float dt) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) quit = true;
    }

    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    mario.vel.x = 0;

    if (mario.climbing) {
        if (keys[SDL_SCANCODE_UP]) mario.vel.y = -MOVE_SPEED;
        else if (keys[SDL_SCANCODE_DOWN]) mario.vel.y = MOVE_SPEED;
        else mario.vel.y = 0;
        if (keys[SDL_SCANCODE_LEFT]) mario.vel.x = -MOVE_SPEED;
        if (keys[SDL_SCANCODE_RIGHT]) mario.vel.x = MOVE_SPEED;

        mario.pos.x += mario.vel.x * dt;
        mario.pos.y += mario.vel.y * dt;

        if (!onLadder(mario.pos.x, mario.pos.y, mario.ladderIndex)) {
            mario.climbing = false;
        }
    } else {
        if (keys[SDL_SCANCODE_LEFT]) mario.vel.x = -MOVE_SPEED;
        if (keys[SDL_SCANCODE_RIGHT]) mario.vel.x = MOVE_SPEED;
        if (keys[SDL_SCANCODE_SPACE] && mario.onGround) {
            mario.vel.y = JUMP_SPEED;
            mario.onGround = false;
        }

        mario.vel.y += GRAVITY * dt;
        mario.pos.x += mario.vel.x * dt;
        mario.pos.y += mario.vel.y * dt;

        mario.onGround = false;
        for (const auto& p : platforms) {
            if (checkCollision(mario.pos.x, mario.pos.y, p)) {
                mario.pos.y = p.y - 32;
                mario.vel.y = 0;
                mario.onGround = true;
            }
        }

        if (onLadder(mario.pos.x, mario.pos.y, mario.ladderIndex)) {
            if (keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_DOWN]) {
                mario.climbing = true;
                mario.vel.y = 0;
            }
        }
    }

    // Keep Mario on screen
    if (mario.pos.x < 0) mario.pos.x = 0;
    if (mario.pos.x > SCREEN_WIDTH - 16) mario.pos.x = SCREEN_WIDTH - 16;
}

void updateBarrels(float dt) {
    for (auto& b : barrels) {
        if (!b.active) continue;
        b.pos.x += b.vel.x * dt;
        b.pos.y += GRAVITY * dt;

        for (const auto& p : platforms) {
            if (checkCollision(b.pos.x, b.pos.y, p)) {
                b.pos.y = p.y - 16;
                b.vel.y = 0;
            }
        }

        if (b.pos.x > SCREEN_WIDTH || b.pos.y > SCREEN_HEIGHT) b.active = false;
    }
}

void render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Draw platforms
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    for (const auto& p : platforms) {
        SDL_Rect rect = { p.x, p.y, p.width, 10 };
        SDL_RenderFillRect(renderer, &rect);
    }

    // Draw ladders
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    for (const auto& l : ladders) {
        SDL_Rect rect = { l.x, l.y, 16, l.height };
        SDL_RenderFillRect(renderer, &rect);
    }

    // Draw Mario (placeholder)
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
    SDL_Rect marioRect = { (int)mario.pos.x, (int)mario.pos.y, 16, 32 };
    SDL_RenderFillRect(renderer, &marioRect);

    // Draw barrels
    SDL_SetRenderDrawColor(renderer, 139, 69, 19, 255);
    for (const auto& b : barrels) {
        if (b.active) {
            SDL_Rect barrelRect = { (int)b.pos.x, (int)b.pos.y, 16, 16 };
            SDL_RenderFillRect(renderer, &barrelRect);
        }
    }

    SDL_RenderPresent(renderer);
}

int main() {
    initSDL();
    generateLevel();

    Uint32 lastTime = SDL_GetTicks();
    Uint32 barrelTimer = 0;

    while (!quit) {
        Uint32 currentTime = SDL_GetTicks();
        float dt = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        barrelTimer += currentTime - lastTime;
        if (barrelTimer > 2000) {  // Spawn barrel every 2 seconds
            spawnBarrel();
            barrelTimer = 0;
        }

        updatePlayer(dt);
        updateBarrels(dt);
        render();
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
