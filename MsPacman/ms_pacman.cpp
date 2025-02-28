#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include <string>
#include <iostream>
#include <cmath>

// Constants
const int SCREEN_WIDTH = 448;  // 28 tiles wide
const int SCREEN_HEIGHT = 496; // 31 tiles tall
const int TILE_SIZE = 16;
const int MAP_WIDTH = 28;
const int MAP_HEIGHT = 31;
const float SPEED = 2.0f;

// Tile Types
enum TileType {
    EMPTY, WALL, DOT, PELLET
};

// Ghost States
enum GhostState {
    NORMAL, VULNERABLE
};

// Entity Structure
struct Entity {
    float x, y;    // Position
    float vx, vy;  // Velocity
    SDL_Texture* texture;
    GhostState state = NORMAL;
    int vulnerableTimer = 0;
};

// Game Class
class Game {
public:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    TTF_Font* font = nullptr;
    SDL_Texture* pacmanTexture = nullptr;
    SDL_Texture* blinkyTexture = nullptr;
    SDL_Texture* pinkyTexture = nullptr;
    SDL_Texture* inkyTexture = nullptr;
    SDL_Texture* clydeTexture = nullptr;
    SDL_Texture* vulnerableTexture = nullptr;
    SDL_Texture* dotTexture = nullptr;
    SDL_Texture* pelletTexture = nullptr;
    SDL_Texture* cherryTexture = nullptr;
    SDL_Texture* wallTexture = nullptr; // Added missing wall texture
    int map[MAP_HEIGHT][MAP_WIDTH];
    Entity pacman;
    std::vector<Entity> ghosts;
    int dotCount = 0;
    int score = 0;
    int lives = 3;
    bool running = true;
    bool fruitActive = false;
    int fruitTimer = 10000; // 10 seconds until fruit appears

    Game() {
        initMap();
        pacman = {13.5f * TILE_SIZE, 23 * TILE_SIZE, 0, 0, nullptr};
        ghosts.push_back({13.5f * TILE_SIZE, 11 * TILE_SIZE, SPEED, 0, nullptr}); // Blinky
        ghosts.push_back({11.5f * TILE_SIZE, 14 * TILE_SIZE, -SPEED, 0, nullptr}); // Pinky
        ghosts.push_back({15.5f * TILE_SIZE, 14 * TILE_SIZE, SPEED, 0, nullptr});  // Inky
        ghosts.push_back({13.5f * TILE_SIZE, 17 * TILE_SIZE, 0, SPEED, nullptr});  // Clyde
    }

    bool init() {
        auto cleanupOnFailure = [this]() {
            if (pacmanTexture) SDL_DestroyTexture(pacmanTexture);
            if (blinkyTexture) SDL_DestroyTexture(blinkyTexture);
            if (pinkyTexture) SDL_DestroyTexture(pinkyTexture);
            if (inkyTexture) SDL_DestroyTexture(inkyTexture);
            if (clydeTexture) SDL_DestroyTexture(clydeTexture);
            if (vulnerableTexture) SDL_DestroyTexture(vulnerableTexture);
            if (dotTexture) SDL_DestroyTexture(dotTexture);
            if (pelletTexture) SDL_DestroyTexture(pelletTexture);
            if (cherryTexture) SDL_DestroyTexture(cherryTexture);
            if (wallTexture) SDL_DestroyTexture(wallTexture);
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
        window = SDL_CreateWindow("Ms. Pac-Man - Level 1", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
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
        pacmanTexture = IMG_LoadTexture(renderer, "pacman.png");
        blinkyTexture = IMG_LoadTexture(renderer, "blinky.png");
        pinkyTexture = IMG_LoadTexture(renderer, "pinky.png");
        inkyTexture = IMG_LoadTexture(renderer, "inky.png");
        clydeTexture = IMG_LoadTexture(renderer, "clyde.png");
        vulnerableTexture = IMG_LoadTexture(renderer, "vulnerable.png");
        dotTexture = IMG_LoadTexture(renderer, "dot.png");
        pelletTexture = IMG_LoadTexture(renderer, "pellet.png");
        cherryTexture = IMG_LoadTexture(renderer, "cherry.png");
        wallTexture = IMG_LoadTexture(renderer, "wall.png");

        if (!font || !pacmanTexture || !blinkyTexture || !pinkyTexture || !inkyTexture || 
            !clydeTexture || !vulnerableTexture || !dotTexture || !pelletTexture || 
            !cherryTexture || !wallTexture) {
            std::cerr << "Asset load failed: " << IMG_GetError() << std::endl;
            cleanupOnFailure();
            return false;
        }

        pacman.texture = pacmanTexture;
        ghosts[0].texture = blinkyTexture;
        ghosts[1].texture = pinkyTexture;
        ghosts[2].texture = inkyTexture;
        ghosts[3].texture = clydeTexture;
        return true;
    }

    void initMap() {
        const char* level[] = {
            "WWWWWWWWWWWWWWWWWWWWWWWWWWWW",
            "W............WW............W",
            "W.WWWW.WWWWW.WW.WWWWW.WWWW.W",
            "W.WWWW.WWWWW.WW.WWWWW.WWWW.W",
            "W..........................W",
            "W.WWWW.WW.WWWWWWWW.WW.WWWW.W",
            "W.WWWW.WW.WWWWWWWW.WW.WWWW.W",
            "W......WW....WW....WW......W",
            "WWWWWW.WWWWW.WW.WWWWW.WWWWWW",
            "     W.WWWWW.WW.WWWWW.W     ",
            "     W.WW          WW.W     ",
            "     W.WW WWW  WWW WW.W     ",
            "WWWWWW.WW W      W WW.WWWWWW",
            "      .   W      W   .      ",
            "WWWWWW.WW W      W WW.WWWWWW",
            "     W.WW WWWWWWWW WW.W     ",
            "     W.WW          WW.W     ",
            "     W.WW WWWWWWWW WW.W     ",
            "WWWWWW.WW.WWWWWWWW.WW.WWWWWW",
            "W............WW............W",
            "W.WWWW.WWWWW.WW.WWWWW.WWWW.W",
            "W.WWWW.WWWWW.WW.WWWWW.WWWW.W",
            "WP....WW....WW....WW....P.W",
            "WWW.WW.WW.WWWWWWWW.WW.WW.WWW",
            "WWW.WW.WW.WWWWWWWW.WW.WW.WWW",
            "W......WW....WW....WW......W",
            "W.WWWW.WWWWW.WW.WWWWW.WWWW.W",
            "W.WWWW.WWWWW.WW.WWWWW.WWWW.W",
            "W..........................W",
            "W............WW............W",
            "WWWWWWWWWWWWWWWWWWWWWWWWWWWW"
        };
        dotCount = 0;
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                switch (level[y][x]) {
                    case 'W': map[y][x] = WALL; break;
                    case 'D': map[y][x] = DOT; dotCount++; break;
                    case 'P': map[y][x] = PELLET; dotCount++; break;
                    default: map[y][x] = EMPTY; break;
                }
            }
        }
    }

    bool isWall(int x, int y) {
        if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) return true;
        return map[y][x] == WALL;
    }

    void handleInput() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
        }
        const Uint8* keys = SDL_GetKeyboardState(NULL);
        pacman.vx = pacman.vy = 0;
        if (keys[SDL_SCANCODE_LEFT]) pacman.vx = -SPEED;
        if (keys[SDL_SCANCODE_RIGHT]) pacman.vx = SPEED;
        if (keys[SDL_SCANCODE_UP]) pacman.vy = -SPEED;
        if (keys[SDL_SCANCODE_DOWN]) pacman.vy = SPEED;
    }

    void update() {
        // Pac-Man Movement
        float newX = pacman.x + pacman.vx;
        float newY = pacman.y + pacman.vy;
        int px = int(std::round(newX / TILE_SIZE));
        int py = int(std::round(newY / TILE_SIZE));
        if (!isWall(px, py)) {
            pacman.x = newX;
            pacman.y = newY;
        }
        if (pacman.x < 0) pacman.x = (MAP_WIDTH - 1) * TILE_SIZE;
        if (pacman.x >= MAP_WIDTH * TILE_SIZE) pacman.x = 0;

        // Collect Dots/Pellets
        if (px >= 0 && px < MAP_WIDTH && py >= 0 && py < MAP_HEIGHT) {
            if (map[py][px] == DOT) {
                map[py][px] = EMPTY;
                score += 10;
                dotCount--;
            } else if (map[py][px] == PELLET) {
                map[py][px] = EMPTY;
                score += 50;
                dotCount--;
                for (auto& ghost : ghosts) {
                    ghost.state = VULNERABLE;
                    ghost.vulnerableTimer = 7000;
                }
            }
        }

        // Fruit Spawn
        if (fruitTimer > 0) {
            fruitTimer -= 16;
            if (fruitTimer <= 0) fruitActive = true;
        }
        if (fruitActive && abs(pacman.x - 13.5f * TILE_SIZE) < TILE_SIZE && 
            abs(pacman.y - 17 * TILE_SIZE) < TILE_SIZE) {
            score += 100;
            fruitActive = false;
            fruitTimer = 10000; // Reset for potential reuse
        }

        // Ghost Movement
        for (auto& ghost : ghosts) {
            if (ghost.vulnerableTimer > 0) {
                ghost.vulnerableTimer -= 16;
                if (ghost.vulnerableTimer <= 0) ghost.state = NORMAL;
            }
            float dx = pacman.x - ghost.x;
            float dy = pacman.y - ghost.y;
            float speed = ghost.state == VULNERABLE ? -SPEED * 0.8f : SPEED;
            ghost.vx = (dx > 0 ? speed : -speed) * (abs(dx) > abs(dy) ? 1 : 0);
            ghost.vy = (dy > 0 ? speed : -speed) * (abs(dx) <= abs(dy) ? 1 : 0);
            newX = ghost.x + ghost.vx;
            newY = ghost.y + ghost.vy;
            int gx = int(std::round(newX / TILE_SIZE));
            int gy = int(std::round(newY / TILE_SIZE));
            if (!isWall(gx, gy)) {
                ghost.x = newX;
                ghost.y = newY;
            }
            if (ghost.x < 0) ghost.x = (MAP_WIDTH - 1) * TILE_SIZE;
            if (ghost.x >= MAP_WIDTH * TILE_SIZE) ghost.x = 0;

            // Collision with Pac-Man
            if (abs(pacman.x - ghost.x) < TILE_SIZE && abs(pacman.y - ghost.y) < TILE_SIZE) {
                if (ghost.state == VULNERABLE) {
                    score += 200;
                    ghost.x = 13.5f * TILE_SIZE;
                    ghost.y = 14 * TILE_SIZE;
                    ghost.state = NORMAL;
                    ghost.vulnerableTimer = 0;
                } else {
                    lives--;
                    pacman.x = 13.5f * TILE_SIZE;
                    pacman.y = 23 * TILE_SIZE;
                    for (size_t i = 0; i < ghosts.size(); i++) {
                        ghosts[i].x = 13.5f * TILE_SIZE;
                        ghosts[i].y = 11 * TILE_SIZE + i * TILE_SIZE;
                        ghosts[i].vx = ghosts[i].vy = 0;
                    }
                    if (lives <= 0) running = false;
                    return; // Exit after one collision per frame
                }
            }
        }

        if (dotCount == 0) running = false; // Level complete
    }

    void render() {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Render Map
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                SDL_Rect dest = {x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE};
                switch (map[y][x]) {
                    case WALL: SDL_RenderCopy(renderer, wallTexture, NULL, &dest); break;
                    case DOT: {
                        SDL_Rect dotRect = {x * TILE_SIZE + 6, y * TILE_SIZE + 6, 4, 4};
                        SDL_RenderCopy(renderer, dotTexture, NULL, &dotRect);
                        break;
                    }
                    case PELLET: {
                        SDL_Rect pelletRect = {x * TILE_SIZE + 4, y * TILE_SIZE + 4, 8, 8};
                        SDL_RenderCopy(renderer, pelletTexture, NULL, &pelletRect);
                        break;
                    }
                }
            }
        }

        // Render Pac-Man
        SDL_Rect pacmanRect = {int(std::round(pacman.x)), int(std::round(pacman.y)), TILE_SIZE, TILE_SIZE};
        SDL_RenderCopy(renderer, pacmanTexture, NULL, &pacmanRect);

        // Render Ghosts
        for (auto& ghost : ghosts) {
            SDL_Rect ghostRect = {int(std::round(ghost.x)), int(std::round(ghost.y)), TILE_SIZE, TILE_SIZE};
            SDL_RenderCopy(renderer, ghost.state == VULNERABLE ? vulnerableTexture : ghost.texture, NULL, &ghostRect);
        }

        // Render Fruit
        if (fruitActive) {
            SDL_Rect cherryRect = {int(13.5f * TILE_SIZE), 17 * TILE_SIZE, TILE_SIZE, TILE_SIZE};
            SDL_RenderCopy(renderer, cherryTexture, NULL, &cherryRect);
        }

        // Render UI
        SDL_Color color = {255, 255, 255, 255};
        std::string text = "Score: " + std::to_string(score) + " Lives: " + std::to_string(lives);
        if (dotCount == 0) text += " - Level Complete!";
        else if (lives <= 0) text += " - Game Over!";
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
        if (pacmanTexture) SDL_DestroyTexture(pacmanTexture);
        if (blinkyTexture) SDL_DestroyTexture(blinkyTexture);
        if (pinkyTexture) SDL_DestroyTexture(pinkyTexture);
        if (inkyTexture) SDL_DestroyTexture(inkyTexture);
        if (clydeTexture) SDL_DestroyTexture(clydeTexture);
        if (vulnerableTexture) SDL_DestroyTexture(vulnerableTexture);
        if (dotTexture) SDL_DestroyTexture(dotTexture);
        if (pelletTexture) SDL_DestroyTexture(pelletTexture);
        if (cherryTexture) SDL_DestroyTexture(cherryTexture);
        if (wallTexture) SDL_DestroyTexture(wallTexture);
        if (font) TTF_CloseFont(font);
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
        ghosts.clear();
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
