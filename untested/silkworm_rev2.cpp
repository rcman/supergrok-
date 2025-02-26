#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include <string>
#include <cmath>

const int SCREEN_WIDTH = 1920;
const int SCREEN_HEIGHT = 1080;

// Player properties
const int PLAYER_WIDTH = 64;
const int PLAYER_HEIGHT = 32;
const int PLAYER_SPEED = 5;

// Bullet properties
const int BULLET_WIDTH = 16;
const int BULLET_HEIGHT = 8;
const int BULLET_SPEED = 10;

// Enemy properties
const int ENEMY_WIDTH = 48;
const int ENEMY_HEIGHT = 24;
const int ENEMY_SPEED = 3;

// Background properties
const int BG_FAR_WIDTH = 1920;
const int BG_FAR_HEIGHT = 1080;
const int BG_NEAR_WIDTH = 1920;
const int BG_NEAR_HEIGHT = 1080;
const int SCROLL_SPEED_FAR = 1;
const int SCROLL_SPEED_NEAR = 3;

struct Bullet {
    int x, y;
};

struct Enemy {
    int x, y;
    float amplitude, frequency, initialY;
};

int main(int argc, char* argv[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0 || IMG_Init(IMG_INIT_PNG) == 0 ||
        Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0 || TTF_Init() < 0) {
        SDL_Log("Initialization failed: %s", SDL_GetError());
        return 1;
    }

    // Create window and renderer
    SDL_Window* window = SDL_CreateWindow("Silkworm Clone", SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!window || !renderer) {
        SDL_Log("Window/Renderer creation failed: %s", SDL_GetError());
        return 1;
    }

    // Load assets
    SDL_Texture* playerTexture = IMG_LoadTexture(renderer, "player.png");
    SDL_Texture* bulletTexture = IMG_LoadTexture(renderer, "bullet.png");
    SDL_Texture* enemyTexture = IMG_LoadTexture(renderer, "enemy.png");
    SDL_Texture* bgFarTexture = IMG_LoadTexture(renderer, "bg_far.png");
    SDL_Texture* bgNearTexture = IMG_LoadTexture(renderer, "bg_near.png");
    Mix_Chunk* shootSound = Mix_LoadWAV("shoot.wav");
    Mix_Chunk* explosionSound = Mix_LoadWAV("explosion.wav");
    Mix_Music* bgMusic = Mix_LoadMUS("background.mp3");
    TTF_Font* font = TTF_OpenFont("font.ttf", 24);

    if (!playerTexture || !bulletTexture || !enemyTexture || !bgFarTexture || !bgNearTexture ||
        !shootSound || !explosionSound || !bgMusic || !font) {
        SDL_Log("Asset loading failed: %s", SDL_GetError());
        return 1;
    }

    // Game variables
    int playerX = 100, playerY = SCREEN_HEIGHT / 2;
    std::vector<Bullet> bullets;
    std::vector<Enemy> enemies;
    int bgFarX = 0, bgNearX = 0;
    int score = 0;
    Uint32 lastEnemySpawn = 0;
    bool running = true;
    const Uint8* keystate = SDL_GetKeyboardState(NULL);

    Mix_PlayMusic(bgMusic, -1); // Start background music

    // Game loop
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
        }

        // Update player
        if (keystate[SDL_SCANCODE_LEFT] && playerX > 0) playerX -= PLAYER_SPEED;
        if (keystate[SDL_SCANCODE_RIGHT] && playerX < SCREEN_WIDTH - PLAYER_WIDTH) playerX += PLAYER_SPEED;
        if (keystate[SDL_SCANCODE_UP] && playerY > 0) playerY -= PLAYER_SPEED;
        if (keystate[SDL_SCANCODE_DOWN] && playerY < SCREEN_HEIGHT - PLAYER_HEIGHT) playerY += PLAYER_SPEED;
        if (keystate[SDL_SCANCODE_SPACE] && !keystate[SDL_SCANCODE_SPACE - 1]) { // Shoot on key press
            bullets.push_back({playerX + PLAYER_WIDTH, playerY + PLAYER_HEIGHT / 2});
            Mix_PlayChannel(-1, shootSound, 0);
        }

        // Update bullets
        for (auto& b : bullets) b.x += BULLET_SPEED;
        bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
                                     [](const Bullet& b) { return b.x > SCREEN_WIDTH; }), bullets.end());

        // Spawn enemies
        if (SDL_GetTicks() - lastEnemySpawn > 1000) { // Every 1 second
            Enemy e = {SCREEN_WIDTH, rand() % (SCREEN_HEIGHT - ENEMY_HEIGHT), 20.0f, 0.01f, float(rand() % (SCREEN_HEIGHT - ENEMY_HEIGHT))};
            enemies.push_back(e);
            lastEnemySpawn = SDL_GetTicks();
        }

        // Update enemies
        for (auto& e : enemies) {
            e.x -= ENEMY_SPEED;
            e.y = e.initialY + e.amplitude * sin(e.frequency * e.x);
        }
        enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
                                     [](const Enemy& e) { return e.x < -ENEMY_WIDTH; }), enemies.end());

        // Collision detection
        for (auto eIt = enemies.begin(); eIt != enemies.end();) {
            bool hit = false;
            for (auto bIt = bullets.begin(); bIt != bullets.end() && !hit;) {
                if (bIt->x < eIt->x + ENEMY_WIDTH && bIt->x + BULLET_WIDTH > eIt->x &&
                    bIt->y < eIt->y + ENEMY_HEIGHT && bIt->y + BULLET_HEIGHT > eIt->y) {
                    Mix_PlayChannel(-1, explosionSound, 0);
                    score += 10;
                    eIt = enemies.erase(eIt);
                    bIt = bullets.erase(bIt);
                    hit = true;
                } else {
                    ++bIt;
                }
            }
            if (!hit) ++eIt;
        }

        // Update background
        bgFarX -= SCROLL_SPEED_FAR;
        if (bgFarX <= -BG_FAR_WIDTH) bgFarX = 0;
        bgNearX -= SCROLL_SPEED_NEAR;
        if (bgNearX <= -BG_NEAR_WIDTH) bgNearX = 0;

        // Render
        SDL_RenderClear(renderer);

        // Background
        SDL_Rect bgFarRect1 = {bgFarX, 0, BG_FAR_WIDTH, BG_FAR_HEIGHT};
        SDL_Rect bgFarRect2 = {bgFarX + BG_FAR_WIDTH, 0, BG_FAR_WIDTH, BG_FAR_HEIGHT};
        SDL_RenderCopy(renderer, bgFarTexture, NULL, &bgFarRect1);
        SDL_RenderCopy(renderer, bgFarTexture, NULL, &bgFarRect2);
        SDL_Rect bgNearRect1 = {bgNearX, 0, BG_NEAR_WIDTH, BG_NEAR_HEIGHT};
        SDL_Rect bgNearRect2 = {bgNearX + BG_NEAR_WIDTH, 0, BG_NEAR_WIDTH, BG_NEAR_HEIGHT};
        SDL_RenderCopy(renderer, bgNearTexture, NULL, &bgNearRect1);
        SDL_RenderCopy(renderer, bgNearTexture, NULL, &bgNearRect2);

        // Player
        SDL_Rect playerRect = {playerX, playerY, PLAYER_WIDTH, PLAYER_HEIGHT};
        SDL_RenderCopy(renderer, playerTexture, NULL, &playerRect);

        // Bullets
        for (const auto& b : bullets) {
            SDL_Rect bulletRect = {b.x, b.y, BULLET_WIDTH, BULLET_HEIGHT};
            SDL_RenderCopy(renderer, bulletTexture, NULL, &bulletRect);
        }

        // Enemies
        for (const auto& e : enemies) {
            SDL_Rect enemyRect = {e.x, int(e.y), ENEMY_WIDTH, ENEMY_HEIGHT};
            SDL_RenderCopy(renderer, enemyTexture, NULL, &enemyRect);
        }

        // Score
        SDL_Color color = {255, 255, 255, 255};
        std::string scoreText = "Score: " + std::to_string(score);
        SDL_Surface* textSurface = TTF_RenderText_Solid(font, scoreText.c_str(), color);
        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        int textW, textH;
        SDL_QueryTexture(textTexture, NULL, NULL, &textW, &textH);
        SDL_Rect textRect = {10, 10, textW, textH};
        SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
        SDL_FreeSurface(textSurface);
        SDL_DestroyTexture(textTexture);

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60 FPS
    }

    // Cleanup
    SDL_DestroyTexture(playerTexture);
    SDL_DestroyTexture(bulletTexture);
    SDL_DestroyTexture(enemyTexture);
    SDL_DestroyTexture(bgFarTexture);
    SDL_DestroyTexture(bgNearTexture);
    Mix_FreeChunk(shootSound);
    Mix_FreeChunk(explosionSound);
    Mix_FreeMusic(bgMusic);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    Mix_Quit();
    IMG_Quit();
    SDL_Quit();

    return 0;
}
