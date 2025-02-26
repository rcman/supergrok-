#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <vector>
#include <cmath>
#include <iostream>

const int SCREEN_WIDTH = 1024;
const int SCREEN_HEIGHT = 768;
const float PLAYER_SPEED = 200.0f;
const float BULLET_SPEED = 400.0f;
const float ENEMY_SPEED = 100.0f;

struct Vec2 {
    float x, y;
};

struct Player {
    Vec2 pos = { SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 };
    float angle = 0;  // Radians
    float speed = PLAYER_SPEED;
};

struct Bullet {
    Vec2 pos;
    Vec2 vel;
    bool active = true;
};

struct Enemy {
    Vec2 pos;
    float angle;
    bool active = true;
};

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
SDL_Texture* playerTexture = nullptr;
SDL_Texture* enemyTexture = nullptr;
SDL_Texture* bulletTexture = nullptr;
Mix_Chunk* shootSound = nullptr;
Mix_Music* bgm = nullptr;
bool quit = false;

Player player;
std::vector<Bullet> bullets;
std::vector<Enemy> enemies;

void initSDL() {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    window = SDL_CreateWindow("Time Pilot Clone", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    IMG_Init(IMG_INIT_PNG);
    Mix_Init(MIX_INIT_WAV);
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);

    // Load graphics
    SDL_Surface* surface = IMG_Load("player.png");
    if (!surface) { std::cerr << "Failed to load player.png: " << IMG_GetError() << std::endl; quit = true; }
    playerTexture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("enemy.png");
    if (!surface) { std::cerr << "Failed to load enemy.png: " << IMG_GetError() << std::endl; quit = true; }
    enemyTexture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("bullet.png");
    if (!surface) { std::cerr << "Failed to load bullet.png: " << IMG_GetError() << std::endl; quit = true; }
    bulletTexture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    // Load sound
    shootSound = Mix_LoadWAV("shoot.wav");
    if (!shootSound) { std::cerr << "Failed to load shoot.wav: " << Mix_GetError() << std::endl; quit = true; }

    bgm = Mix_LoadMUS("bgm.wav");
    if (!bgm) { std::cerr << "Failed to load bgm.wav: " << Mix_GetError() << std::endl; quit = true; }
    Mix_PlayMusic(bgm, -1);  // Loop indefinitely
}

void spawnEnemy() {
    Enemy enemy;
    int side = rand() % 4;
    switch (side) {
        case 0: enemy.pos = { (float)(rand() % SCREEN_WIDTH), -20 }; break;
        case 1: enemy.pos = { (float)(rand() % SCREEN_WIDTH), SCREEN_HEIGHT + 20 }; break;
        case 2: enemy.pos = { -20, (float)(rand() % SCREEN_HEIGHT) }; break;
        case 3: enemy.pos = { SCREEN_WIDTH + 20, (float)(rand() % SCREEN_HEIGHT) }; break;
    }
    enemy.angle = atan2(player.pos.y - enemy.pos.y, player.pos.x - enemy.pos.x);
    enemies.push_back(enemy);
}

void updatePlayer(float dt) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) quit = true;
    }

    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    float turnSpeed = 3.0f;

    if (keys[SDL_SCANCODE_LEFT]) player.angle += turnSpeed * dt;
    if (keys[SDL_SCANCODE_RIGHT]) player.angle -= turnSpeed * dt;
    if (keys[SDL_SCANCODE_SPACE] && bullets.size() < 7) {  // Max 7 bullets on screen
        Bullet bullet;
        bullet.pos = player.pos;
        bullet.vel = { cosf(player.angle) * BULLET_SPEED, sinf(player.angle) * BULLET_SPEED };
        bullets.push_back(bullet);
        Mix_PlayChannel(-1, shootSound, 0);
    }

    player.pos.x += cosf(player.angle) * player.speed * dt;
    player.pos.y += sinf(player.angle) * player.speed * dt;

    if (player.pos.x < 0) player.pos.x += SCREEN_WIDTH;
    if (player.pos.x > SCREEN_WIDTH) player.pos.x -= SCREEN_WIDTH;
    if (player.pos.y < 0) player.pos.y += SCREEN_HEIGHT;
    if (player.pos.y > SCREEN_HEIGHT) player.pos.y -= SCREEN_HEIGHT;
}

void updateBullets(float dt) {
    for (auto& b : bullets) {
        if (!b.active) continue;
        b.pos.x += b.vel.x * dt;
        b.pos.y += b.vel.y * dt;
        if (b.pos.x < 0 || b.pos.x > SCREEN_WIDTH || b.pos.y < 0 || b.pos.y > SCREEN_HEIGHT) {
            b.active = false;
        }
    }
}

void updateEnemies(float dt) {
    for (auto& e : enemies) {
        if (!e.active) continue;
        e.pos.x += cosf(e.angle) * ENEMY_SPEED * dt;
        e.pos.y += sinf(e.angle) * ENEMY_SPEED * dt;

        if (rand() % 100 < 5) {
            e.angle = atan2(player.pos.y - e.pos.y, player.pos.x - e.pos.x);
        }

        for (auto& b : bullets) {
            if (b.active && fabs(b.pos.x - e.pos.x) < 16 && fabs(b.pos.y - e.pos.y) < 16) {
                b.active = false;
                e.active = false;
                break;
            }
        }

        if (fabs(e.pos.x - player.pos.x) < 20 && fabs(e.pos.y - player.pos.y) < 20) {
            e.active = false;  // Placeholder for player damage
        }
    }
}

void render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);  // Blue sky
    SDL_RenderClear(renderer);

    // Draw player
    SDL_Rect playerRect = { (int)(player.pos.x - 16), (int)(player.pos.y - 16), 32, 32 };
    SDL_RenderCopyEx(renderer, playerTexture, nullptr, &playerRect, player.angle * 180 / M_PI, nullptr, SDL_FLIP_NONE);

    // Draw bullets
    for (const auto& b : bullets) {
        if (b.active) {
            SDL_Rect bulletRect = { (int)(b.pos.x - 4), (int)(b.pos.y - 4), 8, 8 };
            SDL_RenderCopy(renderer, bulletTexture, nullptr, &bulletRect);
        }
    }

    // Draw enemies
    for (const auto& e : enemies) {
        if (e.active) {
            SDL_Rect enemyRect = { (int)(e.pos.x - 16), (int)(e.pos.y - 16), 32, 32 };
            SDL_RenderCopyEx(renderer, enemyTexture, nullptr, &enemyRect, e.angle * 180 / M_PI, nullptr, SDL_FLIP_NONE);
        }
    }

    SDL_RenderPresent(renderer);
}

int main() {
    initSDL();
    srand(time(nullptr));

    Uint32 lastTime = SDL_GetTicks();
    Uint32 enemyTimer = 0;

    while (!quit) {
        Uint32 currentTime = SDL_GetTicks();
        float dt = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        enemyTimer += currentTime - lastTime;
        if (enemyTimer > 1000) {  // Spawn enemy every second
            spawnEnemy();
            enemyTimer = 0;
        }

        updatePlayer(dt);
        updateBullets(dt);
        updateEnemies(dt);
        render();
    }

    // Cleanup
    SDL_DestroyTexture(playerTexture);
    SDL_DestroyTexture(enemyTexture);
    SDL_DestroyTexture(bulletTexture);
    Mix_FreeChunk(shootSound);
    Mix_FreeMusic(bgm);
    Mix_CloseAudio();
    Mix_Quit();
    IMG_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
