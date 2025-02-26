#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <cmath>

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define TILE_SIZE 60
#define PLAYER_SPEED 5
#define MAX_ENTITIES 20
#define MAP_WIDTH (SCREEN_WIDTH / TILE_SIZE)  // 32
#define MAP_HEIGHT (SCREEN_HEIGHT / TILE_SIZE) // 18

enum TileType { GRASS, TREE, STONE, WATER };
enum EntityType { WOLF, BEAR };
enum StructureType { NONE, CAMPFIRE, SHELTER, FORGE };

struct Entity {
    int x, y;
    EntityType type;
    int health, speed, damage;
    bool active;
};

struct Resource {
    int x, y;
    TileType type;
    bool collected;
};

struct Structure {
    int x, y;
    StructureType type;
};

struct Player {
    int x, y;
    int health, food, water;
    int wood, stone, meat;
    bool hasPickaxe, hasCampfire;
};

struct World {
    TileType tiles[MAP_HEIGHT][MAP_WIDTH];
};

// Simple noise function (fallback for libnoise)
float simpleNoise(float x, float y) {
    return sin(x * 12.9898 + y * 78.233) * 43758.5453 - floor(sin(x * 12.9898 + y * 78.233) * 43758.5453);
}

void generateWorld(World& world) {
    srand(time(NULL));
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            float noise = simpleNoise(x * 0.1f, y * 0.1f);
            if (noise > 0.3) world.tiles[y][x] = TREE;
            else if (noise < -0.3) world.tiles[y][x] = STONE;
            else if (noise > -0.1 && noise < 0.1) world.tiles[y][x] = WATER;
            else world.tiles[y][x] = GRASS;
        }
    }
}

bool initSDL(SDL_Window** window, SDL_Renderer** renderer, TTF_Font** font, SDL_Texture** textures, Mix_Chunk** craftSound) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0 || IMG_Init(IMG_INIT_PNG) == 0 || TTF_Init() < 0 || Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cout << "SDL init failed: " << SDL_GetError() << std::endl;
        return false;
    }
    *window = SDL_CreateWindow("Survival Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!*window) return false;
    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);
    if (!*renderer) {
        SDL_DestroyWindow(*window);
        return false;
    }
    *font = TTF_OpenFont("font.ttf", 16);
    if (!*font) {
        SDL_DestroyRenderer(*renderer);
        SDL_DestroyWindow(*window);
        return false;
    }
    
    const char* files[] = {"player.png", "wolf.png", "bear.png", "tree.png", "stone.png", "water.png"};
    for (int i = 0; i < 6; i++) {
        textures[i] = IMG_LoadTexture(*renderer, files[i]);
        if (!textures[i]) {
            for (int j = 0; j < i; j++) SDL_DestroyTexture(textures[j]);
            TTF_CloseFont(*font);
            SDL_DestroyRenderer(*renderer);
            SDL_DestroyWindow(*window);
            std::cout << "Failed to load " << files[i] << ": " << IMG_GetError() << std::endl;
            return false;
        }
    }
    
    *craftSound = Mix_LoadWAV("craft.wav");
    if (!*craftSound) {
        for (int i = 0; i < 6; i++) SDL_DestroyTexture(textures[i]);
        TTF_CloseFont(*font);
        SDL_DestroyRenderer(*renderer);
        SDL_DestroyWindow(*window);
        std::cout << "Failed to load craft.wav: " << Mix_GetError() << std::endl;
        return false;
    }
    
    return true;
}

void spawnEntities(World& world, std::vector<Entity>& entities, std::vector<Resource>& resources) {
    srand(time(NULL));
    for (int i = 0; i < MAX_ENTITIES; i++) {
        int x, y;
        do {
            x = rand() % MAP_WIDTH * TILE_SIZE;
            y = rand() % MAP_HEIGHT * TILE_SIZE;
        } while (world.tiles[y / TILE_SIZE][x / TILE_SIZE] != GRASS);
        EntityType type = i % 2 == 0 ? WOLF : BEAR;
        entities.push_back({x, y, type, 50, type == WOLF ? 3 : 2, type == WOLF ? 5 : 10, true});
    }
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (world.tiles[y][x] == TREE || world.tiles[y][x] == STONE) {
                resources.push_back({x * TILE_SIZE, y * TILE_SIZE, world.tiles[y][x], false});
            }
        }
    }
}

bool canMove(int newX, int newY, World& world, std::vector<Resource>& resources, std::vector<Structure>& structures) {
    int tileX = newX / TILE_SIZE, tileY = newY / TILE_SIZE;
    if (newX < 0 || newX >= SCREEN_WIDTH - TILE_SIZE || newY < 0 || newY >= SCREEN_HEIGHT - TILE_SIZE) return false;
    if (world.tiles[tileY][tileX] == WATER) return false;
    for (const auto& res : resources) {
        if (!res.collected && abs(newX - res.x) < TILE_SIZE && abs(newY - res.y) < TILE_SIZE) return false;
    }
    for (const auto& structure : structures) {
        if (abs(newX - structure.x) < TILE_SIZE && abs(newY - structure.y) < TILE_SIZE) return false;
    }
    return true;
}

bool isNearWater(Player& player, World& world) {
    for (int y = std::max(0, player.y / TILE_SIZE - 1); y <= std::min(MAP_HEIGHT - 1, player.y / TILE_SIZE + 1); y++) {
        for (int x = std::max(0, player.x / TILE_SIZE - 1); x <= std::min(MAP_WIDTH - 1, player.x / TILE_SIZE + 1); x++) {
            if (world.tiles[y][x] == WATER) return true;
        }
    }
    return false;
}

bool isNearStructure(Player& player, std::vector<Structure>& structures, StructureType type) {
    for (const auto& structure : structures) {
        if (structure.type == type && abs(player.x - structure.x) < TILE_SIZE * 2 && abs(player.y - structure.y) < TILE_SIZE * 2) {
            return true;
        }
    }
    return false;
}

void update(Player& player, std::vector<Entity>& entities, std::vector<Resource>& resources, std::vector<Structure>& structures, World& world, int gameTime) {
    player.food -= 1; player.water -= 1;
    if (player.food <= 0 || player.water <= 0) player.health -= 1;
    if (player.health <= 0) player.health = 0;
    if (player.food < 0) player.food = 0;
    if (player.water < 0) player.water = 0;

    int activeEnemies = 0;
    for (auto& entity : entities) {
        if (!entity.active) continue;
        activeEnemies++;
        entity.health = std::min(100, entity.health + gameTime / 60); // Cap at 100
        entity.damage = std::min(20, entity.damage + gameTime / 120); // Cap at 20
        
        int dx = player.x - entity.x, dy = player.y - entity.y;
        int dist = sqrt(dx * dx + dy * dy);
        if (dist < 5 * TILE_SIZE) {
            if (dx > 0) entity.x += entity.speed; else if (dx < 0) entity.x -= entity.speed;
            if (dy > 0) entity.y += entity.speed; else if (dy < 0) entity.y -= entity.speed;
            if (dist < TILE_SIZE) player.health -= entity.damage;
        }
    }

    if (activeEnemies < MAX_ENTITIES / 2 && rand() % 60 == 0) {
        int x, y;
        do {
            x = rand() % MAP_WIDTH * TILE_SIZE;
            y = rand() % MAP_HEIGHT * TILE_SIZE;
        } while (world.tiles[y / TILE_SIZE][x / TILE_SIZE] != GRASS || abs(player.x - x) < 5 * TILE_SIZE);
        EntityType type = rand() % 2 == 0 ? WOLF : BEAR;
        entities.push_back({x, y, type, 50, type == WOLF ? 3 : 2, type == WOLF ? 5 : 10, true});
    }

    for (auto& res : resources) {
        if (!res.collected && abs(player.x - res.x) < TILE_SIZE && abs(player.y - res.y) < TILE_SIZE) {
            if (res.type == TREE) player.wood += 10;
            else if (res.type == STONE && player.hasPickaxe) player.stone += 10;
            res.collected = true;
        }
    }
}

void renderCraftingMenu(SDL_Renderer* renderer, TTF_Font* font, Player& player, bool showMenu) {
    if (!showMenu) return;
    SDL_Color white = {255, 255, 255};
    const char* options[] = {
        "1. Pickaxe (10 wood, 5 stone)",
        "2. Campfire (20 wood)",
        "3. Shelter (50 wood, 20 stone)",
        "4. Forge (50 stone, 20 wood)"
    };
    for (int i = 0; i < 4; i++) {
        SDL_Surface* surface = TTF_RenderText_Solid(font, options[i], white);
        SDL_Texture* text = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect rect = {SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 60 + i * 30, surface->w, surface->h};
        SDL_RenderCopy(renderer, text, NULL, &rect);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(text);
    }
}

void render(SDL_Renderer* renderer, TTF_Font* font, SDL_Texture** textures, Player& player, std::vector<Entity>& entities, 
            std::vector<Resource>& resources, std::vector<Structure>& structures, World& world, bool showCraftingMenu) {
    SDL_SetRenderDrawColor(renderer, 0, 100, 0, 255); // Grass
    SDL_RenderClear(renderer);

    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (world.tiles[y][x] == WATER) {
                SDL_Rect rect = {x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE};
                SDL_RenderCopy(renderer, textures[5], NULL, &rect);
            }
        }
    }

    for (const auto& res : resources) {
        if (!res.collected) {
            SDL_Rect rect = {res.x, res.y, TILE_SIZE, TILE_SIZE};
            SDL_RenderCopy(renderer, textures[res.type == TREE ? 3 : 4], NULL, &rect);
        }
    }

    SDL_SetRenderDrawColor(renderer, 139, 69, 19, 255); // Brown for structures
    for (const auto& structure : structures) {
        SDL_Rect rect = {structure.x, structure.y, TILE_SIZE, TILE_SIZE};
        SDL_RenderFillRect(renderer, &rect);
    }

    for (const auto& entity : entities) {
        if (entity.active) {
            SDL_Rect rect = {entity.x, entity.y, TILE_SIZE, TILE_SIZE};
            SDL_RenderCopy(renderer, textures[entity.type == WOLF ? 1 : 2], NULL, &rect);
        }
    }

    SDL_Rect playerRect = {player.x, player.y, TILE_SIZE, TILE_SIZE};
    SDL_RenderCopy(renderer, textures[0], NULL, &playerRect);

    // HUD
    SDL_Color white = {255, 255, 255};
    std::string hudText = "Wood: " + std::to_string(player.wood) + " Stone: " + std::to_string(player.stone) + 
                          " Meat: " + std::to_string(player.meat) + " Pickaxe: " + (player.hasPickaxe ? "Yes" : "No");
    SDL_Surface* surface = TTF_RenderText_Solid(font, hudText.c_str(), white);
    SDL_Texture* text = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect textRect = {10, SCREEN_HEIGHT - 40, surface->w, surface->h};
    SDL_RenderCopy(renderer, text, NULL, &textRect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(text);

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_Rect healthBar = {10, 10, player.health * 2, 20};
    SDL_RenderFillRect(renderer, &healthBar);
    SDL_SetRenderDrawColor(renderer, 255, 165, 0, 255);
    SDL_Rect foodBar = {10, 40, player.food * 2, 20};
    SDL_RenderFillRect(renderer, &foodBar);
    SDL_SetRenderDrawColor(renderer, 0, 191, 255, 255);
    SDL_Rect waterBar = {10, 70, player.water * 2, 20};
    SDL_RenderFillRect(renderer, &waterBar);

    renderCraftingMenu(renderer, font, player, showCraftingMenu);
    SDL_RenderPresent(renderer);
}

int main() {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    TTF_Font* font = nullptr;
    SDL_Texture* textures[6] = {nullptr};
    Mix_Chunk* craftSound = nullptr;
    if (!initSDL(&window, &renderer, &font, textures, &craftSound)) {
        std::cout << "Initialization failed!" << std::endl;
        return 1;
    }

    World world;
    generateWorld(world);
    Player player = {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 100, 100, 100, 0, 0, 0, false, false};
    std::vector<Entity> entities;
    std::vector<Resource> resources;
    std::vector<Structure> structures;
    spawnEntities(world, entities, resources);

    bool quit = false;
    SDL_Event e;
    Uint32 lastTick = SDL_GetTicks();
    int gameTime = 0;
    bool showCraftingMenu = false;

    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) quit = true;
            if (e.type == SDL_KEYDOWN) {
                int newX = player.x, newY = player.y;
                switch (e.key.keysym.sym) {
                    case SDLK_UP: newY -= PLAYER_SPEED; break;
                    case SDLK_DOWN: newY += PLAYER_SPEED; break;
                    case SDLK_LEFT: newX -= PLAYER_SPEED; break;
                    case SDLK_RIGHT: newX += PLAYER_SPEED; break;
                    case SDLK_SPACE: // Attack
                        for (auto& entity : entities) {
                            if (entity.active && abs(player.x - entity.x) < TILE_SIZE * 2 && abs(player.y - entity.y) < TILE_SIZE * 2) {
                                entity.health -= 20;
                                if (entity.health <= 0) {
                                    entity.active = false;
                                    player.meat += 10;
                                }
                            }
                        }
                        break;
                    case SDLK_c: // Toggle crafting menu
                        showCraftingMenu = !showCraftingMenu;
                        break;
                    case SDLK_1: // Craft pickaxe
                        if (showCraftingMenu && !player.hasPickaxe && player.wood >= 10 && player.stone >= 5) {
                            player.hasPickaxe = true;
                            player.wood -= 10; player.stone -= 5;
                            Mix_PlayChannel(-1, craftSound, 0);
                            showCraftingMenu = false;
                        }
                        break;
                    case SDLK_2: // Craft campfire
                        if (showCraftingMenu && player.wood >= 20) {
                            player.hasCampfire = true;
                            player.wood -= 20;
                            structures.push_back({player.x, player.y, CAMPFIRE});
                            Mix_PlayChannel(-1, craftSound, 0);
                            showCraftingMenu = false;
                        }
                        break;
                    case SDLK_3: // Craft shelter
                        if (showCraftingMenu && player.wood >= 50 && player.stone >= 20) {
                            player.wood -= 50; player.stone -= 20;
                            structures.push_back({player.x, player.y, SHELTER});
                            Mix_PlayChannel(-1, craftSound, 0);
                            showCraftingMenu = false;
                        }
                        break;
                    case SDLK_4: // Craft forge
                        if (showCraftingMenu && player.stone >= 50 && player.wood >= 20 && structures.size() > 1) {
                            player.stone -= 50; player.wood -= 20;
                            structures.push_back({player.x, player.y, FORGE});
                            Mix_PlayChannel(-1, craftSound, 0);
                            showCraftingMenu = false;
                        }
                        break;
                    case SDLK_f: // Eat meat
                        if (player.meat >= 5) {
                            player.meat -= 5;
                            player.food = std::min(100, player.food + 20);
                        }
                        break;
                    case SDLK_w: // Drink purified water
                        if (isNearStructure(player, structures, CAMPFIRE) && isNearWater(player, world)) {
                            player.water = std::min(100, player.water + 20);
                        }
                        break;
                }
                if (canMove(newX, newY, world, resources, structures)) {
                    player.x = newX;
                    player.y = newY;
                }
            }
        }

        Uint32 currentTick = SDL_GetTicks();
        if (currentTick - lastTick >= 1000) {
            update(player, entities, resources, structures, world, gameTime);
            gameTime++;
            lastTick = currentTick;
        }

        render(renderer, font, textures, player, entities, resources, structures, world, showCraftingMenu);
        if (player.health <= 0) {
            SDL_Color red = {255, 0, 0};
            SDL_Surface* surface = TTF_RenderText_Solid(font, "Game Over!", red);
            SDL_Texture* text = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_Rect textRect = {SCREEN_WIDTH / 2 - surface->w / 2, SCREEN_HEIGHT / 2 - surface->h / 2, surface->w, surface->h};
            SDL_RenderCopy(renderer, text, NULL, &textRect);
            SDL_RenderPresent(renderer);
            SDL_FreeSurface(surface);
            SDL_DestroyTexture(text);
            SDL_Delay(2000); // Show for 2 seconds
            quit = true;
        }

        SDL_Delay(16); // ~60 FPS
    }

    Mix_FreeChunk(craftSound);
    for (int i = 0; i < 6; i++) SDL_DestroyTexture(textures[i]);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_CloseAudio();
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 0;
}
