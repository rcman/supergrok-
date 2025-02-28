#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include <string>
#include <iostream>
#include <cmath>

// Constants
const int SCREEN_WIDTH = 448;
const int SCREEN_HEIGHT = 496;
const int PLAYER_SPEED = 3;
const float BULLET_SPEED = 5.0f;
const int GEM_VALUE = 1;
const int GOLD_VALUE = 10;
const int ENERGY_VALUE = 50;

// Entity Types
enum EntityType {
    PLAYER, ENEMY, BULLET_PLAYER, BULLET_ENEMY, GEM, GOLD, ENERGY, BOSS
};

// Game States
enum GameState {
    NORMAL, KAKUSEI
};

// Entity Structure
struct Entity {
    float x, y;
    float vx, vy;
    EntityType type;
    SDL_Texture* texture;
    int health = 1;
    bool active = true;
};

// Game Class
class Game {
public:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    TTF_Font* font = nullptr;
    SDL_Texture* playerAgehaTexture = nullptr;
    SDL_Texture* playerTatehaTexture = nullptr;
    SDL_Texture* playerAsagiTexture = nullptr;
    SDL_Texture* bulletPlayerTexture = nullptr;
    SDL_Texture* bulletEnemyTexture = nullptr;
    SDL_Texture* enemyTexture = nullptr;
    SDL_Texture* bossTexture = nullptr;
    SDL_Texture* gemTexture = nullptr;
    SDL_Texture* goldTexture = nullptr;
    SDL_Texture* energyTexture = nullptr;
    std::vector<Entity> entities;
    long long score = 0; // Changed to long long
    int lives = 3;
    int gems = 100;
    int energy = 100;
    GameState state = NORMAL;
    int multiplier = 1;
    bool running = true;
    int enemySpawnTimer = 0;
    bool bossSpawned = false;
    int invincibilityTimer = 0;
    int extraLifeThresholds[2] = {15000000, 35000000};
    bool extraLivesGranted[2] = {false, false};

    Game() {
        entities.push_back({SCREEN_WIDTH / 2 - 8, SCREEN_HEIGHT - 32, 0, 0, PLAYER, nullptr});
    }

    bool init() {
        auto cleanupOnFailure = [this]() {
            if (playerAgehaTexture) SDL_DestroyTexture(playerAgehaTexture);
            if (playerTatehaTexture) SDL_DestroyTexture(playerTatehaTexture);
            if (playerAsagiTexture) SDL_DestroyTexture(playerAsagiTexture);
            if (bulletPlayerTexture) SDL_DestroyTexture(bulletPlayerTexture);
            if (bulletEnemyTexture) SDL_DestroyTexture(bulletEnemyTexture);
            if (enemyTexture) SDL_DestroyTexture(enemyTexture);
            if (bossTexture) SDL_DestroyTexture(bossTexture);
            if (gemTexture) SDL_DestroyTexture(gemTexture);
            if (goldTexture) SDL_DestroyTexture(goldTexture);
            if (energyTexture) SDL_DestroyTexture(energyTexture);
            if (font) TTF_CloseFont(font);
            if (renderer) SDL_DestroyRenderer(renderer);
            if (window) SDL_DestroyWindow(window);
            entities.clear();
            TTF_Quit();
            IMG_Quit();
            SDL_Quit();
        };

        if (SDL_Init(SDL_INIT_VIDEO) < 0 || IMG_Init(IMG_INIT_PNG) == 0 || TTF_Init() < 0) {
            std::cerr << "SDL Init failed: " << SDL_GetError() << std::endl;
            cleanupOnFailure();
            return false;
        }
        window = SDL_CreateWindow("Espgaluda II Clone", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
        if (!window) {
            std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
            cleanupOnFailure();
            return false;
        }
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer) {
            std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
            cleanupOnFailure();
            return false;
        }

        font = TTF_OpenFont("font.ttf", 24);
        if (!font) {
            std::cerr << "Font load failed: " << TTF_GetError() << std::endl;
            cleanupOnFailure();
            return false;
        }
        playerAgehaTexture = IMG_LoadTexture(renderer, "player_ageha.png");
        playerTatehaTexture = IMG_LoadTexture(renderer, "player_tateha.png");
        playerAsagiTexture = IMG_LoadTexture(renderer, "player_asagi.png");
        bulletPlayerTexture = IMG_LoadTexture(renderer, "bullet_player.png");
        bulletEnemyTexture = IMG_LoadTexture(renderer, "bullet_enemy.png");
        enemyTexture = IMG_LoadTexture(renderer, "enemy.png");
        bossTexture = IMG_LoadTexture(renderer, "boss.png");
        gemTexture = IMG_LoadTexture(renderer, "gem.png");
        goldTexture = IMG_LoadTexture(renderer, "gold.png");
        energyTexture = IMG_LoadTexture(renderer, "energy.png");

        if (!playerAgehaTexture || !playerTatehaTexture || !playerAsagiTexture || 
            !bulletPlayerTexture || !bulletEnemyTexture || !enemyTexture || !bossTexture || 
            !gemTexture || !goldTexture || !energyTexture) {
            std::cerr << "Asset load failed: " << IMG_GetError() << std::endl;
            cleanupOnFailure();
            return false;
        }

        entities[0].texture = playerAgehaTexture; // Default to Ageha
        return true;
    }

    void handleInput() {
        if (entities.empty()) return;
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_1: entities[0].texture = playerAgehaTexture; break;
                    case SDLK_2: entities[0].texture = playerTatehaTexture; break;
                    case SDLK_3: entities[0].texture = playerAsagiTexture; break;
                    case SDLK_b: if (energy >= 25) useGuardBarrier(); break;
                    case SDLK_k: toggleKakusei(); break;
                }
            }
        }
        const Uint8* keys = SDL_GetKeyboardState(NULL);
        Entity& player = entities[0];
        player.vx = player.vy = 0;
        if (keys[SDL_SCANCODE_LEFT]) player.vx = -PLAYER_SPEED;
        if (keys[SDL_SCANCODE_RIGHT]) player.vx = PLAYER_SPEED;
        if (keys[SDL_SCANCODE_UP]) player.vy = -PLAYER_SPEED;
        if (keys[SDL_SCANCODE_DOWN]) player.vy = PLAYER_SPEED;

        // Auto-fire
        static int fireTimer = 0;
        if (fireTimer <= 0) {
            firePlayerBullet(keys[SDL_SCANCODE_LSHIFT]);
            fireTimer = 5;
        }
        fireTimer--;
    }

    void firePlayerBullet(bool focused) {
        Entity& player = entities[0];
        if (player.texture == playerAgehaTexture) {
            entities.push_back({player.x, player.y - 8, -BULLET_SPEED, 0, BULLET_PLAYER, bulletPlayerTexture});
            if (!focused) {
                entities.push_back({player.x - 8, player.y - 8, -BULLET_SPEED * 0.8f, -BULLET_SPEED * 0.6f, BULLET_PLAYER, bulletPlayerTexture});
                entities.push_back({player.x + 8, player.y - 8, -BULLET_SPEED * 0.8f, BULLET_SPEED * 0.6f, BULLET_PLAYER, bulletPlayerTexture});
            }
        } else if (player.texture == playerTatehaTexture) {
            entities.push_back({player.x, player.y - 8, -BULLET_SPEED * (focused ? 1.2f : 1.0f), 0, BULLET_PLAYER, bulletPlayerTexture});
        } else { // Asagi
            entities.push_back({player.x, player.y - 8, -BULLET_SPEED * 0.8f, 0, BULLET_PLAYER, bulletPlayerTexture});
        }
    }

    void toggleKakusei() {
        if (state == NORMAL && gems > 0) {
            state = KAKUSEI;
            multiplier = 1;
        } else if (state == KAKUSEI) {
            state = NORMAL;
        }
    }

    void useGuardBarrier() {
        energy -= 25;
        invincibilityTimer = 2000; // 2 seconds
        for (auto& e : entities) {
            if (e.type == BULLET_ENEMY) e.active = false;
        }
    }

    void update() {
        if (entities.empty()) return;

        // Player Movement
        Entity& player = entities[0];
        float newX = player.x + player.vx;
        float newY = player.y + player.vy;
        if (newX >= 0 && newX < SCREEN_WIDTH - 16) player.x = newX;
        if (newY >= 0 && newY < SCREEN_HEIGHT - 16) player.y = newY;

        // Invincibility
        if (invincibilityTimer > 0) invincibilityTimer -= 16;

        // Spawn Enemies
        enemySpawnTimer--;
        if (enemySpawnTimer <= 0 && !bossSpawned) {
            if (score < 5000) {
                entities.push_back({float(rand() % (SCREEN_WIDTH - 16)), 0, 0, 2, ENEMY, enemyTexture, 5});
                enemySpawnTimer = 60;
            } else if (!bossSpawned) {
                entities.push_back({SCREEN_WIDTH / 2 - 16, 0, 0, 1, BOSS, bossTexture, 50});
                bossSpawned = true;
            }
        }

        // Update Entities
        for (auto it = entities.begin(); it != entities.end();) {
            if (!it->active) {
                it = entities.erase(it);
                continue;
            }
            float speedMod = (state == KAKUSEI && it->type == BULLET_ENEMY) ? 0.5f : 1.0f;
            it->x += it->vx * speedMod;
            it->y += it->vy * speedMod;

            if (it->type == BULLET_PLAYER) {
                if (it->y < 0) it->active = false;
            } else if (it->type == BULLET_ENEMY) {
                if (it->y >= SCREEN_HEIGHT) it->active = false;
                if (invincibilityTimer <= 0 && abs(it->x - player.x) < 16 && abs(it->y - player.y) < 16) {
                    if (energy >= 25) {
                        useGuardBarrier();
                    } else {
                        lives--;
                        player.x = SCREEN_WIDTH / 2 - 8;
                        player.y = SCREEN_HEIGHT - 32;
                        if (lives <= 0) running = false;
                    }
                    it->active = false;
                }
            } else if (it->type == ENEMY || it->type == BOSS) {
                if (it->y >= SCREEN_HEIGHT) it->active = false;
                if (rand() % 30 == 0) {
                    entities.push_back({it->x, it->y + 8, 0, BULLET_SPEED, BULLET_ENEMY, bulletEnemyTexture});
                    if (it->type == BOSS && rand() % 2 == 0) {
                        entities.push_back({it->x - 8, it->y + 8, -BULLET_SPEED * 0.5f, BULLET_SPEED, BULLET_ENEMY, bulletEnemyTexture});
                        entities.push_back({it->x + 8, it->y + 8, BULLET_SPEED * 0.5f, BULLET_SPEED, BULLET_ENEMY, bulletEnemyTexture});
                    }
                }
            } else if (it->type == GEM || it->type == GOLD || it->type == ENERGY) {
                if (it->y >= SCREEN_HEIGHT) it->active = false;
                if (abs(it->x - player.x) < 16 && abs(it->y - player.y) < 16) {
                    if (it->type == GEM) gems += GEM_VALUE;
                    else if (it->type == GOLD) score += GOLD_VALUE * multiplier;
                    else energy = std::min(100, energy + ENERGY_VALUE);
                    it->active = false;
                }
            }
            ++it;
        }

        // Collision Detection
        for (auto& bullet : entities) {
            if (bullet.type != BULLET_PLAYER) continue;
            for (auto& target : entities) {
                if (target.type != ENEMY && target.type != BOSS) continue;
                if (abs(bullet.x - target.x) < 16 && abs(bullet.y - target.y) < 16 && target.active) {
                    target.health--;
                    bullet.active = false;
                    if (target.health <= 0) {
                        target.active = false;
                        score += (target.type == ENEMY ? 100 : 1000);
                        entities.push_back({target.x, target.y, 0, 1, GEM, gemTexture});
                        if (state == KAKUSEI) {
                            for (auto& e : entities) {
                                if (e.type == BULLET_ENEMY && abs(e.x - target.x) < 50 && abs(e.y - target.y) < 50) {
                                    e.type = GOLD;
                                    e.texture = goldTexture;
                                    e.vx = 0;
                                    e.vy = 1;
                                    multiplier = std::min(500, multiplier + 1);
                                }
                            }
                            entities.push_back({target.x, target.y, BULLET_SPEED, BULLET_SPEED, BULLET_ENEMY, bulletEnemyTexture});
                        }
                    }
                    break;
                }
            }
        }

        // Kakusei Mode
        if (state == KAKUSEI) {
            gems--;
            if (gems <= 0) state = NORMAL;
        }

        // Extra Lives
        if (lives < 5) {
            if (score >= extraLifeThresholds[0] && !extraLivesGranted[0]) {
                lives++;
                extraLivesGranted[0] = true;
            }
            if (score >= extraLifeThresholds[1] && !extraLivesGranted[1]) {
                lives++;
                extraLivesGranted[1] = true;
            }
        }
    }

    void render() {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        for (const auto& e : entities) {
            if (!e.active) continue;
            SDL_Rect rect;
            rect.x = int(std::round(e.x));
            rect.y = int(std::round(e.y));
            if (e.type == BULLET_PLAYER || e.type == BULLET_ENEMY) {
                rect.w = rect.h = 4;
            } else if (e.type == GEM || e.type == GOLD || e.type == ENERGY) {
                rect.w = rect.h = 8;
            } else if (e.type == BOSS) {
                rect.w = rect.h = 32;
            } else {
                rect.w = rect.h = 16;
            }
            if (e.type == PLAYER && invincibilityTimer > 0 && (SDL_GetTicks() / 100) % 2 == 0) continue; // Blink during invincibility
            SDL_RenderCopy(renderer, e.texture, NULL, &rect);
        }

        SDL_Color color = {255, 255, 255, 255};
        std::string text = "Score: " + std::to_string(score) + " Lives: " + std::to_string(lives) + 
                           " Gems: " + std::to_string(gems) + " Energy: " + std::to_string(energy);
        if (!running) text += lives > 0 ? " - Level Complete!" : " - Game Over!";
        SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), color);
        if (surface) {
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            if (texture) {
                SDL_Rect dest = {10, 10, surface->w, surface->h};
                SDL_RenderCopy(renderer, texture, NULL, &dest);
                SDL_DestroyTexture(texture);
            } else {
                std::cerr << "Texture creation failed: " << SDL_GetError() << std::endl;
            }
            SDL_FreeSurface(surface);
        } else {
            std::cerr << "Text rendering failed: " << TTF_GetError() << std::endl;
        }

        SDL_RenderPresent(renderer);
    }

    void clean() {
        if (playerAgehaTexture) SDL_DestroyTexture(playerAgehaTexture);
        if (playerTatehaTexture) SDL_DestroyTexture(playerTatehaTexture);
        if (playerAsagiTexture) SDL_DestroyTexture(playerAsagiTexture);
        if (bulletPlayerTexture) SDL_DestroyTexture(bulletPlayerTexture);
        if (bulletEnemyTexture) SDL_DestroyTexture(bulletEnemyTexture);
        if (enemyTexture) SDL_DestroyTexture(enemyTexture);
        if (bossTexture) SDL_DestroyTexture(bossTexture);
        if (gemTexture) SDL_DestroyTexture(gemTexture);
        if (goldTexture) SDL_DestroyTexture(goldTexture);
        if (energyTexture) SDL_DestroyTexture(energyTexture);
        if (font) TTF_CloseFont(font);
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
        entities.clear();
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
    }
};

int main() {
    Game game;
    if (!game.init()) {
        std::cerr << "Initialization failed: " << SDL_GetError() << std::endl;
        game.clean();
        return 1;
    }

    while (game.running) {
        game.handleInput();
        game.update();
        game.render();
        SDL_Delay(16); // ~60 FPS
    }

    game.clean();
    return 0;
}
