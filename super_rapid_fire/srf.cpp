#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <iostream>

// Screen dimensions
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

// Player properties
const float PLAYER_SPEED = 300.0f;
const int PLAYER_WIDTH = 32;
const int PLAYER_HEIGHT = 32;

// Bullet properties
const float BULLET_SPEED = 500.0f;
const int BULLET_WIDTH = 8;
const int BULLET_HEIGHT = 16;

// Enemy properties
const float ENEMY_SPEED = 100.0f;
const int ENEMY_WIDTH = 32;
const int ENEMY_HEIGHT = 32;

// Power-up properties
const int POWERUP_WIDTH = 16;
const int POWERUP_HEIGHT = 16;

struct Entity {
    float x, y;
    int w, h;
    SDL_Texture* texture;
};

struct Player : Entity {
    int shootCooldown;
    int powerLevel; // 0 = single shot, 1 = double shot
};

struct Bullet : Entity {
    bool active;
};

struct Enemy : Entity {
    bool active;
};

struct PowerUp : Entity {
    bool active;
};

// Load texture helper
SDL_Texture* loadTexture(const std::string& path, SDL_Renderer* renderer) {
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) {
        std::cerr << "Failed to load image: " << IMG_GetError() << std::endl;
        return nullptr;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

int main(int argc, char* argv[]) {
    srand(time(NULL));

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0 || IMG_Init(IMG_INIT_PNG) == 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("Super Rapid Fire Clone", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // Load assets (replace with your own PNGs)
    SDL_Texture* playerTexture = loadTexture("player.png", renderer); // 32x32px ship
    SDL_Texture* bulletTexture = loadTexture("bullet.png", renderer); // 8x16px bullet
    SDL_Texture* enemyTexture = loadTexture("enemy.png", renderer);   // 32x32px enemy
    SDL_Texture* powerUpTexture = loadTexture("powerup.png", renderer); // 16x16px power-up
    SDL_Texture* bgTexture = loadTexture("background.png", renderer); // 640x960px scrolling bg

    // Initialize player
    Player player = { SCREEN_WIDTH / 2.0f - PLAYER_WIDTH / 2.0f, SCREEN_HEIGHT - PLAYER_HEIGHT - 20, PLAYER_WIDTH, PLAYER_HEIGHT, playerTexture, 10, 0 };

    // Game objects
    std::vector<Bullet> bullets;
    std::vector<Enemy> enemies;
    std::vector<PowerUp> powerUps;
    float bgY = 0.0f; // Background scroll position
    int score = 0;
    int enemySpawnTimer = 0;

    // Game loop
    bool quit = false;
    SDL_Event e;
    Uint32 lastTime = SDL_GetTicks();
    const Uint8* keyboardState = SDL_GetKeyboardState(NULL);

    while (!quit) {
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        // Input handling
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) quit = true;
        }

        // Player movement
        if (keyboardState[SDL_SCANCODE_LEFT]) player.x -= PLAYER_SPEED * deltaTime;
        if (keyboardState[SDL_SCANCODE_RIGHT]) player.x += PLAYER_SPEED * deltaTime;
        if (player.x < 0) player.x = 0;
        if (player.x + PLAYER_WIDTH > SCREEN_WIDTH) player.x = SCREEN_WIDTH - PLAYER_WIDTH;

        // Shooting
        if (keyboardState[SDL_SCANCODE_SPACE] && player.shootCooldown <= 0) {
            Bullet bullet = { player.x + PLAYER_WIDTH / 2 - BULLET_WIDTH / 2, player.y - BULLET_HEIGHT, BULLET_WIDTH, BULLET_HEIGHT, bulletTexture, true };
            bullets.push_back(bullet);
            if (player.powerLevel >= 1) { // Double shot
                Bullet bullet2 = { player.x + PLAYER_WIDTH / 2 - BULLET_WIDTH / 2 - 20, player.y - BULLET_HEIGHT, BULLET_WIDTH, BULLET_HEIGHT, bulletTexture, true };
                bullets.push_back(bullet2);
            }
            player.shootCooldown = 10; // Frames cooldown
        }
        if (player.shootCooldown > 0) player.shootCooldown--;

        // Update bullets
        for (auto& bullet : bullets) {
            if (!bullet.active) continue;
            bullet.y -= BULLET_SPEED * deltaTime;
            if (bullet.y + BULLET_HEIGHT < 0) bullet.active = false;
        }

        // Spawn enemies
        enemySpawnTimer--;
        if (enemySpawnTimer <= 0) {
            Enemy enemy = { static_cast<float>(rand() % (SCREEN_WIDTH - ENEMY_WIDTH)), -ENEMY_HEIGHT, ENEMY_WIDTH, ENEMY_HEIGHT, enemyTexture, true };
            enemies.push_back(enemy);
            enemySpawnTimer = 30 + (rand() % 20); // Random spawn interval
        }

        // Update enemies
        for (auto& enemy : enemies) {
            if (!enemy.active) continue;
            enemy.y += ENEMY_SPEED * deltaTime;
            if (enemy.y > SCREEN_HEIGHT) enemy.active = false;

            // Collision with player
            SDL_Rect playerRect = { static_cast<int>(player.x), static_cast<int>(player.y), PLAYER_WIDTH, PLAYER_HEIGHT };
            SDL_Rect enemyRect = { static_cast<int>(enemy.x), static_cast<int>(enemy.y), ENEMY_WIDTH, ENEMY_HEIGHT };
            if (SDL_HasIntersection(&playerRect, &enemyRect)) {
                std::cout << "Game Over! Score: " << score << std::endl;
                quit = true;
            }

            // Collision with bullets
            for (auto& bullet : bullets) {
                if (!bullet.active) continue;
                SDL_Rect bulletRect = { static_cast<int>(bullet.x), static_cast<int>(bullet.y), BULLET_WIDTH, BULLET_HEIGHT };
                if (SDL_HasIntersection(&bulletRect, &enemyRect)) {
                    bullet.active = false;
                    enemy.active = false;
                    score += 10;

                    // 10% chance to spawn power-up
                    if (rand() % 100 < 10) {
                        PowerUp powerUp = { enemy.x, enemy.y, POWERUP_WIDTH, POWERUP_HEIGHT, powerUpTexture, true };
                        powerUps.push_back(powerUp);
                    }
                }
            }
        }

        // Update power-ups
        for (auto& powerUp : powerUps) {
            if (!powerUp.active) continue;
            powerUp.y += ENEMY_SPEED * deltaTime;
            if (powerUp.y > SCREEN_HEIGHT) powerUp.active = false;

            SDL_Rect powerUpRect = { static_cast<int>(powerUp.x), static_cast<int>(powerUp.y), POWERUP_WIDTH, POWERUP_HEIGHT };
            SDL_Rect playerRect = { static_cast<int>(player.x), static_cast<int>(player.y), PLAYER_WIDTH, PLAYER_HEIGHT };
            if (SDL_HasIntersection(&powerUpRect, &playerRect)) {
                powerUp.active = false;
                if (player.powerLevel < 1) player.powerLevel++; // Upgrade to double shot
            }
        }

        // Scroll background
        bgY += 100.0f * deltaTime; // Scroll speed
        if (bgY >= SCREEN_HEIGHT) bgY -= SCREEN_HEIGHT; // Seamless loop (bg is 640x960px)

        // Render
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Render background (two copies for seamless scrolling)
        SDL_Rect bgRect1 = { 0, static_cast<int>(bgY - SCREEN_HEIGHT), SCREEN_WIDTH, SCREEN_HEIGHT };
        SDL_Rect bgRect2 = { 0, static_cast<int>(bgY), SCREEN_WIDTH, SCREEN_HEIGHT };
        SDL_RenderCopy(renderer, bgTexture, NULL, &bgRect1);
        SDL_RenderCopy(renderer, bgTexture, NULL, &bgRect2);

        // Render player
        SDL_Rect playerRect = { static_cast<int>(player.x), static_cast<int>(player.y), PLAYER_WIDTH, PLAYER_HEIGHT };
        SDL_RenderCopy(renderer, player.texture, NULL, &playerRect);

        // Render bullets
        for (const auto& bullet : bullets) {
            if (bullet.active) {
                SDL_Rect bulletRect = { static_cast<int>(bullet.x), static_cast<int>(bullet.y), BULLET_WIDTH, BULLET_HEIGHT };
                SDL_RenderCopy(renderer, bullet.texture, NULL, &bulletRect);
            }
        }

        // Render enemies
        for (const auto& enemy : enemies) {
            if (enemy.active) {
                SDL_Rect enemyRect = { static_cast<int>(enemy.x), static_cast<int>(enemy.y), ENEMY_WIDTH, ENEMY_HEIGHT };
                SDL_RenderCopy(renderer, enemy.texture, NULL, &enemyRect);
            }
        }

        // Render power-ups
        for (const auto& powerUp : powerUps) {
            if (powerUp.active) {
                SDL_Rect powerUpRect = { static_cast<int>(powerUp.x), static_cast<int>(powerUp.y), POWERUP_WIDTH, POWERUP_HEIGHT };
                SDL_RenderCopy(renderer, powerUp.texture, NULL, &powerUpRect);
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60 FPS
    }

    // Cleanup
    SDL_DestroyTexture(playerTexture);
    SDL_DestroyTexture(bulletTexture);
    SDL_DestroyTexture(enemyTexture);
    SDL_DestroyTexture(powerUpTexture);
    SDL_DestroyTexture(bgTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return 0;
}
