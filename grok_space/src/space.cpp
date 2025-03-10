#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>
#include <cmath>
#include <algorithm>

// Window and game constants
const int WINDOW_WIDTH = 1920;
const int WINDOW_HEIGHT = 1080;
const int PLAYER_SPEED = 12;
const int BULLET_SPEED = 15;
const int ENEMY_SPEED = 7;
const int ENEMY_SHOOT_INTERVAL = 2000; // ms
const int ENEMY_SPAWN_INTERVAL = 1000; // 1 second between spawn events
const int ENEMIES_PER_SPAWN = 3; // Spawn 3 enemies at once
const int POWERUP_DURATION = 30000; // 30 seconds for power-up effects

// Power-up types
enum PowerUpType {
    INVINCIBILITY,
    NUKE,
    DECOY,
    HEALTH_INCREASE,
    MORE_BULLETS,
    FASTER_BULLETS
};

// Forward declarations
class Enemy;
class PowerUp;
class Decoy;
SDL_Renderer* renderer; // Global for simplicity

// Player structure with added fields
struct Player {
    SDL_Rect rect = {WINDOW_WIDTH / 2 - 25, WINDOW_HEIGHT - 100, 64, 64};
    SDL_Texture* texture;
    int health = 100;
    int maxHealth = 100;
    bool invincible = false;
    Uint32 invincibilityEnd = 0;
    int bulletCount = 2;
    int baseBulletCount = 2; // Base value to revert after power-up
    Uint32 bulletCountEnd = 0;
    int bulletSpeed = BULLET_SPEED;
    int baseBulletSpeed = BULLET_SPEED; // Base value to revert after power-up
    Uint32 bulletSpeedEnd = 0;
    int lives = 3;
    std::vector<PowerUpType> activePowerUps;
};

// Bullet structure
struct Bullet {
    SDL_Rect rect;
    int speed;
    bool isPlayerBullet;
    SDL_Texture* texture;
    Bullet(int x, int y, bool playerBullet, int spd, SDL_Renderer* r)
        : rect({x, y, 10, 20}), speed(spd), isPlayerBullet(playerBullet) {
        texture = IMG_LoadTexture(r, playerBullet ? "bullet_player.png" : "bullet_enemy.png");
        if (!texture) {
            SDL_Log("Failed to load bullet texture (%s): %s", playerBullet ? "player" : "enemy", IMG_GetError());
        }
        SDL_Log("Bullet created at (%d, %d), size 10x20, player=%d", x, y, playerBullet);
    }
    ~Bullet() {
        if (texture) SDL_DestroyTexture(texture);
    }
    void update() {
        rect.y += speed;
    }
    void render(SDL_Renderer* r) {
        SDL_SetRenderDrawColor(r, isPlayerBullet ? 0 : 255, isPlayerBullet ? 255 : 0, 0, 255); // Green for player, red for enemy
        SDL_RenderFillRect(r, &rect);
        SDL_Log("Rendering bullet at (%d, %d), player=%d", rect.x, rect.y, isPlayerBullet);
    }
};

// Base Enemy class
class Enemy {
public:
    SDL_Rect rect;
    SDL_Texture* texture;
    int health = 5;
    Uint32 shootTimer;
    virtual ~Enemy() { if (texture) SDL_DestroyTexture(texture); }
    virtual void update() = 0;
    void tryShoot(std::vector<Bullet>& bullets, SDL_Renderer* r) {
        if (SDL_GetTicks() - shootTimer > ENEMY_SHOOT_INTERVAL) {
            shoot(bullets, r);
            shootTimer = SDL_GetTicks();
        }
    }
    virtual void shoot(std::vector<Bullet>& bullets, SDL_Renderer* r) {
        Bullet b(rect.x + rect.w / 2 - 5, rect.y + rect.h, false, 5, r);
        bullets.push_back(b);
        SDL_Log("Enemy shot bullet from (%d, %d)", rect.x + rect.w / 2 - 5, rect.y + rect.h);
    }
    void render(SDL_Renderer* r) {
        if (texture) SDL_RenderCopy(r, texture, NULL, &rect);
    }
};

// Straight Enemy
class StraightEnemy : public Enemy {
public:
    StraightEnemy(int x, int y, SDL_Renderer* r) {
        rect = {x, y, 64, 64};
        texture = IMG_LoadTexture(r, "enemy_straight.png");
        if (!texture) SDL_Log("Failed to load enemy_straight.png: %s", IMG_GetError());
        shootTimer = SDL_GetTicks();
    }
    void update() override {
        rect.y += ENEMY_SPEED;
    }
};

// Sine Wave Enemy
class SineEnemy : public Enemy {
private:
    float time = 0.0f;
    float amplitude = 50.0f;
    float frequency = 2.0f;
    int startX;
public:
    SineEnemy(int x, int y, SDL_Renderer* r) : startX(x) {
        rect = {x, y, 64, 64};
        texture = IMG_LoadTexture(r, "enemy_sine.png");
        if (!texture) SDL_Log("Failed to load enemy_sine.png: %s", IMG_GetError());
        shootTimer = SDL_GetTicks();
    }
    void update() override {
        time += 0.05f;
        rect.x = startX + static_cast<int>(sin(time * frequency) * amplitude);
        rect.y += ENEMY_SPEED;
    }
};

// Zigzag Enemy
class ZigzagEnemy : public Enemy {
private:
    int direction = 1;
public:
    ZigzagEnemy(int x, int y, SDL_Renderer* r) {
        rect = {x, y, 64, 64};
        texture = IMG_LoadTexture(r, "enemy_zigzag.png");
        if (!texture) SDL_Log("Failed to load enemy_zigzag.png: %s", IMG_GetError());
        shootTimer = SDL_GetTicks();
    }
    void update() override {
        rect.x += direction * 2;
        if (rect.x <= 0 || rect.x >= WINDOW_WIDTH - rect.w) direction *= -1;
        rect.y += ENEMY_SPEED;
    }
};

// PowerUp class
class PowerUp {
public:
    PowerUpType type;
    SDL_Rect rect;
    PowerUp(PowerUpType t, int x, int y) : type(t), rect({x, y, 64, 64}) {
        SDL_Log("PowerUp created at (%d, %d), type %d, size 64x64", x, y, t);
    }
    void update() {
        rect.y += 5; // Faster drop speed
    }
    void render(SDL_Renderer* r) const {
        SDL_SetRenderDrawColor(r, 255, 255, 0, 255); // Yellow rectangle
        SDL_RenderFillRect(r, &rect);
        SDL_Log("Rendering powerup at (%d, %d), type %d", rect.x, rect.y, type);
    }
};

// Decoy class
class Decoy {
public:
    SDL_Rect rect;
    SDL_Texture* texture;
    Decoy(int x, int y, SDL_Renderer* r) : rect({x, y, 64, 64}) {
        texture = IMG_LoadTexture(r, "decoy.png");
        if (!texture) SDL_Log("Failed to load decoy.png: %s", IMG_GetError());
    }
    ~Decoy() { if (texture) SDL_DestroyTexture(texture); }
    void update() {}
    void render(SDL_Renderer* r) {
        if (texture) SDL_RenderCopy(r, texture, NULL, &rect);
    }
};

// Render scrolling background
void renderBackground(SDL_Renderer* r, SDL_Texture* bgTexture, int& bgY) {
    bgY += 2;
    if (bgY >= WINDOW_HEIGHT) bgY = 0;
    SDL_Rect bgRect1 = {0, bgY, WINDOW_WIDTH, WINDOW_HEIGHT};
    SDL_Rect bgRect2 = {0, bgY - WINDOW_HEIGHT, WINDOW_WIDTH, WINDOW_HEIGHT};
    SDL_RenderCopy(r, bgTexture, NULL, &bgRect1);
    SDL_RenderCopy(r, bgTexture, NULL, &bgRect2);
}

// Apply power-up effects
void applyPowerUp(Player& player, PowerUpType type, std::vector<Enemy*>& enemies, Decoy*& decoy, Uint32& decoyEnd, int& score) {
    SDL_Log("Applying power-up: %d", type);
    Uint32 currentTime = SDL_GetTicks();
    switch (type) {
        case INVINCIBILITY:
            player.invincible = true;
            player.invincibilityEnd = currentTime + POWERUP_DURATION;
            player.activePowerUps.push_back(INVINCIBILITY);
            SDL_Log("Invincibility activated for 30s");
            break;
        case NUKE:
            for (auto e : enemies) delete e;
            enemies.clear();
            score += 1000;
            player.activePowerUps.push_back(NUKE); // Instant effect, no duration
            SDL_Log("Nuke activated, enemies cleared");
            break;
        case DECOY:
            if (!decoy) {
                decoy = new Decoy(player.rect.x, player.rect.y, renderer);
                decoyEnd = currentTime + POWERUP_DURATION;
                player.activePowerUps.push_back(DECOY);
                SDL_Log("Decoy deployed for 30s");
            }
            break;
        case HEALTH_INCREASE:
            player.health = std::min(player.maxHealth, player.health + 20);
            player.activePowerUps.push_back(HEALTH_INCREASE); // Instant effect, no duration
            SDL_Log("Health increased to %d", player.health);
            break;
        case MORE_BULLETS:
            player.bulletCount += 1;
            player.bulletCountEnd = currentTime + POWERUP_DURATION;
            player.activePowerUps.push_back(MORE_BULLETS);
            SDL_Log("Bullet count increased to %d for 30s", player.bulletCount);
            break;
        case FASTER_BULLETS:
            player.bulletSpeed += 5;
            player.bulletSpeedEnd = currentTime + POWERUP_DURATION;
            player.activePowerUps.push_back(FASTER_BULLETS);
            SDL_Log("Bullet speed increased to %d for 30s", player.bulletSpeed);
            break;
    }
}

// Spawn power-ups when enemies are destroyed
void spawnPowerUp(std::vector<PowerUp>& powerUps, int x, int y, SDL_Renderer* r) {
    if (std::rand() % 10 < 2) { // 20% chance
        PowerUpType type = static_cast<PowerUpType>(std::rand() % 6);
        powerUps.push_back(PowerUp(type, x, y));
    }
}

// Check collisions and clean up off-screen objects
void checkCollisions(Player& player, std::vector<Bullet>& bullets, std::vector<Enemy*>& enemies, std::vector<PowerUp>& powerUps, Decoy* decoy, Uint32& decoyEnd, int& score, bool& running) {
    for (auto bIt = bullets.begin(); bIt != bullets.end();) {
        if (bIt->isPlayerBullet) {
            bool hit = false;
            for (auto eIt = enemies.begin(); eIt != enemies.end();) {
                if (SDL_HasIntersection(&bIt->rect, &(*eIt)->rect)) {
                    (*eIt)->health -= 10;
                    if ((*eIt)->health <= 0) {
                        spawnPowerUp(powerUps, (*eIt)->rect.x, (*eIt)->rect.y, renderer);
                        delete *eIt;
                        eIt = enemies.erase(eIt);
                        score += 100;
                    } else {
                        ++eIt;
                    }
                    hit = true;
                    break;
                } else {
                    ++eIt;
                }
            }
            if (hit) {
                bIt = bullets.erase(bIt);
            } else {
                ++bIt;
            }
        } else {
            if (SDL_HasIntersection(&bIt->rect, &player.rect)) {
                if (!player.invincible) {
                    player.health -= 10;
                    SDL_Log("Player hit, health now %d", player.health);
                    if (player.health <= 0) {
                        player.lives--;
                        player.health = player.maxHealth;
                        SDL_Log("Player lost a life, lives remaining: %d", player.lives);
                        if (player.lives <= 0) running = false;
                    }
                }
                bIt = bullets.erase(bIt);
            } else {
                ++bIt;
            }
        }
    }

    for (auto pIt = powerUps.begin(); pIt != powerUps.end();) {
        if (SDL_HasIntersection(&pIt->rect, &player.rect)) {
            applyPowerUp(player, pIt->type, enemies, decoy, decoyEnd, score);
            pIt = powerUps.erase(pIt);
        } else {
            ++pIt;
        }
    }

    for (auto bIt = bullets.begin(); bIt != bullets.end();) {
        if (bIt->rect.y < -20 || bIt->rect.y > WINDOW_HEIGHT + 20) {
            bIt = bullets.erase(bIt);
        } else {
            ++bIt;
        }
    }

    for (auto pIt = powerUps.begin(); pIt != powerUps.end();) {
        if (pIt->rect.y > WINDOW_HEIGHT + 64) {
            pIt = powerUps.erase(pIt);
            SDL_Log("PowerUp removed (off-screen)");
        } else {
            ++pIt;
        }
    }

    for (auto eIt = enemies.begin(); eIt != enemies.end();) {
        if ((*eIt)->rect.y > WINDOW_HEIGHT + 64) {
            delete *eIt;
            eIt = enemies.erase(eIt);
            SDL_Log("Enemy removed (off-screen)");
        } else {
            ++eIt;
        }
    }
}

// Render HUD with all elements
void renderHUD(SDL_Renderer* r, TTF_Font* font, int score, int health, int lives, int level, const std::vector<PowerUpType>& activePowerUps, bool bossApproaching) {
    if (!font) return;
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color red = {255, 0, 0, 255};

    std::string scoreText = "Score: " + std::to_string(score);
    SDL_Surface* scoreSurface = TTF_RenderText_Solid(font, scoreText.c_str(), white);
    if (!scoreSurface) { SDL_Log("Failed to render score: %s", TTF_GetError()); return; }
    SDL_Texture* scoreTexture = SDL_CreateTextureFromSurface(r, scoreSurface);
    if (!scoreTexture) { SDL_Log("Failed to create score texture: %s", SDL_GetError()); SDL_FreeSurface(scoreSurface); return; }
    SDL_Rect scoreRect = {10, 10, scoreSurface->w, scoreSurface->h};
    SDL_RenderCopy(r, scoreTexture, NULL, &scoreRect);

    std::string healthText = "Health: " + std::to_string(health);
    SDL_Surface* healthSurface = TTF_RenderText_Solid(font, healthText.c_str(), white);
    if (!healthSurface) { SDL_Log("Failed to render health: %s", TTF_GetError()); SDL_FreeSurface(scoreSurface); SDL_DestroyTexture(scoreTexture); return; }
    SDL_Texture* healthTexture = SDL_CreateTextureFromSurface(r, healthSurface);
    if (!healthTexture) { SDL_Log("Failed to create health texture: %s", SDL_GetError()); SDL_FreeSurface(scoreSurface); SDL_DestroyTexture(scoreTexture); SDL_FreeSurface(healthSurface); return; }
    SDL_Rect healthRect = {10, 40, healthSurface->w, healthSurface->h};
    SDL_RenderCopy(r, healthTexture, NULL, &healthRect);

    std::string livesText = "Lives: " + std::to_string(lives);
    SDL_Surface* livesSurface = TTF_RenderText_Solid(font, livesText.c_str(), white);
    if (!livesSurface) { SDL_Log("Failed to render lives: %s", TTF_GetError()); SDL_FreeSurface(scoreSurface); SDL_DestroyTexture(scoreTexture); SDL_FreeSurface(healthSurface); SDL_DestroyTexture(healthTexture); return; }
    SDL_Texture* livesTexture = SDL_CreateTextureFromSurface(r, livesSurface);
    if (!livesTexture) { SDL_Log("Failed to create lives texture: %s", SDL_GetError()); SDL_FreeSurface(scoreSurface); SDL_DestroyTexture(scoreTexture); SDL_FreeSurface(healthSurface); SDL_DestroyTexture(healthTexture); SDL_FreeSurface(livesSurface); return; }
    SDL_Rect livesRect = {10, 70, livesSurface->w, livesSurface->h};
    SDL_RenderCopy(r, livesTexture, NULL, &livesRect);

    std::string levelText = "Level: " + std::to_string(level);
    SDL_Surface* levelSurface = TTF_RenderText_Solid(font, levelText.c_str(), white);
    if (!levelSurface) { SDL_Log("Failed to render level: %s", TTF_GetError()); SDL_FreeSurface(scoreSurface); SDL_DestroyTexture(scoreTexture); SDL_FreeSurface(healthSurface); SDL_DestroyTexture(healthTexture); SDL_FreeSurface(livesSurface); SDL_DestroyTexture(livesTexture); return; }
    SDL_Texture* levelTexture = SDL_CreateTextureFromSurface(r, levelSurface);
    if (!levelTexture) { SDL_Log("Failed to create level texture: %s", SDL_GetError()); SDL_FreeSurface(scoreSurface); SDL_DestroyTexture(scoreTexture); SDL_FreeSurface(healthSurface); SDL_DestroyTexture(healthTexture); SDL_FreeSurface(livesSurface); SDL_DestroyTexture(livesTexture); SDL_FreeSurface(levelSurface); return; }
    SDL_Rect levelRect = {10, 100, levelSurface->w, levelSurface->h};
    SDL_RenderCopy(r, levelTexture, NULL, &levelRect);

    std::string powerUpsText = "Power-Ups: ";
    for (const auto& p : activePowerUps) {
        switch (p) {
            case INVINCIBILITY: powerUpsText += "I "; break;
            case NUKE: powerUpsText += "N "; break;
            case DECOY: powerUpsText += "D "; break;
            case HEALTH_INCREASE: powerUpsText += "H "; break;
            case MORE_BULLETS: powerUpsText += "M "; break;
            case FASTER_BULLETS: powerUpsText += "F "; break;
        }
    }
    SDL_Surface* powerUpsSurface = TTF_RenderText_Solid(font, powerUpsText.c_str(), white);
    if (!powerUpsSurface) { SDL_Log("Failed to render power-ups: %s", TTF_GetError()); SDL_FreeSurface(scoreSurface); SDL_DestroyTexture(scoreTexture); SDL_FreeSurface(healthSurface); SDL_DestroyTexture(healthTexture); SDL_FreeSurface(livesSurface); SDL_DestroyTexture(livesTexture); SDL_FreeSurface(levelSurface); SDL_DestroyTexture(levelTexture); return; }
    SDL_Texture* powerUpsTexture = SDL_CreateTextureFromSurface(r, powerUpsSurface);
    if (!powerUpsTexture) { SDL_Log("Failed to create power-ups texture: %s", SDL_GetError()); SDL_FreeSurface(scoreSurface); SDL_DestroyTexture(scoreTexture); SDL_FreeSurface(healthSurface); SDL_DestroyTexture(healthTexture); SDL_FreeSurface(livesSurface); SDL_DestroyTexture(livesTexture); SDL_FreeSurface(levelSurface); SDL_DestroyTexture(levelTexture); SDL_FreeSurface(powerUpsSurface); return; }
    SDL_Rect powerUpsRect = {10, 130, powerUpsSurface->w, powerUpsSurface->h};
    SDL_RenderCopy(r, powerUpsTexture, NULL, &powerUpsRect);

    if (bossApproaching) {
        SDL_Surface* bossSurface = TTF_RenderText_Solid(font, "Boss Approaching!", red);
        if (!bossSurface) { SDL_Log("Failed to render boss warning: %s", TTF_GetError()); SDL_FreeSurface(scoreSurface); SDL_DestroyTexture(scoreTexture); SDL_FreeSurface(healthSurface); SDL_DestroyTexture(healthTexture); SDL_FreeSurface(livesSurface); SDL_DestroyTexture(livesTexture); SDL_FreeSurface(levelSurface); SDL_DestroyTexture(levelTexture); SDL_FreeSurface(powerUpsSurface); SDL_DestroyTexture(powerUpsTexture); return; }
        SDL_Texture* bossTexture = SDL_CreateTextureFromSurface(r, bossSurface);
        if (!bossTexture) { SDL_Log("Failed to create boss texture: %s", SDL_GetError()); SDL_FreeSurface(scoreSurface); SDL_DestroyTexture(scoreTexture); SDL_FreeSurface(healthSurface); SDL_DestroyTexture(healthTexture); SDL_FreeSurface(livesSurface); SDL_DestroyTexture(livesTexture); SDL_FreeSurface(levelSurface); SDL_DestroyTexture(levelTexture); SDL_FreeSurface(powerUpsSurface); SDL_DestroyTexture(powerUpsTexture); SDL_FreeSurface(bossSurface); return; }
        SDL_Rect bossRect = {WINDOW_WIDTH / 2 - bossSurface->w / 2, 10, bossSurface->w, bossSurface->h};
        SDL_RenderCopy(r, bossTexture, NULL, &bossRect);
        SDL_FreeSurface(bossSurface);
        SDL_DestroyTexture(bossTexture);
    }

    SDL_FreeSurface(scoreSurface);
    SDL_DestroyTexture(scoreTexture);
    SDL_FreeSurface(healthSurface);
    SDL_DestroyTexture(healthTexture);
    SDL_FreeSurface(livesSurface);
    SDL_DestroyTexture(livesTexture);
    SDL_FreeSurface(levelSurface);
    SDL_DestroyTexture(levelTexture);
    SDL_FreeSurface(powerUpsSurface);
    SDL_DestroyTexture(powerUpsTexture);
}

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }
    if (IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG) {
        SDL_Log("IMG_Init failed: %s", IMG_GetError());
        SDL_Quit();
        return 1;
    }
    if (TTF_Init() < 0) {
        SDL_Log("TTF_Init failed: %s", TTF_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Space Shooter", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if (!window) {
        SDL_Log("Window creation failed: %s", SDL_GetError());
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        SDL_Log("Renderer creation failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Texture* bgTexture = IMG_LoadTexture(renderer, "background.png");
    if (!bgTexture) {
        SDL_Log("Failed to load background.png: %s", IMG_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Texture* playerTexture = IMG_LoadTexture(renderer, "player.png");
    if (!playerTexture) {
        SDL_Log("Failed to load player.png: %s", IMG_GetError());
        SDL_DestroyTexture(bgTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    TTF_Font* font = TTF_OpenFont("arial.ttf", 24);
    if (!font) {
        SDL_Log("Failed to load arial.ttf: %s", TTF_GetError());
        SDL_DestroyTexture(playerTexture);
        SDL_DestroyTexture(bgTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    Player player = {{WINDOW_WIDTH / 2 - 25, WINDOW_HEIGHT - 100, 64, 64}, playerTexture};
    std::vector<Bullet> bullets;
    std::vector<Enemy*> enemies;
    std::vector<PowerUp> powerUps;
    Decoy* decoy = nullptr;
    Uint32 decoyEnd = 0;
    int bgY = 0;
    int score = 0;
    int level = 1;
    bool bossApproaching = false;
    bool running = true;
    Uint32 lastShot = 0;
    Uint32 lastEnemySpawn = 0;

    std::srand(std::time(0));

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
        }

        const Uint8* keystates = SDL_GetKeyboardState(NULL);
        if (keystates[SDL_SCANCODE_LEFT] && player.rect.x > 0) player.rect.x -= PLAYER_SPEED;
        if (keystates[SDL_SCANCODE_RIGHT] && player.rect.x < WINDOW_WIDTH - player.rect.w) player.rect.x += PLAYER_SPEED;
        if (keystates[SDL_SCANCODE_UP] && player.rect.y > 0) player.rect.y -= PLAYER_SPEED;
        if (keystates[SDL_SCANCODE_DOWN] && player.rect.y < WINDOW_HEIGHT - player.rect.h) player.rect.y += PLAYER_SPEED;

        // Player shooting
        if (keystates[SDL_SCANCODE_SPACE] && SDL_GetTicks() - lastShot > 300) {
            for (int i = 0; i < player.bulletCount; i++) {
                int offset = (i - (player.bulletCount - 1) / 2) * 20;
                Bullet b(player.rect.x + player.rect.w / 2 + offset - 5, player.rect.y - 20, true, -player.bulletSpeed, renderer);
                bullets.push_back(b);
            }
            lastShot = SDL_GetTicks();
            SDL_Log("Player fired %d bullets", player.bulletCount);
        }

        // Spawn enemies (multiple per interval)
        if (SDL_GetTicks() - lastEnemySpawn > ENEMY_SPAWN_INTERVAL) {
            for (int i = 0; i < ENEMIES_PER_SPAWN; i++) {
                int type = std::rand() % 3;
                int x = std::rand() % (WINDOW_WIDTH - 64);
                switch (type) {
                    case 0: enemies.push_back(new StraightEnemy(x, -64, renderer)); break;
                    case 1: enemies.push_back(new SineEnemy(x, -64, renderer)); break;
                    case 2: enemies.push_back(new ZigzagEnemy(x, -64, renderer)); break;
                }
            }
            lastEnemySpawn = SDL_GetTicks();
            SDL_Log("Spawned %d enemies", ENEMIES_PER_SPAWN);
        }

        // Update game objects
        Uint32 currentTime = SDL_GetTicks();
        for (auto& b : bullets) b.update();
        for (auto e : enemies) {
            e->update();
            e->tryShoot(bullets, renderer);
        }
        for (auto& p : powerUps) p.update();
        if (decoy) decoy->update();

        // Check power-up timers
        if (player.invincible && currentTime > player.invincibilityEnd) {
            player.invincible = false;
            auto it = std::find(player.activePowerUps.begin(), player.activePowerUps.end(), INVINCIBILITY);
            if (it != player.activePowerUps.end()) player.activePowerUps.erase(it);
            SDL_Log("Invincibility ended");
        }
        if (decoy && currentTime > decoyEnd) {
            delete decoy;
            decoy = nullptr;
            auto it = std::find(player.activePowerUps.begin(), player.activePowerUps.end(), DECOY);
            if (it != player.activePowerUps.end()) player.activePowerUps.erase(it);
            SDL_Log("Decoy expired");
        }
        if (player.bulletCount > player.baseBulletCount && currentTime > player.bulletCountEnd) {
            player.bulletCount = player.baseBulletCount;
            auto it = std::find(player.activePowerUps.begin(), player.activePowerUps.end(), MORE_BULLETS);
            if (it != player.activePowerUps.end()) player.activePowerUps.erase(it);
            SDL_Log("More Bullets expired, reverted to %d", player.bulletCount);
        }
        if (player.bulletSpeed > player.baseBulletSpeed && currentTime > player.bulletSpeedEnd) {
            player.bulletSpeed = player.baseBulletSpeed;
            auto it = std::find(player.activePowerUps.begin(), player.activePowerUps.end(), FASTER_BULLETS);
            if (it != player.activePowerUps.end()) player.activePowerUps.erase(it);
            SDL_Log("Faster Bullets expired, reverted to %d", player.bulletSpeed);
        }

        // Handle collisions and cleanup
        checkCollisions(player, bullets, enemies, powerUps, decoy, decoyEnd, score, running);

        // Render
        SDL_RenderClear(renderer);
        renderBackground(renderer, bgTexture, bgY);
        SDL_RenderCopy(renderer, player.texture, NULL, &player.rect);
        for (const auto& e : enemies) e->render(renderer);
        for (auto& b : bullets) b.render(renderer);
        for (const auto& p : powerUps) p.render(renderer);
        if (decoy) decoy->render(renderer);
        renderHUD(renderer, font, score, player.health, player.lives, level, player.activePowerUps, bossApproaching);
        SDL_RenderPresent(renderer);

        const char* error = SDL_GetError();
        if (*error) {
            SDL_Log("Render error: %s", error);
            SDL_ClearError();
        }

        SDL_Delay(16); // ~60 FPS
    }

    // Cleanup
    for (auto e : enemies) delete e;
    if (decoy) delete decoy;
    SDL_DestroyTexture(bgTexture);
    SDL_DestroyTexture(playerTexture);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();

    return 0;
}