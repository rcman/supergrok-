#include <SDL2/SDL.h>
#include <vector>
#include <cmath>

// Constants for window and game dimensions
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int GROUND_Y = 500;          // Ground level for jeep
const int PLAYER_SIZE = 50;
const int ENEMY_SIZE = 50;
const float GRAVITY = 0.5f;        // Gravity for jeep jump

// Player class
class Player {
public:
    float x, y;         // Position
    float vx, vy;       // Velocity
    bool isHelicopter;  // True for helicopter, false for jeep
    int health;         // Player health

    Player(bool heli) : x(0), y(300), vx(0), vy(0), isHelicopter(heli), health(3) {
        if (!heli) y = GROUND_Y; // Jeep starts on ground
    }
};

// Enemy class
class Enemy {
public:
    float x, y;  // Position
    float vx;    // Velocity
    int health;  // Enemy health

    Enemy(float startX, float startY, float speed) : x(startX), y(startY), vx(speed), health(1) {}
};

// Projectile class
class Projectile {
public:
    float x, y;    // Position
    float vx, vy;  // Velocity
    bool active;   // Flag to mark if projectile is active

    Projectile(float startX, float startY, float speedX, float speedY)
        : x(startX), y(startY), vx(speedX), vy(speedY), active(true) {}
};

// Background class for parallax scrolling
class Background {
public:
    float x;          // Position
    float scrollSpeed; // Speed of scrolling

    Background(float speed) : x(0), scrollSpeed(speed) {}
};

// Collision detection function (Axis-Aligned Bounding Box)
bool isColliding(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh) {
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

int main(int argc, char* argv[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    // Create window
    SDL_Window* window = SDL_CreateWindow("Silkworm",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          WINDOW_WIDTH, WINDOW_HEIGHT,
                                          SDL_WINDOW_SHOWN);
    if (!window) {
        SDL_Log("Window creation failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Create renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        SDL_Log("Renderer creation failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Game objects
    Player player1(true);  // Helicopter
    Player player2(false); // Jeep/Tank
    std::vector<Enemy> enemies;
    std::vector<Projectile> projectiles;
    Background bgFar(1.0f);  // Far background layer (slower)
    Background bgNear(2.0f); // Near background layer (faster)

    // Delta time initialization
    Uint32 lastTime = SDL_GetTicks();

    // Game loop
    bool running = true;
    while (running) {
        // Calculate delta time
        Uint32 currentTime = SDL_GetTicks();
        float delta = (currentTime - lastTime) / 1000.0f; // Delta time in seconds
        lastTime = currentTime;

        // Event handling
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    // Player 1 (Helicopter) controls
                    case SDLK_UP:
                        player1.vy = -5;
                        break;
                    case SDLK_DOWN:
                        player1.vy = 5;
                        break;
                    case SDLK_LEFT:
                        player1.vx = -5;
                        break;
                    case SDLK_RIGHT:
                        player1.vx = 5;
                        break;
                    case SDLK_SPACE:
                        projectiles.push_back(Projectile(player1.x + PLAYER_SIZE, player1.y + PLAYER_SIZE / 2, 10, 0)); // Shoot forward
                        break;
                    case SDLK_LCTRL:
                        projectiles.push_back(Projectile(player1.x + PLAYER_SIZE / 2, player1.y + PLAYER_SIZE, 0, 10)); // Shoot downward
                        break;
                    // Player 2 (Jeep/Tank) controls
                    case SDLK_a:
                        player2.vx = -5;
                        break;
                    case SDLK_d:
                        player2.vx = 5;
                        break;
                    case SDLK_w:
                        if (player2.y >= GROUND_Y) { // Jump only if on ground
                            player2.vy = -10;
                        }
                        break;
                    case SDLK_s:
                        projectiles.push_back(Projectile(player2.x + PLAYER_SIZE, player2.y + PLAYER_SIZE / 2, 10, 0)); // Shoot forward
                        break;
                }
            } else if (event.type == SDL_KEYUP) {
                switch (event.key.keysym.sym) {
                    // Stop movement
                    case SDLK_UP:
                    case SDLK_DOWN:
                        player1.vy = 0;
                        break;
                    case SDLK_LEFT:
                    case SDLK_RIGHT:
                        player1.vx = 0;
                        break;
                    case SDLK_a:
                    case SDLK_d:
                        player2.vx = 0;
                        break;
                }
            }
        }

        // Update game state with delta time
        // Player 1 (Helicopter)
        player1.x += player1.vx * delta * 60; // Normalize to 60 FPS
        player1.y += player1.vy * delta * 60;
        if (player1.x < 0) player1.x = 0;
        if (player1.x > WINDOW_WIDTH - PLAYER_SIZE) player1.x = WINDOW_WIDTH - PLAYER_SIZE;
        if (player1.y < 0) player1.y = 0;
        if (player1.y > WINDOW_HEIGHT - PLAYER_SIZE) player1.y = WINDOW_HEIGHT - PLAYER_SIZE;

        // Player 2 (Jeep/Tank) with gravity
        player2.x += player2.vx * delta * 60;
        player2.y += player2.vy * delta * 60;
        player2.vy += GRAVITY * delta * 60; // Apply gravity
        if (player2.y >= GROUND_Y) {
            player2.y = GROUND_Y;
            player2.vy = 0;
        }
        if (player2.x < 0) player2.x = 0;
        if (player2.x > WINDOW_WIDTH - PLAYER_SIZE) player2.x = WINDOW_WIDTH - PLAYER_SIZE;

        // Spawn enemies randomly
        if (rand() % 100 < 2) {
            enemies.push_back(Enemy(WINDOW_WIDTH, rand() % WINDOW_HEIGHT, -2));
        }

        // Update enemies
        for (auto& enemy : enemies) {
            enemy.x += enemy.vx * delta * 60;
        }

        // Update projectiles
        for (auto& proj : projectiles) {
            proj.x += proj.vx * delta * 60;
            proj.y += proj.vy * delta * 60;
        }

        // Collision detection
        for (auto& proj : projectiles) {
            for (auto& enemy : enemies) {
                if (proj.active && isColliding(proj.x, proj.y, 10, 10, enemy.x, enemy.y, ENEMY_SIZE, ENEMY_SIZE)) {
                    enemy.health--;
                    proj.active = false; // Deactivate projectile
                }
            }
        }

        // Remove dead enemies and inactive/off-screen projectiles
        enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
                                     [](const Enemy& e) { return e.health <= 0 || e.x < -ENEMY_SIZE; }), enemies.end());
        projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(),
                                         [](const Projectile& p) { return !p.active || p.x < 0 || p.x > WINDOW_WIDTH || p.y > WINDOW_HEIGHT; }), projectiles.end());

        // Basic health management (example: lose health on enemy collision)
        for (const auto& enemy : enemies) {
            if (isColliding(player1.x, player1.y, PLAYER_SIZE, PLAYER_SIZE, enemy.x, enemy.y, ENEMY_SIZE, ENEMY_SIZE)) {
                player1.health--;
            }
            if (isColliding(player2.x, player2.y, PLAYER_SIZE, PLAYER_SIZE, enemy.x, enemy.y, ENEMY_SIZE, ENEMY_SIZE)) {
                player2.health--;
            }
        }

        // Check game over condition
        if (player1.health <= 0 || player2.health <= 0) {
            running = false; // End game if either player dies
        }

        // Update background scrolling
        bgFar.x -= bgFar.scrollSpeed * delta * 60;
        bgNear.x -= bgNear.scrollSpeed * delta * 60;
        if (bgFar.x <= -WINDOW_WIDTH) bgFar.x += WINDOW_WIDTH;
        if (bgNear.x <= -WINDOW_WIDTH) bgNear.x += WINDOW_WIDTH;

        // Render game
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black background
        SDL_RenderClear(renderer);

        // Render background layers
        SDL_SetRenderDrawColor(renderer, 0, 100, 0, 255); // Dark green far layer
        SDL_Rect bgFarRect = { static_cast<int>(bgFar.x), 0, WINDOW_WIDTH, WINDOW_HEIGHT };
        SDL_RenderFillRect(renderer, &bgFarRect);
        SDL_Rect bgFarRect2 = { static_cast<int>(bgFar.x + WINDOW_WIDTH), 0, WINDOW_WIDTH, WINDOW_HEIGHT };
        SDL_RenderFillRect(renderer, &bgFarRect2);

        SDL_SetRenderDrawColor(renderer, 0, 200, 0, 255); // Light green near layer
        SDL_Rect bgNearRect = { static_cast<int>(bgNear.x), 0, WINDOW_WIDTH, WINDOW_HEIGHT };
        SDL_RenderFillRect(renderer, &bgNearRect);
        SDL_Rect bgNearRect2 = { static_cast<int>(bgNear.x + WINDOW_WIDTH), 0, WINDOW_WIDTH, WINDOW_HEIGHT };
        SDL_RenderFillRect(renderer, &bgNearRect2);

        // Render players
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red helicopter
        SDL_Rect player1Rect = { static_cast<int>(player1.x), static_cast<int>(player1.y), PLAYER_SIZE, PLAYER_SIZE };
        SDL_RenderFillRect(renderer, &player1Rect);

        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // Blue jeep
        SDL_Rect player2Rect = { static_cast<int>(player2.x), static_cast<int>(player2.y), PLAYER_SIZE, PLAYER_SIZE };
        SDL_RenderFillRect(renderer, &player2Rect);

        // Render enemies
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // Yellow enemies
        for (const auto& enemy : enemies) {
            SDL_Rect enemyRect = { static_cast<int>(enemy.x), static_cast<int>(enemy.y), ENEMY_SIZE, ENEMY_SIZE };
            SDL_RenderFillRect(renderer, &enemyRect);
        }

        // Render projectiles
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White projectiles
        for (const auto& proj : projectiles) {
            if (proj.active) {
                SDL_Rect projRect = { static_cast<int>(proj.x), static_cast<int>(proj.y), 10, 10 };
                SDL_RenderFillRect(renderer, &projRect);
            }
        }

        SDL_RenderPresent(renderer);
    }

    // Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
