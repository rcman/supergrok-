#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <vector>
#include <iostream>
#include <cmath>

const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;
const int ROAD_WIDTH = 400;
const int PLAYER_SPEED = 5;
const int BULLET_SPEED = 10;
const int MISSILE_SPEED = 15;
const int ROAD_SPEED = 4;
const int MAX_WEAPONS = 4;

// Structs for game objects
struct Vector2 {
    int x, y;
};

struct Player {
    Vector2 pos;
    SDL_Texture* carTexture;
    SDL_Texture* boatTexture;
    int width = 48, height = 64;
    bool isBoat = false;
    int speed = PLAYER_SPEED;
    int weapons = 0; // Bitmask: 1 = guns, 2 = oil, 4 = smoke, 8 = missiles
    int gear = 0; // 0 = low, 1 = high
};

struct Bullet {
    Vector2 pos;
    Vector2 velocity;
    SDL_Texture* texture;
    bool active = false;
    bool isMissile = false;
};

struct Effect {
    Vector2 pos;
    SDL_Texture* texture;
    bool active = false;
    int lifetime = 60; // Frames
};

struct Enemy {
    Vector2 pos;
    SDL_Texture* texture;
    int width = 48, height = 64;
    int type = 0; // 0 = Switchblade, 1 = Bully, 2 = Enforcer, 3 = Civilian
    bool active = true;
};

struct WeaponsVan {
    Vector2 pos;
    SDL_Texture* texture;
    int width = 64, height = 96;
    bool active = false;
};

struct RoadSegment {
    Vector2 pos;
    SDL_Texture* texture;
    int width = ROAD_WIDTH, height = SCREEN_HEIGHT;
    bool isSnow = false;
    bool isWater = false;
    bool hasBoathouse = false;
};

// Global variables
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
Mix_Chunk* shootSound = nullptr;
Mix_Chunk* oilSound = nullptr;
Mix_Chunk* smokeSound = nullptr;
Mix_Chunk* missileSound = nullptr;
Mix_Music* engineMusic = nullptr;
Mix_Music* peterGunnMusic = nullptr;
Player player;
std::vector<Bullet> bullets;
std::vector<Effect> effects;
std::vector<Enemy> enemies;
WeaponsVan weaponsVan;
std::vector<RoadSegment> roadSegments;
int score = 0;
int distance = 0; // Meters traveled
int extraCars = 2; // Lives

bool initSDL() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) return false;
    window = SDL_CreateWindow("Spy Hunter Clone", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) return false;
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) return false;
    IMG_Init(IMG_INIT_PNG);
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
    return true;
}

SDL_Texture* loadTexture(const char* path) {
    SDL_Surface* surface = IMG_Load(path);
    if (!surface) {
        std::cerr << "Failed to load image: " << IMG_GetError() << std::endl;
        return nullptr;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

void loadAssets() {
    player.carTexture = loadTexture("player_car.png");
    player.boatTexture = loadTexture("player_boat.png");
    player.pos = {SCREEN_WIDTH / 2 - player.width / 2, SCREEN_HEIGHT - 128};

    // Road segments (two for seamless scrolling)
    RoadSegment road1;
    road1.pos = {(SCREEN_WIDTH - ROAD_WIDTH) / 2, 0};
    road1.texture = loadTexture("road.png");
    roadSegments.push_back(road1);
    RoadSegment road2;
    road2.pos = {(SCREEN_WIDTH - ROAD_WIDTH) / 2, -SCREEN_HEIGHT};
    road2.texture = road1.texture;
    roadSegments.push_back(road2);

    // Enemies
    Enemy switchblade = {{SCREEN_WIDTH / 2, -64}, loadTexture("switchblade.png"), 48, 64, 0, true};
    Enemy bully = {{SCREEN_WIDTH / 2 + 100, -128}, loadTexture("bully.png"), 48, 64, 1, true};
    Enemy civilian = {{SCREEN_WIDTH / 2 - 100, -192}, loadTexture("civilian.png"), 48, 64, 3, true};
    enemies.push_back(switchblade);
    enemies.push_back(bully);
    enemies.push_back(civilian);

    weaponsVan.texture = loadTexture("weapons_van.png");
    weaponsVan.pos = {(SCREEN_WIDTH - ROAD_WIDTH) / 2 + ROAD_WIDTH / 2 - 32, -SCREEN_HEIGHT};

    bullets.resize(10);
    for (auto& bullet : bullets) {
        bullet.texture = loadTexture(bullet.isMissile ? "missile.png" : "bullet.png");
    }
    effects.resize(10);
    for (auto& effect : effects) {
        effect.texture = loadTexture(effect.texture == nullptr ? "oil.png" : effect.texture == loadTexture("oil.png") ? "smoke.png" : "oil.png");
    }

    shootSound = Mix_LoadWAV("shoot.wav");
    oilSound = Mix_LoadWAV("oil.wav");
    smokeSound = Mix_LoadWAV("smoke.wav");
    missileSound = Mix_LoadWAV("missile.wav");
    engineMusic = Mix_LoadMUS("engine.wav");
    peterGunnMusic = Mix_LoadMUS("peter_gunn.wav");
    Mix_PlayMusic(peterGunnMusic, -1);
    Mix_PlayMusic(engineMusic, -1);
}

void handleInput(bool& running) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) running = false;
    }

    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    player.velocity.x = 0;

    if (keys[SDL_SCANCODE_LEFT] && player.pos.x > (SCREEN_WIDTH - ROAD_WIDTH) / 2) player.velocity.x = -player.speed;
    if (keys[SDL_SCANCODE_RIGHT] && player.pos.x < (SCREEN_WIDTH + ROAD_WIDTH) / 2 - player.width) player.velocity.x = player.speed;
    if (keys[SDL_SCANCODE_UP]) player.gear = 1; // High gear
    if (keys[SDL_SCANCODE_DOWN]) player.gear = 0; // Low gear

    // Weapons
    if (keys[SDL_SCANCODE_1] && (player.weapons & 1)) { // Machine guns
        for (auto& bullet : bullets) {
            if (!bullet.active) {
                bullet.pos = {player.pos.x + player.width / 2 - 8, player.pos.y};
                bullet.velocity = {0, -BULLET_SPEED};
                bullet.isMissile = false;
                bullet.active = true;
                Mix_PlayChannel(-1, shootSound, 0);
                break;
            }
        }
    }
    if (keys[SDL_SCANCODE_2] && (player.weapons & 2)) { // Oil slick
        for (auto& effect : effects) {
            if (!effect.active) {
                effect.pos = {player.pos.x + player.width / 2 - 16, player.pos.y + player.height};
                effect.texture = loadTexture("oil.png");
                effect.active = true;
                Mix_PlayChannel(-1, oilSound, 0);
                break;
            }
        }
    }
    if (keys[SDL_SCANCODE_3] && (player.weapons & 4)) { // Smoke screen
        for (auto& effect : effects) {
            if (!effect.active) {
                effect.pos = {player.pos.x + player.width / 2 - 16, player.pos.y + player.height};
                effect.texture = loadTexture("smoke.png");
                effect.active = true;
                Mix_PlayChannel(-1, smokeSound, 0);
                break;
            }
        }
    }
    if (keys[SDL_SCANCODE_4] && (player.weapons & 8)) { // Missiles
        for (auto& bullet : bullets) {
            if (!bullet.active) {
                bullet.pos = {player.pos.x + player.width / 2 - 8, player.pos.y};
                bullet.velocity = {0, -MISSILE_SPEED};
                bullet.isMissile = true;
                bullet.active = true;
                Mix_PlayChannel(-1, missileSound, 0);
                break;
            }
        }
    }
}

void update() {
    // Update player
    player.speed = PLAYER_SPEED + player.gear * 2; // High gear increases speed
    player.pos.x += player.velocity.x;

    // Scroll road
    for (auto& segment : roadSegments) {
        segment.pos.y += ROAD_SPEED;
        if (segment.pos.y >= SCREEN_HEIGHT) {
            segment.pos.y -= SCREEN_HEIGHT * 2;
            segment.isSnow = rand() % 10 == 0; // 10% chance of snow
            segment.isWater = rand() % 20 == 0; // 5% chance of water
            segment.hasBoathouse = segment.isWater && rand() % 2 == 0;
            segment.texture = segment.isSnow ? loadTexture("snow_road.png") : segment.isWater ? loadTexture("water.png") : loadTexture("road.png");
        }
    }

    // Weapons van
    if (!weaponsVan.active && rand() % 500 == 0) {
        weaponsVan.active = true;
        weaponsVan.pos.y = -weaponsVan.height;
    }
    if (weaponsVan.active) {
        weaponsVan.pos.y += ROAD_SPEED;
        SDL_Rect playerRect = {player.pos.x, player.pos.y, player.width, player.height};
        SDL_Rect vanRect = {weaponsVan.pos.x, weaponsVan.pos.y, weaponsVan.width, weaponsVan.height};
        if (SDL_HasIntersection(&playerRect, &vanRect)) {
            player.weapons |= (1 << (rand() % MAX_WEAPONS)); // Add random weapon
            weaponsVan.active = false;
        }
        if (weaponsVan.pos.y > SCREEN_HEIGHT) weaponsVan.active = false;
    }

    // Enemies
    for (auto& enemy : enemies) {
        if (!enemy.active) continue;
        enemy.pos.y += ROAD_SPEED;
        if (enemy.pos.y > SCREEN_HEIGHT) {
            enemy.pos.y = -enemy.height;
            enemy.pos.x = (SCREEN_WIDTH - ROAD_WIDTH) / 2 + rand() % (ROAD_WIDTH - enemy.width);
            enemy.active = true;
        }

        // Enemy behavior
        if (enemy.type == 0) { // Switchblade
            if (rand() % 100 < 5) enemy.pos.x += (player.pos.x < enemy.pos.x ? -2 : 2); // Pursue player
        } else if (enemy.type == 1) { // Bully
            enemy.pos.x += (player.pos.x < enemy.pos.x ? -3 : 3); // Ram player
        } else if (enemy.type == 2) { // Enforcer (helicopter mimic)
            enemy.pos.y -= 2; // Move up screen
            if (enemy.pos.y < -enemy.height) enemy.active = false;
        }

        // Collision with player
        SDL_Rect playerRect = {player.pos.x, player.pos.y, player.width, player.height};
        SDL_Rect enemyRect = {enemy.pos.x, enemy.pos.y, enemy.width, enemy.height};
        if (SDL_HasIntersection(&playerRect, &enemyRect)) {
            if (enemy.type == 3) score -= 500; // Civilian penalty
            else { // Crash
                extraCars--;
                if (extraCars < 0) extraCars = 0; // Game over condition
                player.pos = {SCREEN_WIDTH / 2 - player.width / 2, SCREEN_HEIGHT - 128};
            }
            enemy.active = false;
        }
    }

    // Bullets
    for (auto& bullet : bullets) {
        if (bullet.active) {
            bullet.pos.x += bullet.velocity.x;
            bullet.pos.y += bullet.velocity.y;
            if (bullet.pos.y < -16 || bullet.pos.y > SCREEN_HEIGHT) bullet.active = false;

            SDL_Rect bulletRect = {bullet.pos.x, bullet.pos.y, 16, 16};
            for (auto& enemy : enemies) {
                if (!enemy.active) continue;
                SDL_Rect enemyRect = {enemy.pos.x, enemy.pos.y, enemy.width, enemy.height};
                if (SDL_HasIntersection(&bulletRect, &enemyRect)) {
                    bullet.active = false;
                    if (enemy.type != 1 || bullet.isMissile) { // Bullies immune to bullets
                        enemy.active = false;
                        score += (enemy.type == 3 ? -500 : 1000); // Civilian penalty, enemy bonus
                    }
                }
            }
        }
    }

    // Effects (oil, smoke)
    for (auto& effect : effects) {
        if (effect.active) {
            effect.pos.y += ROAD_SPEED;
            effect.lifetime--;
            if (effect.lifetime <= 0) effect.active = false;

            SDL_Rect effectRect = {effect.pos.x, effect.pos.y, 32, 32};
            for (auto& enemy : enemies) {
                if (!enemy.active) continue;
                SDL_Rect enemyRect = {enemy.pos.x, enemy.pos.y, enemy.width, enemy.height};
                if (SDL_HasIntersection(&effectRect, &enemyRect) && enemy.type != 3) {
                    enemy.active = false; // Oil/smoke destroys enemies
                    score += 1000;
                }
            }
        }
    }

    // Boathouse transition
    for (auto& segment : roadSegments) {
        if (segment.hasBoathouse && segment.pos.y + SCREEN_HEIGHT / 2 > player.pos.y && segment.pos.y + SCREEN_HEIGHT / 2 < player.pos.y + player.height) {
            player.isBoat = true;
            player.speed = PLAYER_SPEED + 1; // Boat slightly faster
        }
    }

    // Scoring
    distance += ROAD_SPEED;
    score += distance / 100; // 100 meters = 100 points
    if (distance >= 18000 && extraCars < 3) extraCars++; // Bonus car at 18,000 points
}

void render() {
    SDL_RenderClear(renderer);

    // Render road
    for (const auto& segment : roadSegments) {
        SDL_Rect roadRect = {segment.pos.x, segment.pos.y, segment.width, segment.height};
        SDL_RenderCopy(renderer, segment.texture, nullptr, &roadRect);
        if (segment.hasBoathouse) {
            SDL_Rect boathouseRect = {segment.pos.x + ROAD_WIDTH / 2 - 32, segment.pos.y + SCREEN_HEIGHT / 2 - 32, 64, 64};
            SDL_RenderCopy(renderer, loadTexture("boathouse.png"), nullptr, &boathouseRect);
        }
    }

    // Render weapons van
    if (weaponsVan.active) {
        SDL_Rect vanRect = {weaponsVan.pos.x, weaponsVan.pos.y, weaponsVan.width, weaponsVan.height};
        SDL_RenderCopy(renderer, weaponsVan.texture, nullptr, &vanRect);
    }

    // Render enemies
    for (const auto& enemy : enemies) {
        if (enemy.active) {
            SDL_Rect enemyRect = {enemy.pos.x, enemy.pos.y, enemy.width, enemy.height};
            SDL_RenderCopy(renderer, enemy.texture, nullptr, &enemyRect);
        }
    }

    // Render effects
    for (const auto& effect : effects) {
        if (effect.active) {
            SDL_Rect effectRect = {effect.pos.x, effect.pos.y, 32, 32};
            SDL_RenderCopy(renderer, effect.texture, nullptr, &effectRect);
        }
    }

    // Render bullets
    for (const auto& bullet : bullets) {
        if (bullet.active) {
            SDL_Rect bulletRect = {bullet.pos.x, bullet.pos.y, 16, 16};
            SDL_RenderCopy(renderer, bullet.texture, nullptr, &bulletRect);
        }
    }

    // Render player
    SDL_Rect playerRect = {player.pos.x, player.pos.y, player.width, player.height};
    SDL_RenderCopy(renderer, player.isBoat ? player.boatTexture : player.carTexture, nullptr, &playerRect);

    SDL_RenderPresent(renderer);
}

void cleanup() {
    SDL_DestroyTexture(player.carTexture);
    SDL_DestroyTexture(player.boatTexture);
    SDL_DestroyTexture(roadSegments[0].texture);
    for (auto& enemy : enemies) SDL_DestroyTexture(enemy.texture);
    SDL_DestroyTexture(weaponsVan.texture);
    for (auto& bullet : bullets) SDL_DestroyTexture(bullet.texture);
    for (auto& effect : effects) SDL_DestroyTexture(effect.texture);
    Mix_FreeChunk(shootSound);
    Mix_FreeChunk(oilSound);
    Mix_FreeChunk(smokeSound);
    Mix_FreeChunk(missileSound);
    Mix_FreeMusic(engineMusic);
    Mix_FreeMusic(peterGunnMusic);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_Quit();
    IMG_Quit();
    SDL_Quit();
}

int main(int argc, char* args[]) {
    if (!initSDL()) return 1;
    loadAssets();

    bool running = true;
    while (running && extraCars >= 0) {
        handleInput(running);
        update();
        render();
        SDL_Delay(16); // ~60 FPS
    }

    cleanup();
    return 0;
}
