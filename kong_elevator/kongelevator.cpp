#include <SDL2/SDL.h>
#include <vector>

// Window dimensions
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int LEVEL_HEIGHT = 2000; // Total height of the game level

// Player class
class Player {
public:
    float x, y;
    float vx, vy;
    bool onGround;
    SDL_Rect* onElevator; // Pointer to the elevator the player is on (using rect for simplicity)

    Player() : x(0), y(500), vx(0), vy(0), onGround(false), onElevator(nullptr) {}
};

// Platform class
class Platform {
public:
    SDL_Rect rect;

    Platform(int x, int y, int w, int h) {
        rect.x = x;
        rect.y = y;
        rect.w = w;
        rect.h = h;
    }
};

// Elevator class
class Elevator {
public:
    SDL_Rect rect;
    float speed;
    int minY, maxY;
    bool movingUp;

    Elevator(int x, int y, int w, int h, float s, int min, int max)
        : speed(s), minY(min), maxY(max), movingUp(false) {
        rect.x = x;
        rect.y = y;
        rect.w = w;
        rect.h = h;
    }

    void update() {
        if (movingUp) {
            rect.y -= speed;
            if (rect.y <= minY) {
                rect.y = minY;
                movingUp = false;
            }
        } else {
            rect.y += speed;
            if (rect.y >= maxY) {
                rect.y = maxY;
                movingUp = true;
            }
        }
    }
};

// Enemy class
class Enemy {
public:
    float x, y;
    float vx;
    int minX, maxX;

    Enemy(float startX, float startY, float speed, int min, int max)
        : x(startX), y(startY), vx(speed), minX(min), maxX(max) {}

    void update() {
        x += vx;
        if (x <= minX || x >= maxX) {
            vx = -vx;
        }
    }
};

// Collision detection function
bool isColliding(const SDL_Rect& a, const SDL_Rect& b) {
    return a.x < b.x + b.w && a.x + a.w > b.x &&
           a.y < b.y + b.h && a.y + a.h > b.y;
}

int main(int argc, char* argv[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    // Create window
    SDL_Window* window = SDL_CreateWindow("Donkey Kong: Elevator Mission",
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
    Player player;
    std::vector<Platform> platforms;
    std::vector<Elevator> elevators;
    std::vector<Enemy> enemies;
    SDL_Rect goal = { 700, 50, 50, 50 };
    float cameraY = 0;

    // Initialize platforms
    platforms.push_back(Platform(0, 550, 800, 50));    // Ground
    platforms.push_back(Platform(200, 400, 200, 20));  // Middle platform
    platforms.push_back(Platform(500, 300, 200, 20));  // Upper platform

    // Initialize elevators
    elevators.push_back(Elevator(100, 500, 100, 20, 2, 200, 500));
    elevators.push_back(Elevator(600, 400, 100, 20, 2, 100, 400));

    // Initialize enemies
    enemies.push_back(Enemy(200, 380, 2, 200, 400));  // On middle platform
    enemies.push_back(Enemy(500, 280, 2, 500, 700));  // On upper platform

    // Game loop
    bool running = true;
    while (running) {
        // Event handling
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_LEFT:
                        player.vx = -5;
                        break;
                    case SDLK_RIGHT:
                        player.vx = 5;
                        break;
                    case SDLK_SPACE:
                        if (player.onGround || player.onElevator) {
                            player.vy = -10;
                            player.onGround = false;
                            player.onElevator = nullptr;
                        }
                        break;
                }
            } else if (event.type == SDL_KEYUP) {
                switch (event.key.keysym.sym) {
                    case SDLK_LEFT:
                    case SDLK_RIGHT:
                        player.vx = 0;
                        break;
                }
            }
        }

        // Update game state
        player.x += player.vx;
        player.y += player.vy;
        player.vy += 0.5; // Gravity

        // Ground check
        if (player.y >= 500) {
            player.y = 500;
            player.vy = 0;
            player.onGround = true;
            player.onElevator = nullptr;
        }

        SDL_Rect playerRect = { static_cast<int>(player.x), static_cast<int>(player.y), 50, 50 };

        // Platform collisions
        player.onGround = false; // Reset onGround before checking collisions
        for (const auto& platform : platforms) {
            if (isColliding(playerRect, platform.rect) && player.vy > 0) {
                player.y = platform.rect.y - 50;
                player.vy = 0;
                player.onGround = true;
            }
        }

        // Elevator updates and collisions
        for (auto& elevator : elevators) {
            elevator.update();
            SDL_Rect elevatorRect = elevator.rect;
            if (isColliding(playerRect, elevatorRect) && player.vy > 0 && player.y + 50 <= elevatorRect.y + 10) {
                player.y = elevatorRect.y - 50;
                player.vy = 0;
                player.onGround = true;
                player.onElevator = &elevator.rect;
            }
        }

        // Ride elevator
        if (player.onElevator) {
            player.y = player.onElevator->y - 50;
        }

        // Enemy updates and collisions
        for (auto& enemy : enemies) {
            enemy.update();
            SDL_Rect enemyRect = { static_cast<int>(enemy.x), static_cast<int>(enemy.y), 50, 50 };
            if (isColliding(playerRect, enemyRect)) {
                player.x = 0;
                player.y = 500;
                player.vx = 0;
                player.vy = 0;
                player.onElevator = nullptr;
            }
        }

        // Goal check
        if (isColliding(playerRect, goal)) {
            running = false; // Win condition
        }

        // Update camera
        cameraY = player.y - WINDOW_HEIGHT / 2;
        if (cameraY < 0) cameraY = 0;
        if (cameraY > LEVEL_HEIGHT - WINDOW_HEIGHT) cameraY = LEVEL_HEIGHT - WINDOW_HEIGHT;

        // Render game
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black background
        SDL_RenderClear(renderer);

        // Render platforms (green)
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        for (const auto& platform : platforms) {
            SDL_Rect renderRect = { platform.rect.x, platform.rect.y - static_cast<int>(cameraY), platform.rect.w, platform.rect.h };
            SDL_RenderFillRect(renderer, &renderRect);
        }

        // Render elevators (blue)
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        for (const auto& elevator : elevators) {
            SDL_Rect renderRect = { elevator.rect.x, elevator.rect.y - static_cast<int>(cameraY), elevator.rect.w, elevator.rect.h };
            SDL_RenderFillRect(renderer, &renderRect);
        }

        // Render enemies (yellow)
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        for (const auto& enemy : enemies) {
            SDL_Rect enemyRect = { static_cast<int>(enemy.x), static_cast<int>(enemy.y - cameraY), 50, 50 };
            SDL_RenderFillRect(renderer, &enemyRect);
        }

        // Render player (red)
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_Rect renderPlayerRect = { static_cast<int>(player.x), static_cast<int>(player.y - cameraY), 50, 50 };
        SDL_RenderFillRect(renderer, &renderPlayerRect);

        // Render goal (cyan)
        SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);
        SDL_Rect renderGoal = { goal.x, goal.y - static_cast<int>(cameraY), goal.w, goal.h };
        SDL_RenderFillRect(renderer, &renderGoal);

        SDL_RenderPresent(renderer);
    }

    // Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
