#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <iostream>

// Screen dimensions
const int SCREEN_WIDTH = 1920;
const int SCREEN_HEIGHT = 1080;
const int VIRTUAL_WIDTH = 640;
const int VIRTUAL_HEIGHT = 480;
const float SCALE_FACTOR = 2.25f;
const int OFFSET_X = 240;

// Player properties
const float PLAYER_SPEED = 300.0f;
const int PLAYER_WIDTH = 32;
const int PLAYER_HEIGHT = 32;
const int MAX_HEALTH = 100;

// Bullet properties
const float BULLET_SPEED = 500.0f;
const float ENEMY_BULLET_SPEED = 300.0f;
const int BULLET_WIDTH = 8;
const int BULLET_HEIGHT = 16;

// Enemy properties
const float ENEMY_SPEED = 100.0f;
const int ENEMY_WIDTH = 32;
const int ENEMY_HEIGHT = 32;

// Power-up properties
const int POWERUP_WIDTH = 16;
const int POWERUP_HEIGHT = 16;

enum PowerUpType {
    POWERUP_SPEED,
    POWERUP_SHOT,
    POWERUP_HEALTH,
    POWERUP_COUNT
};

struct Entity {
    float x, y;
    int w, h;
    SDL_Texture* texture;
};

struct Player : Entity {
    int shootCooldown;
    int powerLevel;     // 0 = single, 1 = double, 2 = triple
    float speedBoost;   // Speed multiplier
    int health;
    float speedTimer;   // Duration of speed boost
};

struct Bullet : Entity {
    bool active;
    bool isEnemyBullet; // True if fired by enemy
};

struct Enemy : Entity {
    bool active;
    int shootCooldown;
};

struct PowerUp : Entity {
    bool active;
    PowerUpType type;
};

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

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0 || IMG_Init(IMG_INIT_PNG) == 0 || 
        Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0 || TTF_Init() < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("Super Rapid Fire Clone", SDL_WINDOWPOS_UNDEFINED, 
        SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // Load assets
    SDL_Texture* playerTexture = loadTexture("player.png", renderer);
    SDL_Texture* bulletTexture = loadTexture("bullet.png", renderer);
    SDL_Texture* enemyTexture = loadTexture("enemy.png", renderer);
    SDL_Texture* powerUpTextures[POWERUP_COUNT] = {
        loadTexture("powerup_speed.png", renderer),    // Speed boost
        loadTexture("powerup_shot.png", renderer),     // Better shot
        loadTexture("powerup_health.png", renderer)    // Health increase
    };
    SDL_Texture* bgTexture = loadTexture("background.png", renderer);
    Mix_Chunk* shootSound = Mix_LoadWAV("shoot.wav");
    Mix_Chunk* explosionSound = Mix_LoadWAV("explosion.wav");
    TTF_Font* font = TTF_OpenFont("arial.ttf", 24);

    // Initialize player
    Player player = { VIRTUAL_WIDTH / 2.0f - PLAYER_WIDTH / 2.0f, VIRTUAL_HEIGHT - PLAYER_HEIGHT - 20, 
        PLAYER_WIDTH, PLAYER_HEIGHT, playerTexture, 10, 0, 1.0f, MAX_HEALTH, 0.0f };

    std::vector<Bullet> bullets;
    std::vector<Enemy> enemies;
    std::vector<PowerUp> powerUps;
    float bgY = 0.0f;
    int score = 0;
    int enemySpawnTimer = 0;

    bool quit = false;
    SDL_Event e;
    Uint32 lastTime = SDL_GetTicks();
    const Uint8* keyboardState = SDL_GetKeyboardState(NULL);

    while (!quit) {
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) quit = true;
        }

        // Player movement
        float currentSpeed = PLAYER_SPEED * player.speedBoost;
        if (keyboardState[SDL_SCANCODE_LEFT]) player.x -= currentSpeed * deltaTime;
        if (keyboardState[SDL_SCANCODE_RIGHT]) player.x += currentSpeed * deltaTime;
        if (player.x < 0) player.x = 0;
        if (player.x + PLAYER_WIDTH > VIRTUAL_WIDTH) player.x = VIRTUAL_WIDTH - PLAYER_WIDTH;

        // Shooting
        if (keyboardState[SDL_SCANCODE_SPACE] && player.shootCooldown <= 0) {
            Bullet bullet = { player.x + PLAYER_WIDTH / 2 - BULLET_WIDTH / 2, player.y - BULLET_HEIGHT, 
                BULLET_WIDTH, BULLET_HEIGHT, bulletTexture, true, false };
            bullets.push_back(bullet);
            if (player.powerLevel >= 1) { // Double shot
                Bullet bullet2 = { player.x + PLAYER_WIDTH / 2 - BULLET_WIDTH / 2 - 20, player.y - BULLET_HEIGHT, 
                    BULLET_WIDTH, BULLET_HEIGHT, bulletTexture, true, false };
                bullets.push_back(bullet2);
            }
            if (player.powerLevel >= 2) { // Triple shot
                Bullet bullet3 = { player.x + PLAYER_WIDTH / 2 - BULLET_WIDTH / 2 + 20, player.y - BULLET_HEIGHT, 
                    BULLET_WIDTH, BULLET_HEIGHT, bulletTexture, true, false };
                bullets.push_back(bullet3);
            }
            Mix_PlayChannel(-1, shootSound, 0);
            player.shootCooldown = 10;
        }
        if (player.shootCooldown > 0) player.shootCooldown--;

        // Update speed boost timer
        if (player.speedTimer > 0) {
            player.speedTimer -= deltaTime;
            if (player.speedTimer <= 0) player.speedBoost = 1.0f;
        }

        // Update bullets
        for (auto& bullet : bullets) {
            if (!bullet.active) continue;
            float speed = bullet.isEnemyBullet ? ENEMY_BULLET_SPEED : BULLET_SPEED;
            bullet.y += (bullet.isEnemyBullet ? 1 : -1) * speed * deltaTime;
            if (bullet.y + BULLET_HEIGHT < 0 || bullet.y > VIRTUAL_HEIGHT) bullet.active = false;
        }

        // Spawn enemies
        enemySpawnTimer--;
        if (enemySpawnTimer <= 0) {
            Enemy enemy = { static_cast<float>(rand() % (VIRTUAL_WIDTH - ENEMY_WIDTH)), -ENEMY_HEIGHT, 
                ENEMY_WIDTH, ENEMY_HEIGHT, enemyTexture, true, 60 };
            enemies.push_back(enemy);
            enemySpawnTimer = 30 + (rand() % 20);
        }

        // Update enemies
        for (auto& enemy : enemies) {
            if (!enemy.active) continue;
            enemy.y += ENEMY_SPEED * deltaTime;
            if (enemy.y > VIRTUAL_HEIGHT) enemy.active = false;

            // Enemy shooting
            if (enemy.shootCooldown <= 0) {
                Bullet bullet = { enemy.x + ENEMY_WIDTH / 2 - BULLET_WIDTH / 2, enemy.y + ENEMY_HEIGHT, 
                    BULLET_WIDTH, BULLET_HEIGHT, bulletTexture, true, true };
                bullets.push_back(bullet);
                enemy.shootCooldown = 60 + (rand() % 30);
            }
            enemy.shootCooldown--;

            // Collision with player
            SDL_Rect playerRect = { static_cast<int>(player.x), static_cast<int>(player.y), PLAYER_WIDTH, PLAYER_HEIGHT };
            SDL_Rect enemyRect = { static_cast<int>(enemy.x), static_cast<int>(enemy.y), ENEMY_WIDTH, ENEMY_HEIGHT };
            if (SDL_HasIntersection(&playerRect, &enemyRect) && player.health > 0) {
                player.health -= 20;
                enemy.active = false;
                Mix_PlayChannel(-1, explosionSound, 0);
            }

            // Collision with player bullets
            for (auto& bullet : bullets) {
                if (!bullet.active || bullet.isEnemyBullet) continue;
                SDL_Rect bulletRect = { static_cast<int>(bullet.x), static_cast<int>(bullet.y), BULLET_WIDTH, BULLET_HEIGHT };
                if (SDL_HasIntersection(&bulletRect, &enemyRect)) {
                    bullet.active = false;
                    enemy.active = false;
                    Mix_PlayChannel(-1, explosionSound, 0);
                    score += 10;
                    if (rand() % 100 < 20) {
                        PowerUp powerUp = { enemy.x, enemy.y, POWERUP_WIDTH, POWERUP_HEIGHT, 
                            powerUpTextures[rand() % POWERUP_COUNT], true, static_cast<PowerUpType>(rand() % POWERUP_COUNT) };
                        powerUps.push_back(powerUp);
                    }
                }
            }
        }

        // Check player collision with enemy bullets
        SDL_Rect playerRect = { static_cast<int>(player.x), static_cast<int>(player.y), PLAYER_WIDTH, PLAYER_HEIGHT };
        for (auto& bullet : bullets) {
            if (!bullet.active || !bullet.isEnemyBullet) continue;
            SDL_Rect bulletRect = { static_cast<int>(bullet.x), static_cast<int>(bullet.y), BULLET_WIDTH, BULLET_HEIGHT };
            if (SDL_HasIntersection(&playerRect, &bulletRect) && player.health > 0) {
                player.health -= 10;
                bullet.active = false;
            }
        }

        // Update power-ups
        for (auto& powerUp : powerUps) {
            if (!powerUp.active) continue;
            powerUp.y += ENEMY_SPEED * deltaTime;
            if (powerUp.y > VIRTUAL_HEIGHT) powerUp.active = false;

            SDL_Rect powerUpRect = { static_cast<int>(powerUp.x), static_cast<int>(powerUp.y), POWERUP_WIDTH, POWERUP_HEIGHT };
            if (SDL_HasIntersection(&powerUpRect, &playerRect)) {
                powerUp.active = false;
                switch (powerUp.type) {
                    case POWERUP_SPEED:
                        player.speedBoost = 1.5f;
                        player.speedTimer = 5.0f; // 5 seconds duration
                        break;
                    case POWERUP_SHOT:
                        if (player.powerLevel < 2) player.powerLevel++;
                        break;
                    case POWERUP_HEALTH:
                        player.health = std::min(MAX_HEALTH, player.health + 25);
                        break;
                }
            }
        }

        if (player.health <= 0) {
            std::cout << "Game Over! Score: " << score << std::endl;
            quit = true;
        }

        // Scroll background
        bgY += 100.0f * deltaTime;
        if (bgY >= VIRTUAL_HEIGHT) bgY -= VIRTUAL_HEIGHT;

        // Render
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_Rect bgSrc1 = { 0, static_cast<int>(bgY), VIRTUAL_WIDTH, VIRTUAL_HEIGHT - static_cast<int>(bgY) };
        SDL_Rect bgDst1 = { OFFSET_X, 0, static_cast<int>(VIRTUAL_WIDTH * SCALE_FACTOR), static_cast<int>((VIRTUAL_HEIGHT - bgY) * SCALE_FACTOR) };
        SDL_Rect bgSrc2 = { 0, 0, VIRTUAL_WIDTH, static_cast<int>(bgY) };
        SDL_Rect bgDst2 = { OFFSET_X, static_cast<int>((VIRTUAL_HEIGHT - bgY) * SCALE_FACTOR), static_cast<int>(VIRTUAL_WIDTH * SCALE_FACTOR), static_cast<int>(bgY * SCALE_FACTOR) };
        SDL_RenderCopy(renderer, bgTexture, &bgSrc1, &bgDst1);
        SDL_RenderCopy(renderer, bgTexture, &bgSrc2, &bgDst2);

        SDL_Rect playerDst = { static_cast<int>(player.x * SCALE_FACTOR) + OFFSET_X, static_cast<int>(player.y * SCALE_FACTOR), 
            static_cast<int>(PLAYER_WIDTH * SCALE_FACTOR), static_cast<int>(PLAYER_HEIGHT * SCALE_FACTOR) };
        SDL_RenderCopy(renderer, player.texture, NULL, &playerDst);

        for (const auto& bullet : bullets) {
            if (bullet.active) {
                SDL_Rect bulletDst = { static_cast<int>(bullet.x * SCALE_FACTOR) + OFFSET_X, static_cast<int>(bullet.y * SCALE_FACTOR), 
                    static_cast<int>(BULLET_WIDTH * SCALE_FACTOR), static_cast<int>(BULLET_HEIGHT * SCALE_FACTOR) };
                SDL_RenderCopy(renderer, bullet.texture, NULL, &bulletDst);
            }
        }

        for (const auto& enemy : enemies) {
            if (enemy.active) {
                SDL_Rect enemyDst = { static_cast<int>(enemy.x * SCALE_FACTOR) + OFFSET_X, static_cast<int>(enemy.y * SCALE_FACTOR), 
                    static_cast<int>(ENEMY_WIDTH * SCALE_FACTOR), static_cast<int>(ENEMY_HEIGHT * SCALE_FACTOR) };
                SDL_RenderCopy(renderer, enemy.texture, NULL, &enemyDst);
            }
        }

        for (const auto& powerUp : powerUps) {
            if (powerUp.active) {
                SDL_Rect powerUpDst = { static_cast<int>(powerUp.x * SCALE_FACTOR) + OFFSET_X, static_cast<int>(powerUp.y * SCALE_FACTOR), 
                    static_cast<int>(POWERUP_WIDTH * SCALE_FACTOR), static_cast<int>(POWERUP_HEIGHT * SCALE_FACTOR) };
                SDL_RenderCopy(renderer, powerUp.texture, NULL, &powerUpDst);
            }
        }

        // Render health bar
        SDL_Rect healthBg = { OFFSET_X + 10, 40, 200, 20 };
        SDL_Rect healthFill = { OFFSET_X + 10, 40, static_cast<int>(200 * (player.health / (float)MAX_HEALTH)), 20 };
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &healthBg);
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_RenderFillRect(renderer, &healthFill);

        // Render score
        std::string scoreText = "Score: " + std::to_string(score);
        SDL_Surface* textSurface = TTF_RenderText_Solid(font, scoreText.c_str(), {255, 255, 255});
        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        SDL_Rect textDst = { OFFSET_X + 10, 10, textSurface->w, textSurface->h };
        SDL_RenderCopy(renderer, textTexture, NULL, &textDst);
        SDL_FreeSurface(textSurface);
        SDL_DestroyTexture(textTexture);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    // Cleanup
    SDL_DestroyTexture(playerTexture);
    SDL_DestroyTexture(bulletTexture);
    SDL_DestroyTexture(enemyTexture);
    for (int i = 0; i < POWERUP_COUNT; i++) SDL_DestroyTexture(powerUpTextures[i]);
    SDL_DestroyTexture(bgTexture);
    Mix_FreeChunk(shootSound);
    Mix_FreeChunk(explosionSound);
    TTF_CloseFont(font);
    Mix_CloseAudio();
    TTF_Quit();
    IMG_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
