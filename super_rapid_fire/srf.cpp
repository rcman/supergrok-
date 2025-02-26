#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>

// Window dimensions and frame rate
const int WINDOW_WIDTH = 512;
const int WINDOW_HEIGHT = 512;
const int TARGET_FPS = 60;
const float TARGET_DELTA = 1.0f / TARGET_FPS;

// Player structure
struct Player {
    float x = WINDOW_WIDTH / 2;
    float y = WINDOW_HEIGHT - 100;
    float speed = 200.0f; // Pixels per second
    int fireRate = 10;    // Frames between shots
    int fireCounter = 0;
    int shotLevel = 1;    // Power-up level
    int health = 3;
    float width = 32.0f;
    float height = 32.0f;
};

// Bullet structure
struct Bullet {
    float x, y;
    float speed = -600.0f; // Pixels per second (upward)
    bool active = true;
    float width = 8.0f;
    float height = 16.0f;
};

// Enemy structure
struct Enemy {
    float x, y;
    float speed = 100.0f; // Pixels per second
    int health = 1;
    int type; // 0: straight, 1: sine wave
    float angle = 0.0f;
    float startX;
    float width = 32.0f;
    float height = 32.0f;
};

// Power-up structure
struct PowerUp {
    float x, y;
    float speed = 100.0f; // Pixels per second
    bool active = true;
    float width = 16.0f;
    float height = 16.0f;
};

// Function to load textures
SDL_Texture* loadTexture(const char* filepath, SDL_Renderer* renderer) {
    SDL_Surface* surface = SDL_LoadBMP(filepath); // Replace with SDL_image for PNG support
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

// AABB collision detection
bool AABBCollision(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh) {
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

// Update player function
void updatePlayer(Player& player, const Uint8* keys, std::vector<Bullet>& bullets, float delta) {
    if (keys[SDL_SCANCODE_UP] && player.y > 0) player.y -= player.speed * delta;
    if (keys[SDL_SCANCODE_DOWN] && player.y < WINDOW_HEIGHT - player.height) player.y += player.speed * delta;
    if (keys[SDL_SCANCODE_LEFT] && player.x > 0) player.x -= player.speed * delta;
    if (keys[SDL_SCANCODE_RIGHT] && player.x < WINDOW_WIDTH - player.width) player.x += player.speed * delta;

    if (keys[SDL_SCANCODE_SPACE] && player.fireCounter <= 0) {
        if (player.shotLevel == 1) {
            bullets.push_back({player.x + 12, player.y, -600.0f});
        } else if (player.shotLevel == 2) { // Spread shot
            bullets.push_back({player.x + 12, player.y, -600.0f});
            bullets.push_back({player.x + 8, player.y, -480.0f});
            bullets.push_back({player.x + 16, player.y, -480.0f});
        }
        player.fireCounter = player.fireRate;
        // Play shoot sound (commented out for brevity)
    }
    if (player.fireCounter > 0) player.fireCounter--;
}

// Update bullets function
void updateBullets(std::vector<Bullet>& bullets, float delta) {
    for (auto& bullet : bullets) {
        if (bullet.active) {
            bullet.y += bullet.speed * delta;
            if (bullet.y < 0) bullet.active = false;
        }
    }
}

// Update enemies function
void updateEnemies(std::vector<Enemy>& enemies, std::vector<Bullet>& bullets, int& score, float delta) {
    for (auto it = enemies.begin(); it != enemies.end();) {
        if (it->type == 0) {
            it->y += it->speed * delta; // Straight down
        } else {
            it->y += it->speed * delta;
            it->x = it->startX + sin(it->y * 0.05f) * 50.0f; // Sine wave tied to y-position
        }

        // Keep enemies within horizontal bounds
        if (it->x < 0) it->x = 0;
        if (it->x > WINDOW_WIDTH - it->width) it->x = WINDOW_WIDTH - it->width;

        for (auto& bullet : bullets) {
            if (bullet.active && AABBCollision(bullet.x, bullet.y, bullet.width, bullet.height, it->x, it->y, it->width, it->height)) {
                it->health--;
                bullet.active = false;
                // Play explosion sound (commented out for brevity)
            }
        }

        if (it->health <= 0 || it->y > WINDOW_HEIGHT) {
            if (it->health <= 0) score += 100;
            it = enemies.erase(it);
        } else {
            ++it;
        }
    }
}

// Update power-ups function
void updatePowerUps(std::vector<PowerUp>& powerups, Player& player, float delta) {
    for (auto it = powerups.begin(); it != powerups.end();) {
        it->y += it->speed * delta;
        if (AABBCollision(player.x, player.y, player.width, player.height, it->x, it->y, it->width, it->height)) {
            player.shotLevel = 2; // Upgrade to spread shot
            it->active = false;
        }
        if (!it->active || it->y > WINDOW_HEIGHT) {
            it = powerups.erase(it);
        } else {
            ++it;
        }
    }
}

// Render function
void render(SDL_Renderer* renderer, SDL_Texture* playerTexture, SDL_Texture* enemyTexture, SDL_Texture* bulletTexture, SDL_Texture* powerupTexture, SDL_Texture* backgroundTexture, const Player& player, const std::vector<Bullet>& bullets, const std::vector<Enemy>& enemies, const std::vector<PowerUp>& powerups, float backgroundY, int score, TTF_Font* font) {
    // Render background
    SDL_Rect bgRect1 = {0, (int)(backgroundY - 1024), WINDOW_WIDTH, 1024};
    SDL_Rect bgRect2 = {0, (int)backgroundY, WINDOW_WIDTH, 1024};
    SDL_RenderCopy(renderer, backgroundTexture, NULL, &bgRect1);
    SDL_RenderCopy(renderer, backgroundTexture, NULL, &bgRect2);

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
    int score = 0;

    // Enemy spawn timer
    Uint32 enemySpawnTimer = 0;
    const Uint32 spawnInterval = 1000; // Spawn every 1 second (in ms)

    // Game loop
    bool running = true;
    SDL_Event event;
    Uint32 lastTime = SDL_GetTicks();
    Uint32 currentTime;

    while (running) {
        // Calculate delta time
        currentTime = SDL_GetTicks();
        float delta = (currentTime - lastTime) / 1000.0f; // Delta time in seconds
        lastTime = currentTime;

        // Event handling
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
        }

        const Uint8* keys = SDL_GetKeyboardState(NULL);

        // Clear renderer
        SDL_RenderClear(renderer);

        // Update background
        backgroundY += 100.0f * delta; // Scroll speed: 100 pixels per second
        if (backgroundY >= 1024) backgroundY -= 1024; // Loop background

        // Update player
        updatePlayer(player, keys, bullets, delta);

        // Update bullets
        updateBullets(bullets, delta);

        // Spawn enemies and power-ups
        if (SDL_GetTicks() - enemySpawnTimer > spawnInterval) {
            int type = rand() % 2;
            float startX = (float)(rand() % (WINDOW_WIDTH - 32));
            enemies.push_back({startX, 0, 100.0f, 1, type, 0.0f, startX});
            if (rand() % 10 == 0) powerups.push_back({(float)(rand() % (WINDOW_WIDTH - 16)), 0});
            enemySpawnTimer = SDL_GetTicks();
        }

        // Update enemies
        updateEnemies(enemies, bullets, score, delta);

        // Update power-ups
        updatePowerUps(powerups, player, delta);

        // Check for player-enemy collisions
        for (auto& enemy : enemies) {
            if (AABBCollision(player.x, player.y, player.width, player.height, enemy.x, enemy.y, enemy.width, enemy.height)) {
                player.health--;
                enemy.health = 0; // Destroy enemy on collision
            }
        }

        // Check game over condition
        if (player.health <= 0) {
            running = false; // End game if player health reaches zero
        }

        // Render everything
        render(renderer, playerTexture, enemyTexture, bulletTexture, powerupTexture, backgroundTexture, player, bullets, enemies, powerups, backgroundY, score, font);

        // Present renderer
        SDL_RenderPresent(renderer);

        // Cap frame rate to TARGET_FPS
        Uint32 frameTime = SDL_GetTicks() - currentTime;
        if (frameTime < TARGET_DELTA * 1000) {
            SDL_Delay((Uint32)(TARGET_DELTA * 1000 - frameTime));
        }
    }

    // Cleanup
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
