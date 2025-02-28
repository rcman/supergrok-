#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include <string>
#include <iostream>
#include <cmath>

// Constants
const int SCREEN_WIDTH = 640;  // 20 tiles wide
const int SCREEN_HEIGHT = 480; // 15 tiles tall
const int TILE_SIZE = 32;
const int MAP_WIDTH = 20;
const int MAP_HEIGHT = 15;
const float GRAVITY = 0.2f;
const float ACCEL = 0.1f;
const float MAX_SPEED = 2.0f;

// Tile Types
enum TileType {
    EMPTY, BRICK, CONCRETE, LADDER, ROPE, GOLD
};

// Entity Structure
struct Entity {
    float x, y;    // Position
    float vx, vy;  // Velocity
    SDL_Texture* texture;
    bool alive = true;
    int trappedTimer = 0; // For enemies in holes
};

// Hole Structure
struct Hole {
    int x, y;
    int timer; // Time until hole refills
};

// Game Class
class Game {
public:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    TTF_Font* font = nullptr;
    SDL_Texture* playerTexture = nullptr;
    SDL_Texture* guardTexture = nullptr;
    SDL_Texture* goldTexture = nullptr;
    SDL_Texture* brickTexture = nullptr;
    SDL_Texture* concreteTexture = nullptr;
    SDL_Texture* ladderTexture = nullptr;
    SDL_Texture* ropeTexture = nullptr;
    int map[MAP_HEIGHT][MAP_WIDTH];
    Entity player;
    std::vector<Entity> guards;
    std::vector<Hole> holes;
    int goldCount = 0;
    bool running = true;
    bool levelComplete = false;
    bool editing = false;
    int editX = 0, editY = 0;

    Game() {
        initMap();
        player = {float(MAP_WIDTH / 2) * TILE_SIZE, 0, 0, 0, nullptr};
        guards.push_back({float(MAP_WIDTH - 2) * TILE_SIZE, 0, 0, 0, nullptr});
        guards.push_back({2 * TILE_SIZE, 0, 0, 0, nullptr});
    }

    bool init() {
        auto cleanupOnFailure = [this]() {
            if (playerTexture) SDL_DestroyTexture(playerTexture);
            if (guardTexture) SDL_DestroyTexture(guardTexture);
            if (goldTexture) SDL_DestroyTexture(goldTexture);
            if (brickTexture) SDL_DestroyTexture(brickTexture);
            if (concreteTexture) SDL_DestroyTexture(concreteTexture);
            if (ladderTexture) SDL_DestroyTexture(ladderTexture);
            if (ropeTexture) SDL_DestroyTexture(ropeTexture);
            if (font) TTF_CloseFont(font);
            if (renderer) SDL_DestroyRenderer(renderer);
            if (window) SDL_DestroyWindow(window);
            TTF_Quit();
            IMG_Quit();
            SDL_Quit();
        };

        if (SDL_Init(SDL_INIT_VIDEO) < 0 || IMG_Init(IMG_INIT_PNG) == 0 || TTF_Init() < 0) {
            std::cerr << "SDL Init failed: " << SDL_GetError() << std::endl;
            cleanupOnFailure();
            return false;
        }
        window = SDL_CreateWindow("Lode Runner", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
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
        playerTexture = IMG_LoadTexture(renderer, "player.png");
        guardTexture = IMG_LoadTexture(renderer, "guard.png");
        goldTexture = IMG_LoadTexture(renderer, "gold.png");
        brickTexture = IMG_LoadTexture(renderer, "brick.png");
        concreteTexture = IMG_LoadTexture(renderer, "concrete.png");
        ladderTexture = IMG_LoadTexture(renderer, "ladder.png");
        ropeTexture = IMG_LoadTexture(renderer, "rope.png");

        if (!playerTexture || !guardTexture || !goldTexture || !brickTexture || 
            !concreteTexture || !ladderTexture || !ropeTexture) {
            std::cerr << "Texture load failed: " << IMG_GetError() << std::endl;
            cleanupOnFailure();
            return false;
        }

        player.texture = playerTexture;
        for (auto& guard : guards) guard.texture = guardTexture;
        return true;
    }

    void initMap() {
        const char* level[] = {
            "CCCCCCCCCCCCCCCCCCCC",
            "C____B____G____B___C",
            "C_BB___BB___BB___B_C",
            "C_____G____G____L__C",
            "C_BB___BB___BB___B_C",
            "C__________R_______C",
            "C_BB___BB___BB___B_C",
            "C_____G____G____L__C",
            "C_BB___BB___BB___B_C",
            "C__________R_____G_C",
            "C_BB___BB___BB___B_C",
            "C_____G____G____L__C",
            "C_BB___BB___BB___B_C",
            "C__________________C",
            "CCCCCCCCCCCCCCCCCCCC"
        };
        goldCount = 0;
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                switch (level[y][x]) {
                    case 'C': map[y][x] = CONCRETE; break;
                    case 'B': map[y][x] = BRICK; break;
                    case 'L': map[y][x] = LADDER; break;
                    case 'R': map[y][x] = ROPE; break;
                    case 'G': map[y][x] = GOLD; goldCount++; break;
                    default: map[y][x] = EMPTY; break;
                }
            }
        }
    }

    bool isSolid(int x, int y) {
        if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) return true;
        return map[y][x] == BRICK || map[y][x] == CONCRETE;
    }

    bool isLadder(int x, int y) {
        if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) return false;
        return map[y][x] == LADDER;
    }

    bool isRope(int x, int y) {
        if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) return false;
        return map[y][x] == ROPE;
    }

    void handleInput() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_e) {
                editing = !editing;
            }
        }
        const Uint8* keys = SDL_GetKeyboardState(NULL);

        if (editing) {
            if (keys[SDL_SCANCODE_LEFT] && editX > 0) editX--;
            if (keys[SDL_SCANCODE_RIGHT] && editX < MAP_WIDTH - 1) editX++;
            if (keys[SDL_SCANCODE_UP] && editY > 0) editY--;
            if (keys[SDL_SCANCODE_DOWN] && editY < MAP_HEIGHT - 1) editY++;
            if (keys[SDL_SCANCODE_1]) map[editY][editX] = EMPTY;
            if (keys[SDL_SCANCODE_2]) map[editY][editX] = BRICK;
            if (keys[SDL_SCANCODE_3]) map[editY][editX] = CONCRETE;
            if (keys[SDL_SCANCODE_4]) map[editY][editX] = LADDER;
            if (keys[SDL_SCANCODE_5]) map[editY][editX] = ROPE;
            if (keys[SDL_SCANCODE_6]) { 
                if (map[editY][editX] != GOLD) goldCount++; 
                map[editY][editX] = GOLD; 
            }
            if (keys[SDL_SCANCODE_R]) { initMap(); player.x = float(MAP_WIDTH / 2) * TILE_SIZE; player.y = 0; player.alive = true; }
        } else if (player.alive) {
            if (keys[SDL_SCANCODE_LEFT]) player.vx -= ACCEL;
            else if (keys[SDL_SCANCODE_RIGHT]) player.vx += ACCEL;
            else player.vx *= 0.8f;
            player.vx = std::max(-MAX_SPEED, std::min(MAX_SPEED, player.vx));

            int px = int(std::round(player.x / TILE_SIZE));
            int py = int(std::round(player.y / TILE_SIZE));
            if (keys[SDL_SCANCODE_UP] && isLadder(px, py)) player.vy = -MAX_SPEED;
            else if (keys[SDL_SCANCODE_DOWN] && isLadder(px, py)) player.vy = MAX_SPEED;
            else if (isLadder(px, py) || isRope(px, py)) player.vy = 0;

            if (keys[SDL_SCANCODE_SPACE]) {
                if (keys[SDL_SCANCODE_LEFT] && map[py + 1][px - 1] == BRICK) {
                    map[py + 1][px - 1] = EMPTY;
                    holes.push_back({px - 1, py + 1, 3000});
                } else if (keys[SDL_SCANCODE_RIGHT] && map[py + 1][px + 1] == BRICK) {
                    map[py + 1][px + 1] = EMPTY;
                    holes.push_back({px + 1, py + 1, 3000});
                }
            }
        }
    }

    void update() {
        if (editing) return;

        // Player Movement
        if (player.alive) {
            int px = int(std::round(player.x / TILE_SIZE));
            int py = int(std::round(player.y / TILE_SIZE));
            if (!isSolid(px, py + 1) && !isLadder(px, py) && !isRope(px, py)) {
                player.vy += GRAVITY;
            }
            float newX = player.x + player.vx;
            float newY = player.y + player.vy;

            if (isSolid(int(std::round(newX / TILE_SIZE)), py) || 
                (player.vy < 0 && isSolid(px, py - 1))) {
                newX = player.x;
                player.vy = 0;
            }
            if (isSolid(px, int(std::round(newY / TILE_SIZE)))) newY = player.y;
            player.x = newX;
            player.y = newY;
            if (player.y >= SCREEN_HEIGHT) { 
                player.alive = false; 
                player.y = 0; 
                player.x = float(MAP_WIDTH / 2) * TILE_SIZE; 
                player.vx = 0; 
                player.vy = 0; 
            }

            // Collect Gold
            if (map[py][px] == GOLD) {
                map[py][px] = EMPTY;
                goldCount--;
                if (goldCount == 0) levelComplete = true;
            }
        }

        // Guard Movement
        for (auto& guard : guards) {
            if (!guard.alive) {
                if (guard.trappedTimer > 0) {
                    guard.trappedTimer -= 16;
                    bool stillTrapped = false;
                    for (const auto& hole : holes) {
                        if (int(std::round(guard.x / TILE_SIZE)) == hole.x && 
                            int(std::round(guard.y / TILE_SIZE)) == hole.y) {
                            stillTrapped = true;
                            break;
                        }
                    }
                    if (guard.trappedTimer <= 0 || !stillTrapped) {
                        guard.alive = true;
                        guard.trappedTimer = 0;
                    }
                }
                continue;
            }
            float dx = player.x - guard.x;
            guard.vx += (dx > 0 ? ACCEL : -ACCEL);
            guard.vx = std::max(-MAX_SPEED * 0.75f, std::min(MAX_SPEED * 0.75f, guard.vx));
            int gx = int(std::round(guard.x / TILE_SIZE));
            int gy = int(std::round(guard.y / TILE_SIZE));
            if (!isSolid(gx, gy + 1) && !isLadder(gx, gy) && !isRope(gx, gy)) {
                guard.vy += GRAVITY;
            }
            float newX = guard.x + guard.vx;
            float newY = guard.y + guard.vy;

            if (isSolid(int(std::round(newX / TILE_SIZE)), gy) || 
                (guard.vy < 0 && isSolid(gx, gy - 1))) {
                newX = guard.x;
                guard.vy = 0;
            }
            if (isSolid(gx, int(std::round(newY / TILE_SIZE)))) newY = guard.y;
            guard.x = newX;
            guard.y = newY;

            // Trap in Holes
            for (auto& hole : holes) {
                if (int(std::round(guard.x / TILE_SIZE)) == hole.x && 
                    int(std::round(guard.y / TILE_SIZE)) == hole.y) {
                    guard.alive = false;
                    guard.trappedTimer = 5000;
                }
            }
            if (guard.y >= SCREEN_HEIGHT) { 
                guard.y = 0; 
                guard.x = float(rand() % MAP_WIDTH) * TILE_SIZE; 
                guard.alive = true; 
                guard.vx = 0; 
                guard.vy = 0; 
            }
        }

        // Update Holes
        for (auto it = holes.begin(); it != holes.end();) {
            it->timer -= 16;
            if (it->timer <= 0) {
                map[it->y][it->x] = BRICK;
                it = holes.erase(it);
            } else ++it;
        }

        // Collision with Guards
        for (auto& guard : guards) {
            if (guard.alive && abs(player.x - guard.x) < TILE_SIZE && abs(player.y - guard.y) < TILE_SIZE) {
                player.alive = false;
                player.y = 0;
                player.x = float(MAP_WIDTH / 2) * TILE_SIZE;
                player.vx = 0;
                player.vy = 0;
            }
        }
    }

    void render() {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Render Map
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                SDL_Rect dest = {x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE};
                switch (map[y][x]) {
                    case BRICK: SDL_RenderCopy(renderer, brickTexture, NULL, &dest); break;
                    case CONCRETE: SDL_RenderCopy(renderer, concreteTexture, NULL, &dest); break;
                    case LADDER: SDL_RenderCopy(renderer, ladderTexture, NULL, &dest); break;
                    case ROPE: SDL_RenderCopy(renderer, ropeTexture, NULL, &dest); break;
                    case GOLD: SDL_RenderCopy(renderer, goldTexture, NULL, &dest); break;
                }
            }
        }

        // Render Player
        if (player.alive) {
            SDL_Rect playerRect = {int(std::round(player.x)), int(std::round(player.y)), TILE_SIZE, TILE_SIZE};
            SDL_RenderCopy(renderer, playerTexture, NULL, &playerRect);
        }

        // Render Guards
        for (auto& guard : guards) {
            if (guard.alive || guard.trappedTimer > 0) {
                SDL_Rect guardRect = {int(std::round(guard.x)), int(std::round(guard.y)), TILE_SIZE, TILE_SIZE};
                SDL_RenderCopy(renderer, guardTexture, NULL, &guardRect);
            }
        }

        // Render Editor Cursor
        if (editing) {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            SDL_Rect cursor = {editX * TILE_SIZE, editY * TILE_SIZE, TILE_SIZE, TILE_SIZE};
            SDL_RenderDrawRect(renderer, &cursor);
        }

        // Render UI
        SDL_Color color = {255, 255, 255, 255};
        std::string text = "Gold: " + std::to_string(goldCount) + (editing ? " [Editing]" : "");
        if (levelComplete) text = "Level Complete!";
        else if (!player.alive) text += " - Dead";
        SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), color);
        if (surface) {
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            if (texture) {
                SDL_Rect dest = {10, 10, surface->w, surface->h};
                SDL_RenderCopy(renderer, texture, NULL, &dest);
                SDL_DestroyTexture(texture);
            }
            SDL_FreeSurface(surface);
        } else {
            std::cerr << "Text rendering failed: " << TTF_GetError() << std::endl;
        }

        SDL_RenderPresent(renderer);
    }

    void clean() {
        if (playerTexture) SDL_DestroyTexture(playerTexture);
        if (guardTexture) SDL_DestroyTexture(guardTexture);
        if (goldTexture) SDL_DestroyTexture(goldTexture);
        if (brickTexture) SDL_DestroyTexture(brickTexture);
        if (concreteTexture) SDL_DestroyTexture(concreteTexture);
        if (ladderTexture) SDL_DestroyTexture(ladderTexture);
        if (ropeTexture) SDL_DestroyTexture(ropeTexture);
        if (font) TTF_CloseFont(font);
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
        guards.clear();
        holes.clear();
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
