#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <vector>
#include <string>

// Constants
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int TILE_SIZE = 32;
const int LEVEL_WIDTH = 40;  // Wider level for scrolling
const int LEVEL_HEIGHT = 15;

// Game states
enum GameState { MENU, PLAYING, PAUSED };
GameState gameState = MENU;

// Level layout (0 = empty, 1 = wall, 2 = trap trigger)
int level[LEVEL_HEIGHT][LEVEL_WIDTH] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1}, // Trap trigger
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

// Player properties
struct Player {
    float x = 100.0f;
    float y = 100.0f;
    float velY = 0.0f;
    const float speed = 5.0f;
    const float gravity = 0.5f;
    const float jumpStrength = -10.0f;
    int animFrame = 0;
    int animDelay = 0;
    int health = 3;
    bool alive = true;
} player;

// Enemy structure
struct Enemy {
    float x;
    float y;
    float speed;
    int direction; // 1 = right, -1 = left
    int animFrame;
    int animDelay;
};

// Trap structure
struct Trap {
    float x;
    float y;
    bool active;
    int timer;
};

std::vector<Enemy> enemies;
std::vector<Trap> traps;

// Input states
bool keys[4] = {false}; // 0: left, 1: right, 2: up, 3: down
bool jumpPressed = false;

// Camera
float cameraX = 0.0f;

// Collision detection
bool collidesWithWall(float x, float y) {
    int leftTile = (int)(x / TILE_SIZE);
    int rightTile = (int)((x + TILE_SIZE - 1) / TILE_SIZE);
    int topTile = (int)(y / TILE_SIZE);
    int bottomTile = (int)((y + TILE_SIZE - 1) / TILE_SIZE);

    for (int ty = topTile; ty <= bottomTile; ty++) {
        for (int tx = leftTile; tx <= rightTile; tx++) {
            if (tx >= 0 && tx < LEVEL_WIDTH && ty >= 0 && ty < LEVEL_HEIGHT && level[ty][tx] == 1) {
                return true;
            }
        }
    }
    return false;
}

// Check if player is on the ground
bool isOnGround(float x, float y) {
    int leftTileX = (int)(x / TILE_SIZE);
    int rightTileX = (int)((x + TILE_SIZE - 1) / TILE_SIZE);
    int bottomTileY = (int)((y + TILE_SIZE) / TILE_SIZE);
    if (bottomTileY >= LEVEL_HEIGHT) return false;
    return level[bottomTileY][leftTileX] == 1 || level[bottomTileY][rightTileX] == 1;
}

int main(int argc, char* argv[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        return 1;
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        SDL_Log("Unable to initialize SDL_image: %s", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        SDL_Log("Unable to initialize SDL_mixer: %s", Mix_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Rick Dangerous",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        0
    );
    if (!window) {
        Mix_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        SDL_DestroyWindow(window);
        Mix_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Load assets
    SDL_Texture* tilesTexture = IMG_LoadTexture(renderer, "assets/tiles.png");
    SDL_Texture* playerTexture = IMG_LoadTexture(renderer, "assets/rick.png"); // 4-frame animation (32x32 each)
    SDL_Texture* enemyTexture = IMG_LoadTexture(renderer, "assets/enemy.png"); // 2-frame animation (32x32 each)
    SDL_Texture* trapTexture = IMG_LoadTexture(renderer, "assets/trap.png");
    SDL_Texture* menuTexture = IMG_LoadTexture(renderer, "assets/menu.png");
    Mix_Chunk* jumpSound = Mix_LoadWAV("assets/jump.wav");
    Mix_Chunk* trapSound = Mix_LoadWAV("assets/trap.wav");

    // Check asset loading
    if (!tilesTexture || !playerTexture || !enemyTexture || !trapTexture || !menuTexture || !jumpSound || !trapSound) {
        SDL_Log("Failed to load assets");
        Mix_FreeChunk(trapSound);
        Mix_FreeChunk(jumpSound);
        SDL_DestroyTexture(menuTexture);
        SDL_DestroyTexture(trapTexture);
        SDL_DestroyTexture(enemyTexture);
        SDL_DestroyTexture(playerTexture);
        SDL_DestroyTexture(tilesTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        Mix_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Initialize enemies and traps
    enemies.push_back({400.0f, 400.0f, 2.0f, 1, 0, 0}); // Enemy at (400, 400)
    for (int y = 0; y < LEVEL_HEIGHT; y++) {
        for (int x = 0; x < LEVEL_WIDTH; x++) {
            if (level[y][x] == 2) {
                traps.push_back({(float)(x * TILE_SIZE), (float)(y * TILE_SIZE), false, 0});
            }
        }
    }

    // Game loop
    bool running = true;
    Uint32 lastTime = SDL_GetTicks();
    while (running) {
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f; // Seconds
        lastTime = currentTime;

        // Handle events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_LEFT: keys[0] = true; break;
                    case SDLK_RIGHT: keys[1] = true; break;
                    case SDLK_UP: keys[2] = true; break;
                    case SDLK_DOWN: keys[3] = true; break;
                    case SDLK_SPACE: jumpPressed = true; break;
                    case SDLK_RETURN:
                        if (gameState == MENU) gameState = PLAYING;
                        break;
                    case SDLK_p:
                        if (gameState == PLAYING) gameState = PAUSED;
                        else if (gameState == PAUSED) gameState = PLAYING;
                        break;
                }
            } else if (event.type == SDL_KEYUP) {
                switch (event.key.keysym.sym) {
                    case SDLK_LEFT: keys[0] = false; break;
                    case SDLK_RIGHT: keys[1] = false; break;
                    case SDLK_UP: keys[2] = false; break;
                    case SDLK_DOWN: keys[3] = false; break;
                }
            }
        }

        if (gameState == PLAYING) {
            // Update player
            float dx = (keys[1] - keys[0]) * player.speed;
            float newX = player.x + dx;
            if (!collidesWithWall(newX, player.y)) {
                player.x = newX;
            } else {
                if (dx > 0) player.x = ((int)((player.x + TILE_SIZE - 1 + dx) / TILE_SIZE)) * TILE_SIZE - TILE_SIZE;
                else if (dx < 0) player.x = ((int)((player.x + dx) / TILE_SIZE) + 1) * TILE_SIZE;
            }

            player.velY += player.gravity;
            float newY = player.y + player.velY;
            if (!collidesWithWall(player.x, newY)) {
                player.y = newY;
            } else {
                if (player.velY > 0) {
                    player.y = ((int)((player.y + TILE_SIZE - 1 + player.velY) / TILE_SIZE)) * TILE_SIZE - TILE_SIZE;
                    player.velY = 0;
                } else if (player.velY < 0) {
                    player.y = ((int)((player.y + player.velY) / TILE_SIZE) + 1) * TILE_SIZE;
                    player.velY = 0;
                }
            }

            if (jumpPressed && isOnGround(player.x, player.y)) {
                player.velY = player.jumpStrength;
                Mix_PlayChannel(-1, jumpSound, 0);
            }
            jumpPressed = false;

            // Player animation
            if (dx != 0) {
                player.animDelay++;
                if (player.animDelay > 5) {
                    player.animFrame = (player.animFrame + 1) % 4; // 4 frames
                    player.animDelay = 0;
                }
            } else {
                player.animFrame = 0; // Idle frame
            }

            // Update camera
            cameraX = player.x - WINDOW_WIDTH / 2;
            if (cameraX < 0) cameraX = 0;
            if (cameraX > LEVEL_WIDTH * TILE_SIZE - WINDOW_WIDTH) cameraX = LEVEL_WIDTH * TILE_SIZE - WINDOW_WIDTH;

            // Update enemies
            for (auto& enemy : enemies) {
                float newX = enemy.x + enemy.speed * enemy.direction;
                if (!collidesWithWall(newX, enemy.y)) {
                    enemy.x = newX;
                } else {
                    enemy.direction *= -1; // Reverse direction
                }
                enemy.animDelay++;
                if (enemy.animDelay > 10) {
                    enemy.animFrame = (enemy.animFrame + 1) % 2; // 2 frames
                    enemy.animDelay = 0;
                }

                // Check collision with player
                if (player.alive && SDL_HasIntersection(&(SDL_Rect){(int)player.x, (int)player.y, TILE_SIZE, TILE_SIZE},
                                                        &(SDL_Rect){(int)enemy.x, (int)enemy.y, TILE_SIZE, TILE_SIZE})) {
                    player.health--;
                    if (player.health <= 0) player.alive = false;
                }
            }

            // Update traps
            for (auto& trap : traps) {
                if (!trap.active && SDL_HasIntersection(&(SDL_Rect){(int)player.x, (int)player.y, TILE_SIZE, TILE_SIZE},
                                                        &(SDL_Rect){(int)trap.x, (int)trap.y, TILE_SIZE, TILE_SIZE})) {
                    trap.active = true;
                    trap.timer = 60; // 1 second at 60 FPS
                    Mix_PlayChannel(-1, trapSound, 0);
                }
                if (trap.active) {
                    trap.timer--;
                    if (trap.timer <= 0) {
                        trap.active = false;
                        if (SDL_HasIntersection(&(SDL_Rect){(int)player.x, (int)player.y, TILE_SIZE, TILE_SIZE},
                                                &(SDL_Rect){(int)trap.x, (int)trap.y, TILE_SIZE, TILE_SIZE})) {
                            player.health--;
                            if (player.health <= 0) player.alive = false;
                        }
                    }
                }
            }
        }

        // Render
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        if (gameState == MENU) {
            SDL_RenderCopy(renderer, menuTexture, NULL, NULL);
        } else if (gameState == PLAYING || gameState == PAUSED) {
            // Render level
            int startTileX = (int)(cameraX / TILE_SIZE);
            int endTileX = startTileX + (WINDOW_WIDTH / TILE_SIZE) + 1;
            if (endTileX > LEVEL_WIDTH) endTileX = LEVEL_WIDTH;

            for (int y = 0; y < LEVEL_HEIGHT; y++) {
                for (int x = startTileX; x < endTileX; x++) {
                    int tileType = level[y][x];
                    if (tileType == 1) {
                        SDL_Rect src = {0, 0, TILE_SIZE, TILE_SIZE};
                        SDL_Rect dst = {x * TILE_SIZE - (int)cameraX, y * TILE_SIZE, TILE_SIZE, TILE_SIZE};
                        SDL_RenderCopy(renderer, tilesTexture, &src, &dst);
                    }
                }
            }

            // Render traps
            for (const auto& trap : traps) {
                SDL_Rect src = {trap.active ? TILE_SIZE : 0, 0, TILE_SIZE, TILE_SIZE};
                SDL_Rect dst = {(int)trap.x - (int)cameraX, (int)trap.y, TILE_SIZE, TILE_SIZE};
                SDL_RenderCopy(renderer, trapTexture, &src, &dst);
            }

            // Render enemies
            for (const auto& enemy : enemies) {
                SDL_Rect src = {enemy.animFrame * TILE_SIZE, 0, TILE_SIZE, TILE_SIZE};
                SDL_Rect dst = {(int)enemy.x - (int)cameraX, (int)enemy.y, TILE_SIZE, TILE_SIZE};
                SDL_RenderCopy(renderer, enemyTexture, &src, &dst);
            }

            // Render player
            if (player.alive) {
                SDL_Rect src = {player.animFrame * TILE_SIZE, 0, TILE_SIZE, TILE_SIZE};
                SDL_Rect dst = {(int)player.x - (int)cameraX, (int)player.y, TILE_SIZE, TILE_SIZE};
                SDL_RenderCopy(renderer, playerTexture, &src, &dst);
            }

            // Render HUD (health)
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            for (int i = 0; i < player.health; i++) {
                SDL_Rect heart = {10 + i * 40, 10, 32, 32};
                SDL_RenderFillRect(renderer, &heart);
            }

            if (gameState == PAUSED) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 128);
                SDL_Rect pauseOverlay = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
                SDL_RenderFillRect(renderer, &pauseOverlay);
            }
        }

        SDL_RenderPresent(renderer);

        // Cap frame rate to ~60 FPS
        SDL_Delay(16);
    }

    // Cleanup
    Mix_FreeChunk(trapSound);
    Mix_FreeChunk(jumpSound);
    SDL_DestroyTexture(menuTexture);
    SDL_DestroyTexture(trapTexture);
    SDL_DestroyTexture(enemyTexture);
    SDL_DestroyTexture(playerTexture);
    SDL_DestroyTexture(tilesTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_Quit();
    IMG_Quit();
    SDL_Quit();

    return 0;
}
