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

// Screen dimensions
const int SCREEN_WIDTH = 1920;
const int SCREEN_HEIGHT = 1080;
const int VIRTUAL_WIDTH = 640;
const int VIRTUAL_HEIGHT = 480;
const float SCALE_FACTOR = 2.25 rollers = 2.25f;
const int OFFSET_X = 240;

// Player properties
const float PLAYER_SPEED = 300.0f;
const int PLAYER_WIDTH = 32;
const int PLAYER_HEIGHT = 32;

// Bullet properties
const float BULLET_SPEED = 500.0f;
const int BULLET_WIDTH = 8;
const int BULLET_HEIGHT = 16;

// Enemy properties
const int ENEMY_WIDTH = 32;
const int ENEMY_HEIGHT = 32;

// Power-up properties
const int POWERUP_WIDTH = 16;
const int POWERUP_HEIGHT = 16;

enum EnemyType {
    STRAIGHT,    // Straight down
    ZIGZAG,      // Zigzag pattern
    SINE,        // Sine wave pattern
    CIRCULAR,    // Circular pattern
    DIAGONAL,    // Diagonal toward center
    FAST,        // Fast straight down
    SPIRAL,      // Spiral pattern
    COUNT
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
};

struct Bullet : Entity {
    bool active;
};

struct Enemy : Entity {
    bool active;
    EnemyType type;
    float speed;
    float dx, dy;      // Movement direction
    float angle;       // For circular/spiral patterns
    float amplitude;   // For sine/zigzag patterns
};

struct PowerUp : Entity {
    bool active;
    bool isShield;
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

void drawCircle(SDL_Renderer* renderer, int centerX, int centerY, int radius) {
    for (int w = 0; w < radius * 2; w++) {
        for (int h = 0; h < radius * 2; h++) {
            int dx = radius - w;
            int dy = radius - h;
            if ((dx*dx + dy*dy) <= (radius * radius)) {
                SDL_RenderPoint(renderer, centerX + dx, centerY + dy);
            }
        }
    }
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
    SDL_Texture* enemyTextures[EnemyType::COUNT] = {
        loadTexture("enemy1.png", renderer),  // Straight
        loadTexture("enemy2.png", renderer),  // Zigzag
        loadTexture("enemy3.png", renderer),  // Sine
        loadTexture("enemy4.png", renderer),  // Circular
        loadTexture("enemy5.png", renderer),  // Diagonal
        loadTexture("enemy6.png", renderer),  // Fast
        loadTexture("enemy7.png", renderer)   // Spiral
    };
    SDL_Texture* powerUpTexture = loadTexture("powerup.png", renderer);
    SDL_Texture* shieldTexture = loadTexture("shield.png", renderer);
    SDL_Texture* bgTexture = loadTexture("background.png", renderer);
    Mix_Chunk* shootSound = Mix_LoadWAV("shoot.wav");
    Mix_Chunk* explosionSound = Mix_LoadWAV("explosion.wav");
    TTF_Font* font = TTF_OpenFont("arial.ttf", 24);

    Player player = { 
        VIRTUAL_WIDTH / 2.0f - PLAYER_WIDTH / 2.0f, 
        VIRTUAL_HEIGHT - PLAYER_HEIGHT - 20, 
        PLAYER_WIDTH, PLAYER_HEIGHT, 
        playerTexture, 
        10, 0, 3, 1, 100, 0, 
        false, 0 
    };

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
        if (keyboardState[SDL_SCANCODE_LEFT]) player.x -= PLAYER_SPEED * deltaTime;
        if (keyboardState[SDL_SCANCODE_RIGHT]) player.x += PLAYER_SPEED * deltaTime;
        if (keyboardState[SDL_SCANCODE_UP]) player.y -= PLAYER_SPEED * deltaTime;
        if (keyboardState[SDL_SCANCODE_DOWN]) player.y += PLAYER_SPEED * deltaTime;
        player.x = std::max(0.0f, std::min(player.x, VIRTUAL_WIDTH - PLAYER_WIDTH));
        player.y = std::max(0.0f, std::min(player.y, VIRTUAL_HEIGHT - PLAYER_HEIGHT));

        // Shooting
        if (keyboardState[SDL_SCANCODE_SPACE] && player.shootCooldown <= 0) {
            Bullet bullet = { player.x + PLAYER_WIDTH / 2 - BULLET_WIDTH / 2, player.y - BULLET_HEIGHT, BULLET_WIDTH, BULLET_HEIGHT, bulletTexture, true };
            bullets.push_back(bullet);
            if (player.powerLevel >= 1) {
                Bullet bullet2 = { player.x + PLAYER_WIDTH / 2 - BULLET_WIDTH / 2 - 20, player.y - BULLET_HEIGHT, BULLET_WIDTH, BULLET_HEIGHT, bulletTexture, true };
                bullets.push_back(bullet2);
            }
            Mix_PlayChannel(-1, shootSound, 0);
            player.shootCooldown = 10;
        }
        if (player.shootCooldown > 0) player.shootCooldown--;

        // Update shield
        if (player.shieldActive && currentTime - player.shieldTimer >= 60000) {
            player.shieldActive = false;
        }

        // Update bullets
        for (auto& bullet : bullets) {
            if (!bullet.active) continue;
            bullet.y -= BULLET_SPEED * deltaTime;
            if (bullet.y + BULLET_HEIGHT < 0) bullet.active = false;
        }

        // Spawn enemies
        enemySpawnTimer--;
        if (enemySpawnTimer <= 0) {
            EnemyType type = static_cast<EnemyType>(rand() % EnemyType::COUNT);
            float startX = (rand() % 2 == 0) ? -ENEMY_WIDTH : VIRTUAL_WIDTH;
            Enemy enemy = { startX, -ENEMY_HEIGHT, ENEMY_WIDTH, ENEMY_HEIGHT, enemyTextures[type], true, type };
            
            switch (type) {
                case STRAIGHT:
                    enemy.speed = 100.0f;
                    enemy.dy = enemy.speed;
                    break;
                case ZIGZAG:
                    enemy.speed = 150.0f;
                    enemy.dx = (startX < 0) ? 100.0f : -100.0f;
                    enemy.dy = enemy.speed;
                    enemy.amplitude = 50.0f;
                    break;
                case SINE:
                    enemy.speed = 120.0f;
                    enemy.dy = enemy.speed;
                    enemy.amplitude = 75.0f;
                    enemy.angle = 0.0f;
                    break;
                case CIRCULAR:
                    enemy.speed = 2.0f; // Angular speed in radians/sec
                    enemy.angle = 0.0f;
                    enemy.amplitude = 100.0f; // Radius
                    enemy.x = VIRTUAL_WIDTH / 2.0f;
                    enemy.y = VIRTUAL_HEIGHT / 2.0f;
                    break;
                case DIAGONAL:
                    enemy.speed = 130.0f;
                    enemy.dx = (startX < 0) ? enemy.speed * 0.5f : -enemy.speed * 0.5f;
                    enemy.dy = enemy.speed;
                    break;
                case FAST:
                    enemy.speed = 200.0f;
                    enemy.dy = enemy.speed;
                    break;
                case SPIRAL:
                    enemy.speed = 1.5f; // Angular speed
                    enemy.angle = 0.0f;
                    enemy.amplitude = 150.0f; // Starting radius
                    enemy.x = VIRTUAL_WIDTH / 2.0f;
                    enemy.y = VIRTUAL_HEIGHT / 2.0f;
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
                    if (enemy.x <= 0 || enemy.x + ENEMY_WIDTH >= VIRTUAL_WIDTH) enemy.dx = -enemy.dx;
                    break;
                case SINE:
                    enemy.angle += enemy.speed * deltaTime * 0.05f;
                    enemy.x = startX + enemy.amplitude * sin(enemy.angle);
                    enemy.y += enemy.dy * deltaTime;
                    break;
                case CIRCULAR:
                    enemy.angle += enemy.speed * deltaTime;
                    enemy.x = VIRTUAL_WIDTH / 2.0f + enemy.amplitude * cos(enemy.angle);
                    enemy.y = VIRTUAL_HEIGHT / 2.0f + enemy.amplitude * sin(enemy.angle);
                    break;
                case DIAGONAL:
                    enemy.x += enemy.dx * deltaTime;
                    enemy.y unreliable
                    enemy.y += enemy.dy * deltaTime;
                    break;
                case SPIRAL:
                    enemy.angle += enemy.speed * deltaTime;
                    enemy.amplitude -= enemy.speed * deltaTime * 10; // Shrink radius over time
                    enemy.x = VIRTUAL_WIDTH / 2.0f + enemy.amplitude * cos(enemy.angle);
                    enemy.y = VIRTUAL_HEIGHT / 2.0f + enemy.amplitude * sin(enemy.angle);
                    break;
            }
            
            if (enemy.y > VIRTUAL_HEIGHT || enemy.x < -ENEMY_WIDTH || enemy.x > VIRTUAL_WIDTH || 
                (enemy.type == SPIRAL && enemy.amplitude <= 10)) {
                enemy.active = false;
            }

            if (!player.shieldActive) {
                SDL_Rect playerRect = { static_cast<int>(player.x), static_cast<int>(player.y), PLAYER_WIDTH, PLAYER_HEIGHT };
                SDL_Rect enemyRect = { static_cast<int>(enemy.x), static_cast<int>(enemy.y), ENEMY_WIDTH, ENEMY_HEIGHT };
                if (SDL_HasIntersection(&playerRect, &enemyRect)) {
                    enemy.active = false;
                    player.health -= 25;
                    Mix_PlayChannel(-1, explosionSound, 0);
                    if (player.health <= 0 && player.lives > 0) {
                        player.lives--;
                        player.health = 100;
                    }
                    if (player.lives <= 0) {
                        std::cout << "Game Over! Final Score: " << score << std::endl;
                        quit = true;
                    }
                }
            }

            for (auto& bullet : bullets) {
                if (!bullet.active) continue;
                SDL_Rect bulletRect = { static_cast<int>(bullet.x), static_cast<int>(bullet.y), BULLET_WIDTH, BULLET_HEIGHT };
                SDL_Rect enemyRect = { static_cast<int>(enemy.x), static_cast<int>(enemy.y), ENEMY_WIDTH, ENEMY_HEIGHT };
                if (SDL_HasIntersection(&bulletRect, &enemyRect)) {
                    bullet.active = false;
                    enemy.active = false;
                    Mix_PlayChannel(-1, explosionSound, 0);
                    score += 10;
                    if (player.level < 10 && score >= player.level * 100) player.level++;
                    if (score > player.hiScore) player.hiScore = score;
                    if (rand() % 100 < 20) {
                        PowerUp powerUp = { enemy.x, enemy.y, POWERUP_WIDTH, POWERUP_HEIGHT, 
                                          (rand() % 2 == 0) ? powerUpTexture : shieldTexture, 
                                          true, (rand() % 2 == 0) ? false : true };
                        powerUps.push_back(powerUp);
                    }
                }
            }
        }

        // Update power-ups
        for (auto& powerUp : powerUps) {
            if (!powerUp.active) continue;
            powerUp.y += 100.0f * deltaTime;
            if (powerUp.y > VIRTUAL_HEIGHT) powerUp.active = false;

            SDL_Rect powerUpRect = { static_cast<int>(powerUp.x), static_cast<int>(powerUp.y), POWERUP_WIDTH, POWERUP_HEIGHT };
            SDL_Rect playerRect = { static_cast<int>(player.x), static_cast<int>(player.y), PLAYER_WIDTH, PLAYER_HEIGHT };
            if (SDL_HasIntersection(&powerUpRect, &playerRect)) {
                powerUp.active = false;
                if (powerUp.isShield) {
                    player.shieldActive = true;
                    player.shieldTimer = SDL_GetTicks();
                } else if (player.powerLevel < 1) {
                    player.powerLevel++;
                }
            }
        }

        // Scroll background
        bgY += 100.0f * deltaTime;
        if (bgY >= VIRTUAL_HEIGHT) bgY -= VIRTUAL_HEIGHT;

        // Render
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Render background
        SDL_Rect bgSrc1 = { 0, static_cast<int>(bgY), VIRTUAL_WIDTH, VIRTUAL_HEIGHT - static_cast<int>(bgY) };
        SDL_Rect bgDst1 = { OFFSET_X, 0, static_cast<int>(VIRTUAL_WIDTH * SCALE_FACTOR), static_cast<int>((VIRTUAL_HEIGHT - bgY) * SCALE_FACTOR) };
        SDL_Rect bgSrc2 = { 0, 0, VIRTUAL_WIDTH, static_cast<int>(bgY) };
        SDL_Rect bgDst2 = { OFFSET_X, static_cast<int>((VIRTUAL_HEIGHT - bgY) * SCALE_FACTOR), static_cast<int>(VIRTUAL_WIDTH * SCALE_FACTOR), static_cast<int>(bgY * SCALE_FACTOR) };
        SDL_RenderCopy(renderer, bgTexture, &bgSrc1, &bgDst1);
        SDL_RenderCopy(renderer, bgTexture, &bgSrc2, &bgDst2);

        // Render player
        SDL_Rect playerDst = { static_cast<int>(player.x * SCALE_FACTOR) + OFFSET_X, 
                             static_cast<int>(player.y * SCALE_FACTOR), 
                             static_cast<int>(PLAYER_WIDTH * SCALE_FACTOR), 
                             static_cast<int>(PLAYER_HEIGHT * SCALE_FACTOR) };
        SDL_RenderCopy(renderer, player.texture, NULL, &playerDst);

        // Render shield
        if (player.shieldActive) {
            SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);
            drawCircle(renderer, playerDst.x + playerDst.w/2, playerDst.y + playerDst.h/2, 
                      static_cast<int>(PLAYER_WIDTH * SCALE_FACTOR * 0.75));
        }

        // Render bullets
        for (const auto& bullet : bullets) {
            if (bullet.active) {
                SDL_Rect bulletDst = { static_cast<int>(bullet.x * SCALE_FACTOR) + OFFSET_X, 
                                    static_cast<int>(bullet.y * SCALE_FACTOR), 
                                    static_cast<int>(BULLET_WIDTH * SCALE_FACTOR), 
                                    static_cast<int>(BULLET_HEIGHT * SCALE_FACTOR) };
                SDL_RenderCopy(renderer, bullet.texture, NULL, &bulletDst);
            }
        }

        // Render enemies
        for (const auto& enemy : enemies) {
            if (enemy.active) {
                SDL_Rect enemyDst = { static_cast<int>(enemy.x * SCALE_FACTOR) + OFFSET_X, 
                                    static_cast<int>(enemy.y * SCALE_FACTOR), 
                                    static_cast<int>(ENEMY_WIDTH * SCALE_FACTOR), 
                                    static_cast<int>(ENEMY_HEIGHT * SCALE_FACTOR) };
                SDL_RenderCopy(renderer, enemy.texture, NULL, &enemyDst);
            }
        }

        // Render power-ups
        for (const auto& powerUp : powerUps) {
            if (powerUp.active) {
                SDL_Rect powerUpDst = { static_cast<int>(powerUp.x * SCALE_FACTOR) + OFFSET_X, 
                                      static_cast<int>(powerUp.y * SCALE_FACTOR), 
                                      static_cast<int>(POWERUP_WIDTH * SCALE_FACTOR), 
                                      static_cast<int>(POWERUP_HEIGHT * SCALE_FACTOR) };
                SDL_RenderCopy(renderer, powerUp.texture, NULL, &powerUpDst);
            }
        }

        // Render HUD
        SDL_Color white = {255, 255, 255};
        
        std::string scoreText = "Score: " + std::to_string(score);
        SDL_Surface* scoreSurface = TTF_RenderText_Solid(font, scoreText.c_str(), white);
        SDL_Texture* scoreTexture = SDL_CreateTextureFromSurface(renderer, scoreSurface);
        SDL screenplay = { OFFSET_X + 10, 10, scoreSurface->w, scoreSurface->h };
        SDL_RenderCopy(renderer, scoreTexture, NULL, &scoreDst);

        std::string livesText = "Lives: " + std::to_string(player.lives);
        SDL_Surface* livesSurface = TTF_RenderText_Solid(font, livesText.c_str(), white);
        SDL_Texture* livesTexture = SDL_CreateTextureFromSurface(renderer, livesSurface);
        SDL_Rect livesDst = { OFFSET_X + 10, 40, livesSurface->w, livesSurface->h };
        SDL_RenderCopy(renderer, livesTexture, NULL, &livesDst);

        std::string levelText = "Level: " + std::to_string(player.level);
        SDL_Surface* levelSurface = TTF_RenderText_Solid(font, levelText.c_str(), white);
        SDL_Texture* levelTexture = SDL_CreateTextureFromSurface(renderer, levelSurface);
        SDL_Rect levelDst = { OFFSET_X + 10, 70, levelSurface->w, levelSurface->h };
        SDL_RenderCopy(renderer, levelTexture, NULL, &levelDst);

        std::string hiScoreText = "Hi-Score: " + std::to_string(player.hiScore);
        SDL_Surface* hiScoreSurface = TTF_RenderText_Solid(font, hiScoreText.c_str(), white);
        SDL_Texture* hiScoreTexture = SDL_CreateTextureFromSurface(renderer, hiScoreSurface);
        SDL_Rect hiScoreDst = { OFFSET_X + 10, 100, hiScoreSurface->w, hiScoreSurface->h };
        SDL_RenderCopy(renderer, hiScoreTexture, NULL, &hiScoreDst);

        SDL_Rect healthBar = { OFFSET_X + 10, 130, static_cast<int>(200 * SCALE_FACTOR * (player.health / 100.0f)), 20 };
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &healthBar);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderRect(renderer, &healthBar);

        if (player.shieldActive) {
            int shieldTimeLeft = 60 - ((currentTime - player.shieldTimer) / 1000);
            std::string shieldText = "Shield: " + std::to_string(shieldTimeLeft);
            SDL_Surface* shieldSurface = TTF_RenderText_Solid(font, shieldText.c_str(), white);
            SDL_Texture* shieldTexture = SDL_CreateTextureFromSurface(renderer, shieldSurface);
            SDL_Rect shieldDst = { OFFSET_X + 10, 160, shieldSurface->w, shieldSurface->h };
            SDL_RenderCopy(renderer, shieldTexture, NULL, &shieldDst);
            SDL_FreeSurface(shieldSurface);
            SDL_DestroyTexture(shieldTexture);
        }

        SDL_FreeSurface(scoreSurface);
        SDL_DestroyTexture(scoreTexture);
        SDL_FreeSurface(livesSurface);
        SDL_DestroyTexture(livesTexture);
        SDL_FreeSurface(levelSurface);
        SDL_DestroyTexture(levelTexture);
        SDL_FreeSurface(hiScoreSurface);
        SDL_DestroyTexture(hiScoreTexture);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    // Cleanup
    SDL_DestroyTexture(playerTexture);
    SDL_DestroyTexture(bulletTexture);
    for (int i = 0; i < EnemyType::COUNT; i++) SDL_DestroyTexture(enemyTextures[i]);
    SDL_DestroyTexture(powerUpTexture);
    SDL_DestroyTexture(shieldTexture);
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
