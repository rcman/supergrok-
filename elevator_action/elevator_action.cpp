#include <SDL2/SDL.h>
#include <vector>
#include <cstdlib>
#include <ctime>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int FLOOR_HEIGHT = 150;
const int PLAYER_SIZE = 30;
const int ELEVATOR_WIDTH = 50;
const int ELEVATOR_HEIGHT = 60;
const int ENEMY_SIZE = 30;
const int BULLET_SIZE = 5;

struct Player { int x, y; int floor; bool jumping; int jumpVel; };
struct Elevator { int x, y; int targetY; bool moving; };
struct Enemy { int x, y; int floor; bool alive; };
struct Bullet { int x, y; int velX; bool active; };

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) { printf("SDL Init failed: %s\n", SDL_GetError()); return 1; }
    SDL_Window* window = SDL_CreateWindow("Elevator Action", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if (!window) { printf("Window creation failed: %s\n", SDL_GetError()); SDL_Quit(); return 1; }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) { printf("Renderer creation failed: %s\n", SDL_GetError()); SDL_DestroyWindow(window); SDL_Quit(); return 1; }

    Player player = {100, SCREEN_HEIGHT - FLOOR_HEIGHT - PLAYER_SIZE, 0, false, 0};
    std::vector<Elevator> elevators = {{50, SCREEN_HEIGHT - FLOOR_HEIGHT - ELEVATOR_HEIGHT, SCREEN_HEIGHT - FLOOR_HEIGHT - ELEVATOR_HEIGHT, false},
                                       {SCREEN_WIDTH - 100, SCREEN_HEIGHT - FLOOR_HEIGHT - ELEVATOR_HEIGHT, SCREEN_HEIGHT - FLOOR_HEIGHT - ELEVATOR_HEIGHT, false}};
    std::vector<Enemy> enemies;
    srand(time(0));
    for (int i = 0; i < 3; i++) {
        int floor = rand() % 3;
        enemies.push_back({rand() % (SCREEN_WIDTH - ENEMY_SIZE), SCREEN_HEIGHT - (floor + 1) * FLOOR_HEIGHT - ENEMY_SIZE, floor, true});
    }
    std::vector<Bullet> bullets;
    bool running = true, elevatorTriggered = false;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_LEFT: player.x -= 10; break;
                    case SDLK_RIGHT: player.x += 10; break;
                    case SDLK_SPACE: 
                        if (!player.jumping) { player.jumping = true; player.jumpVel = -15; }
                        break;
                    case SDLK_z: bullets.push_back({player.x + PLAYER_SIZE, player.y, 10, true}); break;
                    case SDLK_e:
                        if (!elevatorTriggered) {
                            for (auto& elevator : elevators) {
                                if (abs(player.x - elevator.x) < 50 && abs(player.y - elevator.y) < 50) {
                                    elevator.moving = true;
                                    elevator.targetY = SCREEN_HEIGHT - ((rand() % 3) + 1) * FLOOR_HEIGHT - ELEVATOR_HEIGHT;
                                    elevatorTriggered = true;
                                }
                            }
                        }
                        break;
                }
            }
            if (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_e) elevatorTriggered = false;
        }

        if (player.jumping) {
            player.y += player.jumpVel;
            player.jumpVel += 1;
            int targetY = SCREEN_HEIGHT - ((SCREEN_HEIGHT - player.y - PLAYER_SIZE) / FLOOR_HEIGHT + 1) * FLOOR_HEIGHT - PLAYER_SIZE;
            if (player.y >= targetY) {
                player.y = targetY;
                player.jumping = false;
                player.floor = (SCREEN_HEIGHT - player.y - PLAYER_SIZE) / FLOOR_HEIGHT;
            }
        }
        if (player.x < 0) player.x = 0;
        if (player.x > SCREEN_WIDTH - PLAYER_SIZE) player.x = SCREEN_WIDTH - PLAYER_SIZE;

        for (auto& elevator : elevators) {
            if (elevator.moving) {
                if (elevator.y < elevator.targetY) elevator.y += 5;
                else if (elevator.y > elevator.targetY) elevator.y -= 5;
                else elevator.moving = false;
                if (player.x + PLAYER_SIZE > elevator.x && player.x < elevator.x + ELEVATOR_WIDTH &&
                    player.y + PLAYER_SIZE >= elevator.y && player.y + PLAYER_SIZE <= elevator.y + ELEVATOR_HEIGHT) {
                    player.y = elevator.y - PLAYER_SIZE;
                    player.floor = (SCREEN_HEIGHT - elevator.y - ELEVATOR_HEIGHT) / FLOOR_HEIGHT;
                }
            }
        }

        for (auto& enemy : enemies) {
            if (enemy.alive) {
                enemy.x += (rand() % 2 == 0 ? 2 : -2);
                if (enemy.x < 0) enemy.x = 0;
                if (enemy.x > SCREEN_WIDTH - ENEMY_SIZE) enemy.x = SCREEN_WIDTH - ENEMY_SIZE;
                enemy.y = SCREEN_HEIGHT - (enemy.floor + 1) * FLOOR_HEIGHT - ENEMY_SIZE;
            }
        }

        for (auto& bullet : bullets) {
            if (bullet.active) {
                bullet.x += bullet.velX;
                if (bullet.x > SCREEN_WIDTH) bullet.active = false;
                for (auto& enemy : enemies) {
                    if (enemy.alive && bullet.x + BULLET_SIZE > enemy.x && bullet.x < enemy.x + ENEMY_SIZE &&
                        bullet.y + BULLET_SIZE > enemy.y && bullet.y < enemy.y + ENEMY_SIZE) {
                        enemy.alive = false;
                        bullet.active = false;
                    }
                }
            }
        }
        bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [](const Bullet& b) { return !b.active; }), bullets.end());

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        for (int i = 0; i < 3; i++) {
            SDL_Rect floor = {0, SCREEN_HEIGHT - (i + 1) * FLOOR_HEIGHT, SCREEN_WIDTH, 20};
            SDL_RenderFillRect(renderer, &floor);
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        SDL_Rect playerRect = {player.x, player.y, PLAYER_SIZE, PLAYER_SIZE};
        SDL_RenderFillRect(renderer, &playerRect);
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        for (const auto& elevator : elevators) {
            SDL_Rect elevatorRect = {elevator.x, elevator.y, ELEVATOR_WIDTH, ELEVATOR_HEIGHT};
            SDL_RenderFillRect(renderer, &elevatorRect);
        }
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        for (const auto& enemy : enemies) {
            if (enemy.alive) {
                SDL_Rect enemyRect = {enemy.x, enemy.y, ENEMY_SIZE, ENEMY_SIZE};
                SDL_RenderFillRect(renderer, &enemyRect);
            }
        }
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        for (const auto& bullet : bullets) {
            if (bullet.active) {
                SDL_Rect bulletRect = {bullet.x, bullet.y, BULLET_SIZE, BULLET_SIZE};
                SDL_RenderFillRect(renderer, &bulletRect);
            }
        }
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
