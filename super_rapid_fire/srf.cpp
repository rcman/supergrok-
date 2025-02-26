#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>

// Window dimensions
const int WINDOW_WIDTH = 512;
const int WINDOW_HEIGHT = 512;

// Player structure
struct Player {
    float x = WINDOW_WIDTH / 2;
    float y = WINDOW_HEIGHT - 100;
    float speed = 5.0f;
    int fireRate = 10; // Frames between shots
    int fireCounter = 0;
    int shotLevel = 1; // Power-up level
    int health = 3;
};

// Bullet structure
struct Bullet {
    float x, y;
    float speed = -10.0f; // Negative for upward movement
    bool active = true;
};

// Enemy structure
struct Enemy {
    float x, y;
    float speed;
    int health = 1;
    int type; // 0: straight, 1: sine wave
    float angle = 0.0f; // For sine movement
};

// Power-up structure
struct PowerUp {
    float x, y;
    float speed = 2.0f;
    bool active = true;
};

// Function to load textures
SDL_Texture* loadTexture(const char* filepath, SDL_Renderer* renderer) {
    SDL_Surface* surface = SDL_LoadBMP(filepath); // Replace with SDL_image for PNG support
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

int main(int argc, char* argv[]) {
    // Seed random number generator
    std::srand(std::time(nullptr));

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    // Initialize SDL_mixer for audio
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        SDL_Log("Mix_OpenAudio failed: %s", Mix_GetError());
        SDL_Quit();
        return 1;
    }

    // Initialize SDL_ttf for text rendering
    if (TTF_Init() == -1) {
        SDL_Log("TTF_Init failed: %s", TTF_GetError());
        Mix_CloseAudio();
        SDL_Quit();
        return 1;
    }

    // Create window and renderer
    SDL_Window* window = SDL_CreateWindow("Super Rapid Fire Clone", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if (!window) {
        SDL_Log("Window creation failed: %s", SDL_GetError());
        TTF_Quit();
        Mix_CloseAudio();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        SDL_Log("Renderer creation failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        Mix_CloseAudio();
        SDL_Quit();
        return 1;
    }

    // Load textures (replace with actual file paths)
    SDL_Texture* playerTexture = loadTexture("player.bmp", renderer);
    SDL_Texture* enemyTexture = loadTexture("enemy.bmp", renderer);
    SDL_Texture* bulletTexture = loadTexture("bullet.bmp", renderer);
    SDL_Texture* powerupTexture = loadTexture("powerup.bmp", renderer);
    SDL_Texture* backgroundTexture = loadTexture("background.bmp", renderer);

    // Load sounds (replace with actual file paths)
    Mix_Music* bgMusic = Mix_LoadMUS("background.mp3");
    Mix_Chunk* shootSound = Mix_LoadWAV("shoot.wav");
    Mix_Chunk* explosionSound = Mix_LoadWAV("explosion.wav");

    // Load font for score (replace with actual font file)
    TTF_Font* font = TTF_OpenFont("font.ttf", 24);
    if (!font) {
        SDL_Log("Failed to load font: %s", TTF_GetError());
        // Cleanup and exit (minimal cleanup here for brevity)
    }

    // Game objects
    Player player;
    std::vector<Bullet> bullets;
    std::vector<Enemy> enemies;
    std::vector<PowerUp> powerups;
    float backgroundY = 0.0f;
    const float scrollSpeed = 2.0f;
    const int backgroundHeight = 1024; // Assuming background is 512x1024
    int score = 0;

    // Enemy spawn timer
    Uint32 enemySpawnTimer = 0;
    const Uint32 spawnInterval = 1000; // Spawn every 1 second

    // Game loop
    bool running = true;
    SDL_Event event;

    // Play background music
    Mix_PlayMusic(bgMusic, -1);

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
        }

        const Uint8* keys = SDL_GetKeyboardState(NULL);

        // Clear renderer
        SDL_RenderClear(renderer);

        // Update background
        backgroundY += scrollSpeed;
        if (backgroundY >= backgroundHeight) backgroundY = 0;

        // Render background
        SDL_Rect bgRect1 = {0, (int)(backgroundY - backgroundHeight), WINDOW_WIDTH, backgroundHeight};
        SDL_Rect bgRect2 = {0, (int)backgroundY, WINDOW_WIDTH, backgroundHeight};
        SDL_RenderCopy(renderer, backgroundTexture, NULL, &bgRect1);
        SDL_RenderCopy(renderer, backgroundTexture, NULL, &bgRect2);

        // Update player
        if (keys[SDL_SCANCODE_UP] && player.y > 0) player.y -= player.speed;
        if (keys[SDL_SCANCODE_DOWN] && player.y < WINDOW_HEIGHT - 32) player.y += player.speed;
        if (keys[SDL_SCANCODE_LEFT] && player.x > 0) player.x -= player.speed;
        if (keys[SDL_SCANCODE_RIGHT] && player.x < WINDOW_WIDTH - 32) player.x += player.speed;

        if (keys[SDL_SCANCODE_SPACE] && player.fireCounter <= 0) {
            if (player.shotLevel == 1) {
                bullets.push_back({player.x + 12, player.y, -10.0f});
            } else if (player.shotLevel == 2) { // Spread shot
                bullets.push_back({player.x + 12, player.y, -10.0f});
                bullets.push_back({player.x + 8, player.y, -8.0f});
                bullets.push_back({player.x + 16, player.y, -8.0f});
            }
            player.fireCounter = player.fireRate;
            Mix_PlayChannel(-1, shootSound, 0);
        }
        if (player.fireCounter > 0) player.fireCounter--;

        // Update bullets
        for (auto& bullet : bullets) {
            if (bullet.active) {
                bullet.y += bullet.speed;
                if (bullet.y < 0) bullet.active = false;
            }
        }

        // Spawn enemies
        if (SDL_GetTicks() - enemySpawnTimer > spawnInterval) {
            int type = rand() % 2;
            enemies.push_back({(float)(rand() % (WINDOW_WIDTH - 32)), 0, 3.0f, 1, type});
            if (rand() % 10 == 0) powerups.push_back({(float)(rand() % (WINDOW_WIDTH - 16)), 0});
            enemySpawnTimer = SDL_GetTicks();
        }

        // Update enemies
        for (auto it = enemies.begin(); it != enemies.end();) {
            if (it->type == 0) {
                it->y += it->speed; // Straight down
            } else {
                it->y += it->speed;
                it->x += sin(it->angle) * 5.0f; // Sine wave
                it->angle += 0.1f;
            }

            for (auto& bullet : bullets) {
                if (bullet.active && abs(bullet.x - it->x) < 16 && abs(bullet.y - it->y) < 16) {
                    it->health--;
                    bullet.active = false;
                    Mix_PlayChannel(-1, explosionSound, 0);
                }
            }

            if (it->health <= 0 || it->y > WINDOW_HEIGHT) {
                if (it->health <= 0) score += 100;
                it = enemies.erase(it);
            } else {
                ++it;
            }
        }

        // Update power-ups
        for (auto it = powerups.begin(); it != powerups.end();) {
            it->y += it->speed;
            if (abs(it->x - player.x) < 32 && abs(it->y - player.y) < 32) {
                player.shotLevel = 2; // Upgrade to spread shot
                it->active = false;
            }
            if (!it->active || it->y > WINDOW_HEIGHT) {
                it = powerups.erase(it);
            } else {
                ++it;
            }
        }

        // Render player
        SDL_Rect playerRect = {(int)player.x, (int)player.y, 32, 32};
        SDL_RenderCopy(renderer, playerTexture, NULL, &playerRect);

        // Render bullets
        for (const auto& bullet : bullets) {
            if (bullet.active) {
                SDL_Rect rect = {(int)bullet.x, (int)bullet.y, 8, 16};
                SDL_RenderCopy(renderer, bulletTexture, NULL, &rect);
            }
        }

        // Render enemies
        for (const auto& enemy : enemies) {
            SDL_Rect rect = {(int)enemy.x, (int)enemy.y, 32, 32};
            SDL_RenderCopy(renderer, enemyTexture, NULL, &rect);
        }

        // Render power-ups
        for (const auto& powerup : powerups) {
            if (powerup.active) {
                SDL_Rect rect = {(int)powerup.x, (int)powerup.y, 16, 16};
                SDL_RenderCopy(renderer, powerupTexture, NULL, &rect);
            }
        }

        // Render score
        SDL_Color white = {255, 255, 255};
        SDL_Surface* scoreSurface = TTF_RenderText_Solid(font, ("Score: " + std::to_string(score)).c_str(), white);
        SDL_Texture* scoreTexture = SDL_CreateTextureFromSurface(renderer, scoreSurface);
        SDL_Rect scoreRect = {10, 10, scoreSurface->w, scoreSurface->h};
        SDL_RenderCopy(renderer, scoreTexture, NULL, &scoreRect);
        SDL_FreeSurface(scoreSurface);
        SDL_DestroyTexture(scoreTexture);

        // Present renderer
        SDL_RenderPresent(renderer);
    }

    // Cleanup
    Mix_FreeMusic(bgMusic);
    Mix_FreeChunk(shootSound);
    Mix_FreeChunk(explosionSound);
    TTF_CloseFont(font);
    SDL_DestroyTexture(playerTexture);
    SDL_DestroyTexture(enemyTexture);
    SDL_DestroyTexture(bulletTexture);
    SDL_DestroyTexture(powerupTexture);
    SDL_DestroyTexture(backgroundTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    Mix_CloseAudio();
    SDL_Quit();

    return 0;
}
