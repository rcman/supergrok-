#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <cmath>
#include <algorithm>

// Screen dimensions (native resolution)
const int SCREEN_WIDTH = 1920;
const int SCREEN_HEIGHT = 1080;

// Player properties (scaled up from 32x32)
const float PLAYER_SPEED = 675.0f; // Adjusted from 300 * 2.25
const int PLAYER_WIDTH = 72;       // 32 * 2.25
const int PLAYER_HEIGHT = 72;

// Bullet properties (scaled up from 8x16)
const float BULLET_SPEED = 1125.0f; // Adjusted from 500 * 2.25
const int BULLET_WIDTH = 18;        // 8 * 2.25
const int BULLET_HEIGHT = 36;

// Enemy properties (scaled up from 32x32)
const int ENEMY_WIDTH = 72;  // 32 * 2.25
const int ENEMY_HEIGHT = 72;

// Power-up properties (scaled up from 16x16)
const int POWERUP_WIDTH = 36;  // 16 * 2.25
const int POWERUP_HEIGHT = 36;

enum EnemyType {
    STRAIGHT,
    ZIGZAG,
    SINE,
    CIRCULAR,
    DIAGONAL,
    FAST,
    SPIRAL,
    COUNT
};

enum PowerUpType {
    SHIELD,
    HEALTH_INCREASE,
    FULL_HEALTH,
    ADDITIONAL_BULLETS,
    NUKE,
    POWERUP_BULLET_SPEED
};

struct Entity {
    float x, y;
    int w, h;
    SDL_Texture* texture;
};

struct Player : Entity {
    int shootCooldown;
    int powerLevel;
    int lives;
    int level;
    int health;
    int hiScore;
    bool shieldActive;
    Uint32 shieldTimer;
    bool extraBulletsActive;
    Uint32 extraBulletsTimer;
    bool bulletSpeedActive;
    Uint32 bulletSpeedTimer;
    float originalBulletSpeed;
};

struct Bullet : Entity {
    bool active;
};

struct Enemy : Entity {
    bool active;
    EnemyType type;
    float speed;
    float dx, dy;
    float angle;
    float amplitude;
    float startX;
};

struct PowerUp : Entity {
    bool active;
    PowerUpType type;
    Uint32 timer;
};

// Texture loading with error handling
SDL_Texture* loadTexture(const std::string& path, SDL_Renderer* renderer) {
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) {
        std::cerr << "Failed to load image: " << path << " - " << IMG_GetError() << std::endl;
        return nullptr;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (!texture) {
        std::cerr << "Failed to create texture from " << path << ": " << SDL_GetError() << std::endl;
    }
    return texture;
}

// Circle drawing function
void drawCircle(SDL_Renderer* renderer, int centerX, int centerY, int radius) {
    for (int w = 0; w < radius * 2; w++) {
        for (int h = 0; h < radius * 2; h++) {
            int dx = radius - w;
            int dy = radius - h;
            if ((dx * dx + dy * dy) <= (radius * radius)) {
                SDL_RenderDrawPoint(renderer, centerX + dx, centerY + dy);
            }
        }
    }
}

int main(int argc, char* argv[]) {
    srand(static_cast<unsigned>(time(nullptr)));

    // Initialize SDL subsystems
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return -1;
    }
    if (IMG_Init(IMG_INIT_PNG) == 0) {
        std::cerr << "IMG_Init failed: " << IMG_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "Mixer initialization failed: " << Mix_GetError() << std::endl;
        IMG_Quit();
        SDL_Quit();
        return -1;
    }
    if (TTF_Init() < 0) {
        std::cerr << "TTF initialization failed: " << TTF_GetError() << std::endl;
        Mix_CloseAudio();
        IMG_Quit();
        SDL_Quit();
        return -1;
    }

    // Create window and renderer
    SDL_Window* window = SDL_CreateWindow("Super Rapid Fire Clone", SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        TTF_Quit();
        Mix_CloseAudio();
        IMG_Quit();
        SDL_Quit();
        return -1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        TTF_Quit();
        Mix_CloseAudio();
        IMG_Quit();
        SDL_Quit();
        return -1;
    }

    // Load assets with null checks
    SDL_Texture* playerTexture = loadTexture("player.png", renderer);
    SDL_Texture* bulletTexture = loadTexture("bullet.png", renderer);
    SDL_Texture* enemyTextures[EnemyType::COUNT] = {
        loadTexture("enemy1.png", renderer),
        loadTexture("enemy2.png", renderer),
        loadTexture("enemy3.png", renderer),
        loadTexture("enemy4.png", renderer),
        loadTexture("enemy5.png", renderer),
        loadTexture("enemy6.png", renderer),
        loadTexture("enemy7.png", renderer)
    };
    SDL_Texture* shieldTexture = loadTexture("shield.png", renderer);
    SDL_Texture* healthIncreaseTexture = loadTexture("health_increase.png", renderer);
    SDL_Texture* fullHealthTexture = loadTexture("full_health.png", renderer);
    SDL_Texture* additionalBulletsTexture = loadTexture("additional_bullets.png", renderer);
    SDL_Texture* nukeTexture = loadTexture("nuke.png", renderer);
    SDL_Texture* bulletSpeedTexture = loadTexture("bullet_speed.png", renderer);
    SDL_Texture* bgTexture = loadTexture("background.png", renderer);
    Mix_Chunk* shootSound = Mix_LoadWAV("shoot.wav");
    Mix_Chunk* explosionSound = Mix_LoadWAV("explosion.wav");
    TTF_Font* font = TTF_OpenFont("arial.ttf", 24);

    // Check critical assets and exit if any fail
    if (!playerTexture || !bulletTexture || !bgTexture || !shootSound || !explosionSound || !font) {
        std::cerr << "Failed to load critical assets" << std::endl;
        for (int i = 0; i < EnemyType::COUNT; i++) if (enemyTextures[i]) SDL_DestroyTexture(enemyTextures[i]);
        if (playerTexture) SDL_DestroyTexture(playerTexture);
        if (bulletTexture) SDL_DestroyTexture(bulletTexture);
        if (bgTexture) SDL_DestroyTexture(bgTexture);
        if (shootSound) Mix_FreeChunk(shootSound);
        if (explosionSound) Mix_FreeChunk(explosionSound);
        if (font) TTF_CloseFont(font);
        if (shieldTexture) SDL_DestroyTexture(shieldTexture);
        if (healthIncreaseTexture) SDL_DestroyTexture(healthIncreaseTexture);
        if (fullHealthTexture) SDL_DestroyTexture(fullHealthTexture);
        if (additionalBulletsTexture) SDL_DestroyTexture(additionalBulletsTexture);
        if (nukeTexture) SDL_DestroyTexture(nukeTexture);
        if (bulletSpeedTexture) SDL_DestroyTexture(bulletSpeedTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        Mix_CloseAudio();
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return -1;
    }

    // Initialize game state with adjusted positions for 1920x1080
    Player player = {
        SCREEN_WIDTH / 2.0f - PLAYER_WIDTH / 2.0f,
        SCREEN_HEIGHT - PLAYER_HEIGHT - 45,  // Scaled from 20 * 2.25
        PLAYER_WIDTH, PLAYER_HEIGHT,
        playerTexture,
        10, 0, 3, 1, 100, 0,
        false, 0,
        false, 0,
        false, 0,
        BULLET_SPEED
    };

    std::vector<Bullet> bullets;
    std::vector<Enemy> enemies;
    std::vector<PowerUp> powerUps;
    float bgY = 0.0f;
    int score = 0;
    int enemySpawnTimer = 0;
    bool gameOver = false;

    bool quit = false;
    SDL_Event e;
    Uint32 lastTime = SDL_GetTicks();
    const Uint8* keyboardState = SDL_GetKeyboardState(nullptr);

    while (!quit) {
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) quit = true;
        }

        if (!gameOver) {
            // Player movement
            if (keyboardState[SDL_SCANCODE_LEFT]) player.x -= PLAYER_SPEED * deltaTime;
            if (keyboardState[SDL_SCANCODE_RIGHT]) player.x += PLAYER_SPEED * deltaTime;
            if (keyboardState[SDL_SCANCODE_UP]) player.y -= PLAYER_SPEED * deltaTime;
            if (keyboardState[SDL_SCANCODE_DOWN]) player.y += PLAYER_SPEED * deltaTime;
            player.x = std::max(0.0f, std::min(player.x, static_cast<float>(SCREEN_WIDTH - PLAYER_WIDTH)));
            player.y = std::max(0.0f, std::min(player.y, static_cast<float>(SCREEN_HEIGHT - PLAYER_HEIGHT)));

            // Update power-up timers
            if (player.shieldActive && currentTime - player.shieldTimer >= 60000) player.shieldActive = false;
            if (player.extraBulletsActive && currentTime - player.extraBulletsTimer >= 60000) {
                player.extraBulletsActive = false;
                player.powerLevel = 0;
            }
            if (player.bulletSpeedActive && currentTime - player.bulletSpeedTimer >= 60000) player.bulletSpeedActive = false;

            // Shooting
            if (keyboardState[SDL_SCANCODE_SPACE] && player.shootCooldown <= 0) {
                Bullet bullet = { player.x + PLAYER_WIDTH / 2 - BULLET_WIDTH / 2, player.y - BULLET_HEIGHT,
                                  BULLET_WIDTH, BULLET_HEIGHT, bulletTexture, true };
                bullets.push_back(bullet);
                if (player.powerLevel >= 1 || player.extraBulletsActive) {
                    Bullet bulletLeft = { player.x + PLAYER_WIDTH / 2 - BULLET_WIDTH / 2 - 45, player.y - BULLET_HEIGHT,
                                          BULLET_WIDTH, BULLET_HEIGHT, bulletTexture, true };
                    Bullet bulletRight = { player.x + PLAYER_WIDTH / 2 - BULLET_WIDTH / 2 + 45, player.y - BULLET_HEIGHT,
                                           BULLET_WIDTH, BULLET_HEIGHT, bulletTexture, true };
                    bullets.push_back(bulletLeft);
                    bullets.push_back(bulletRight);
                }
                if (shootSound) Mix_PlayChannel(-1, shootSound, 0);
                player.shootCooldown = player.bulletSpeedActive ? 5 : 10;
            }
            if (player.shootCooldown > 0) player.shootCooldown--;

            // Update bullets
            float currentBulletSpeed = player.bulletSpeedActive ? player.originalBulletSpeed * 2 : player.originalBulletSpeed;
            for (auto& bullet : bullets) {
                if (bullet.active) {
                    bullet.y -= currentBulletSpeed * deltaTime;
                    if (bullet.y + BULLET_HEIGHT < 0) bullet.active = false;
                }
            }

            // Spawn enemies off the top of the screen
            enemySpawnTimer--;
            if (enemySpawnTimer <= 0) {
                EnemyType type = static_cast<EnemyType>(rand() % EnemyType::COUNT);
                float startX = static_cast<float>(rand() % (SCREEN_WIDTH - ENEMY_WIDTH)); // Spawn within screen width
                Enemy enemy = { startX, -ENEMY_HEIGHT, ENEMY_WIDTH, ENEMY_HEIGHT, enemyTextures[type], true, type, 0.0f, 0.0f, 0.0f, 0.0f, startX }; // y = -ENEMY_HEIGHT
                switch (type) {
                    case STRAIGHT: 
                        enemy.speed = 225.0f; 
                        enemy.dy = enemy.speed; 
                        break;
                    case ZIGZAG:
                        enemy.speed = 337.5f;
                        enemy.dx = (startX < SCREEN_WIDTH / 2) ? 225.0f : -225.0f;
                        enemy.dy = enemy.speed;
                        enemy.amplitude = 112.5f;
                        break;
                    case SINE:
                        enemy.speed = 270.0f;
                        enemy.dy = enemy.speed;
                        enemy.amplitude = 168.75f;
                        enemy.angle = 0.0f;
                        break;
                    case CIRCULAR:
                        enemy.speed = 4.5f;
                        enemy.angle = 0.0f;
                        enemy.amplitude = 225.0f;
                        enemy.dy = 225.0f; // Add downward movement
                        break;
                    case DIAGONAL:
                        enemy.speed = 292.5f;
                        enemy.dx = (startX < SCREEN_WIDTH / 2) ? enemy.speed * 0.5f : -enemy.speed * 0.5f;
                        enemy.dy = enemy.speed;
                        break;
                    case FAST: 
                        enemy.speed = 450.0f; 
                        enemy.dy = enemy.speed; 
                        break;
                    case SPIRAL:
                        enemy.speed = 3.375f;
                        enemy.angle = 0.0f;
                        enemy.amplitude = 337.5f;
                        enemy.dy = 225.0f; // Add downward movement
                        break;
                }
                enemies.push_back(enemy);
                enemySpawnTimer = 30 + (rand() % 20);
            }

            // Update enemies
            for (auto& enemy : enemies) {
                if (!enemy.active) continue;
                switch (enemy.type) {
                    case STRAIGHT:
                    case FAST:
                        enemy.y += enemy.dy * deltaTime;
                        break;
                    case ZIGZAG:
                        enemy.x += enemy.dx * deltaTime;
                        enemy.y += enemy.dy * deltaTime;
                        if (enemy.x <= 0 || enemy.x + ENEMY_WIDTH >= SCREEN_WIDTH) enemy.dx = -enemy.dx;
                        break;
                    case SINE:
                        enemy.angle += enemy.speed * deltaTime * 0.05f;
                        enemy.x = enemy.startX + enemy.amplitude * sin(enemy.angle);
                        enemy.y += enemy.dy * deltaTime;
                        enemy.x = std::max(0.0f, std::min(enemy.x, static_cast<float>(SCREEN_WIDTH - ENEMY_WIDTH)));
                        break;
                    case CIRCULAR:
                        enemy.angle += enemy.speed * deltaTime;
                        enemy.x = enemy.startX + enemy.amplitude * cos(enemy.angle);
                        enemy.y += enemy.dy * deltaTime; // Move downward
                        break;
                    case DIAGONAL:
                        enemy.x += enemy.dx * deltaTime;
                        enemy.y += enemy.dy * deltaTime;
                        break;
                    case SPIRAL:
                        enemy.angle += enemy.speed * deltaTime;
                        enemy.amplitude -= enemy.speed * deltaTime * 10;
                        enemy.x = enemy.startX + enemy.amplitude * cos(enemy.angle);
                        enemy.y += enemy.dy * deltaTime; // Move downward
                        break;
                }
                if (enemy.y > SCREEN_HEIGHT || enemy.x < -ENEMY_WIDTH || enemy.x > SCREEN_WIDTH ||
                    (enemy.type == SPIRAL && enemy.amplitude <= 10)) {
                    enemy.active = false;
                }

                // Collision with player
                if (!player.shieldActive) {
                    SDL_Rect playerRect = { static_cast<int>(player.x), static_cast<int>(player.y), PLAYER_WIDTH, PLAYER_HEIGHT };
                    SDL_Rect enemyRect = { static_cast<int>(enemy.x), static_cast<int>(enemy.y), ENEMY_WIDTH, ENEMY_HEIGHT };
                    if (SDL_HasIntersection(&playerRect, &enemyRect)) {
                        enemy.active = false;
                        player.health -= 25;
                        if (explosionSound) Mix_PlayChannel(-1, explosionSound, 0);
                        if (player.health <= 0 && player.lives > 0) {
                            player.lives--;
                            player.health = 100;
                        }
                        if (player.lives <= 0) {
                            gameOver = true;
                        }
                    }
                }

                // Collision with bullets
                for (auto& bullet : bullets) {
                    if (!bullet.active) continue;
                    SDL_Rect bulletRect = { static_cast<int>(bullet.x), static_cast<int>(bullet.y), BULLET_WIDTH, BULLET_HEIGHT };
                    SDL_Rect enemyRect = { static_cast<int>(enemy.x), static_cast<int>(enemy.y), ENEMY_WIDTH, ENEMY_HEIGHT };
                    if (SDL_HasIntersection(&bulletRect, &enemyRect)) {
                        bullet.active = false;
                        enemy.active = false;
                        if (explosionSound) Mix_PlayChannel(-1, explosionSound, 0);
                        score += 10;
                        if (player.level < 10 && score >= player.level * 100) player.level++;
                        if (score > player.hiScore) player.hiScore = score;
                        if (rand() % 100 < 20) {
                            PowerUpType type = static_cast<PowerUpType>(rand() % 6);
                            SDL_Texture* texture = nullptr;
                            switch (type) {
                                case SHIELD: texture = shieldTexture; break;
                                case HEALTH_INCREASE: texture = healthIncreaseTexture; break;
                                case FULL_HEALTH: texture = fullHealthTexture; break;
                                case ADDITIONAL_BULLETS: texture = additionalBulletsTexture; break;
                                case NUKE: texture = nukeTexture; break;
                                case POWERUP_BULLET_SPEED: texture = bulletSpeedTexture; break;
                            }
                            if (texture) {
                                PowerUp powerUp = { enemy.x, enemy.y, POWERUP_WIDTH, POWERUP_HEIGHT, texture, true, type, 0 };
                                powerUps.push_back(powerUp);
                            }
                        }
                    }
                }
            }

            // Update power-ups
            for (auto& powerUp : powerUps) {
                if (!powerUp.active) continue;
                powerUp.y += 225.0f * deltaTime;
                if (powerUp.y > SCREEN_HEIGHT) powerUp.active = false;

                SDL_Rect powerUpRect = { static_cast<int>(powerUp.x), static_cast<int>(powerUp.y), POWERUP_WIDTH, POWERUP_HEIGHT };
                SDL_Rect playerRect = { static_cast<int>(player.x), static_cast<int>(player.y), PLAYER_WIDTH, PLAYER_HEIGHT };
                if (SDL_HasIntersection(&powerUpRect, &playerRect)) {
                    powerUp.active = false;
                    powerUp.timer = SDL_GetTicks();
                    switch (powerUp.type) {
                        case SHIELD:
                            player.shieldActive = true;
                            player.shieldTimer = powerUp.timer;
                            break;
                        case HEALTH_INCREASE:
                            player.health = std::min(100, static_cast<int>(player.health * 1.25));
                            break;
                        case FULL_HEALTH:
                            player.health = 100;
                            break;
                        case ADDITIONAL_BULLETS:
                            player.extraBulletsActive = true;
                            player.extraBulletsTimer = powerUp.timer;
                            break;
                        case NUKE:
                            for (auto& enemy : enemies) {
                                if (enemy.active) {
                                    enemy.active = false;
                                    score += 10;
                                }
                            }
                            if (explosionSound) Mix_PlayChannel(-1, explosionSound, 0);
                            break;
                        case POWERUP_BULLET_SPEED:
                            player.bulletSpeedActive = true;
                            player.bulletSpeedTimer = powerUp.timer;
                            break;
                    }
                }
            }

            // Clean up inactive entities
            bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
                                         [](const Bullet& b) { return !b.active; }), bullets.end());
            enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
                                         [](const Enemy& e) { return !e.active; }), enemies.end());
            powerUps.erase(std::remove_if(powerUps.begin(), powerUps.end(),
                                          [](const PowerUp& p) { return !p.active; }), powerUps.end());

            // Scroll background
            bgY += 225.0f * deltaTime;
            if (bgY >= SCREEN_HEIGHT) bgY -= SCREEN_HEIGHT;
        }

        // Render
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_Rect bgSrc1 = { 0, static_cast<int>(bgY), SCREEN_WIDTH, SCREEN_HEIGHT - static_cast<int>(bgY) };
        SDL_Rect bgDst1 = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT - static_cast<int>(bgY) };
        SDL_Rect bgSrc2 = { 0, 0, SCREEN_WIDTH, static_cast<int>(bgY) };
        SDL_Rect bgDst2 = { 0, SCREEN_HEIGHT - static_cast<int>(bgY), SCREEN_WIDTH, static_cast<int>(bgY) };
        SDL_RenderCopy(renderer, bgTexture, &bgSrc1, &bgDst1);
        SDL_RenderCopy(renderer, bgTexture, &bgSrc2, &bgDst2);

        SDL_Rect playerDst = { static_cast<int>(player.x),
                               static_cast<int>(player.y),
                               PLAYER_WIDTH,
                               PLAYER_HEIGHT };
        SDL_RenderCopy(renderer, player.texture, nullptr, &playerDst);

        if (player.shieldActive) {
            SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);
            drawCircle(renderer, playerDst.x + playerDst.w / 2, playerDst.y + playerDst.h / 2,
                       static_cast<int>(PLAYER_WIDTH * 0.75));
        }

        for (const auto& bullet : bullets) {
            if (bullet.active) {
                SDL_Rect bulletDst = { static_cast<int>(bullet.x),
                                       static_cast<int>(bullet.y),
                                       BULLET_WIDTH,
                                       BULLET_HEIGHT };
                SDL_RenderCopy(renderer, bullet.texture, nullptr, &bulletDst);
            }
        }

        for (const auto& enemy : enemies) {
            if (enemy.active) {
                SDL_Rect enemyDst = { static_cast<int>(enemy.x),
                                      static_cast<int>(enemy.y),
                                      ENEMY_WIDTH,
                                      ENEMY_HEIGHT };
                SDL_RenderCopy(renderer, enemy.texture, nullptr, &enemyDst);
            }
        }

        for (const auto& powerUp : powerUps) {
            if (powerUp.active) {
                SDL_Rect powerUpDst = { static_cast<int>(powerUp.x),
                                        static_cast<int>(powerUp.y),
                                        POWERUP_WIDTH,
                                        POWERUP_HEIGHT };
                SDL_RenderCopy(renderer, powerUp.texture, nullptr, &powerUpDst);
            }
        }

        // HUD rendering
        SDL_Color white = {255, 255, 255, 255};
        auto renderText = [&](const std::string& text, int x, int y) {
            if (!font) return;
            SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), white);
            if (!surface) return;
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            if (!texture) {
                SDL_FreeSurface(surface);
                return;
            }
            SDL_Rect dst = { x, y, surface->w, surface->h };
            SDL_RenderCopy(renderer, texture, nullptr, &dst);
            SDL_FreeSurface(surface);
            SDL_DestroyTexture(texture);
        };

        renderText("Score: " + std::to_string(score), 10, 10);
        renderText("Lives: " + std::to_string(player.lives), 10, 40);
        renderText("Level: " + std::to_string(player.level), 10, 70);
        renderText("Hi-Score: " + std::to_string(player.hiScore), 10, 100);

        SDL_Rect healthBar = { 10, 130, static_cast<int>(450 * (player.health / 100.0f)), 45 };
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &healthBar);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &healthBar);

        if (player.shieldActive) {
            int timeLeft = 60 - ((currentTime - player.shieldTimer) / 1000);
            renderText("Shield: " + std::to_string(timeLeft), 10, 160);
        }
        if (player.extraBulletsActive) {
            int timeLeft = 60 - ((currentTime - player.extraBulletsTimer) / 1000);
            renderText("Extra Bullets: " + std::to_string(timeLeft), 10, 190);
        }
        if (player.bulletSpeedActive) {
            int timeLeft = 60 - ((currentTime - player.bulletSpeedTimer) / 1000);
            renderText("Bullet Speed: " + std::to_string(timeLeft), 10, 220);
        }

        // Game over screen
        if (gameOver) {
            renderText("Game Over! Final Score: " + std::to_string(score), SCREEN_WIDTH / 2 - 200, SCREEN_HEIGHT / 2 - 50);
            SDL_RenderPresent(renderer);
            SDL_Delay(2000); // Show for 2 seconds before quitting
            quit = true;
        } else {
            SDL_RenderPresent(renderer);
            // Precise frame rate control
            Uint32 frameTime = SDL_GetTicks() - currentTime;
            int targetFrameTime = 16; // ~60 FPS
            if (frameTime < targetFrameTime) {
                SDL_Delay(targetFrameTime - frameTime);
            }
        }
    }

    // Cleanup
    SDL_DestroyTexture(playerTexture);
    SDL_DestroyTexture(bulletTexture);
    for (int i = 0; i < EnemyType::COUNT; i++) SDL_DestroyTexture(enemyTextures[i]);
    SDL_DestroyTexture(shieldTexture);
    SDL_DestroyTexture(healthIncreaseTexture);
    SDL_DestroyTexture(fullHealthTexture);
    SDL_DestroyTexture(additionalBulletsTexture);
    SDL_DestroyTexture(nukeTexture);
    SDL_DestroyTexture(bulletSpeedTexture);
    SDL_DestroyTexture(bgTexture);
    Mix_FreeChunk(shootSound);
    Mix_FreeChunk(explosionSound);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_CloseAudio();
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();

    return 0;
}
