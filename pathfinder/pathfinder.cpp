#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define TILE_SIZE 60
#define PLAYER_SPEED 5
#define ZOMBIE_SPEED 2
#define NUM_KEYS 3
#define NUM_ZOMBIES 5
#define MAX_MAZES 3

typedef struct { int x, y; bool active; } Entity;
typedef struct { int x, y; bool collected; } Item;
typedef struct { int x, y; int g, h, f; int parentX, parentY; } Node;

int currentMaze = 0;
int keysCollected = 0;

// Multiple mazes (1 = wall, 0 = path, 2 = room)
int mazes[MAX_MAZES][SCREEN_HEIGHT / TILE_SIZE][SCREEN_WIDTH / TILE_SIZE] = {
    { // Maze 1
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,0,0,0,1,0,0,0,0,1,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,2,0,0,0,0,1},
        {1,0,1,0,1,0,1,1,0,1,0,1,1,1,1,0,1,0,1,1,1,1,0,1,1,1,0,1,1,1,0,1},
        {1,0,1,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,0,0,0,1,0,0,0,1,0,0,0,0,0,1},
        {1,0,1,1,1,1,1,1,1,1,1,1,1,0,1,0,1,1,1,1,0,1,1,1,0,1,2,1,1,1,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
    },
    { // Maze 2 (more complex)
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,0,0,1,0,0,0,1,0,0,0,0,0,1,0,0,0,1,0,0,0,0,0,1,0,0,2,0,0,0,0,1},
        {1,0,1,1,1,0,1,1,0,1,1,1,0,1,0,1,1,1,0,1,1,1,0,1,0,1,0,1,1,1,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,1,1,1,1,0,1,1,1,1,1,1,0,1,1,1,0,1,1,1,1,1,0,1,1,1,2,1,1,1,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
    },
    { // Maze 3 (harder)
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,0,0,0,0,1,0,0,0,1,0,0,0,0,0,1,0,0,0,1,0,0,0,0,0,1,2,0,0,0,0,1},
        {1,0,1,1,0,1,0,1,1,1,0,1,1,1,0,1,0,1,1,1,0,1,1,1,0,1,0,1,1,1,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,1,1,1,0,1,1,1,1,1,0,1,1,1,0,1,0,1,1,1,1,1,0,1,1,1,2,1,1,1,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
    }
};

Entity player = {TILE_SIZE, TILE_SIZE, true};
Item keys[NUM_KEYS];
Entity zombies[NUM_ZOMBIES];
Entity distraction = {-1, -1, false};
SDL_Texture *playerTex, *zombieTex, *keyTex, *itemTex;

bool initSDL(SDL_Window** window, SDL_Renderer** renderer) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0 || IMG_Init(IMG_INIT_PNG) == 0) return false;
    *window = SDL_CreateWindow("Maze Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!*window) return false;
    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);
    if (!*renderer) return false;
    playerTex = IMG_LoadTexture(*renderer, "player.png");
    zombieTex = IMG_LoadTexture(*renderer, "zombie.png");
    keyTex = IMG_LoadTexture(*renderer, "key.png");
    itemTex = IMG_LoadTexture(*renderer, "item.png");
    if (!playerTex || !zombieTex || !keyTex || !itemTex) {
        printf("Texture loading failed: %s\n", SDL_GetError());
        return false;
    }
    return true;
}

void spawnEntities() {
    srand(time(NULL));
    int roomCount = 0;
    int roomX[NUM_KEYS], roomY[NUM_KEYS];
    for (int y = 0; y < SCREEN_HEIGHT / TILE_SIZE && roomCount < NUM_KEYS; y++) {
        for (int x = 0; x < SCREEN_WIDTH / TILE_SIZE; x++) {
            if (mazes[currentMaze][y][x] == 2) {
                roomX[roomCount] = x * TILE_SIZE;
                roomY[roomCount] = y * TILE_SIZE;
                roomCount++;
            }
        }
    }
    for (int i = 0; i < NUM_KEYS; i++) {
        keys[i].x = roomX[i] + TILE_SIZE / 4;
        keys[i].y = roomY[i] + TILE_SIZE / 4;
        keys[i].collected = false;
    }
    for (int i = 0; i < NUM_ZOMBIES; i++) {
        do {
            zombies[i].x = (rand() % (SCREEN_WIDTH / TILE_SIZE - 2) + 1) * TILE_SIZE;
            zombies[i].y = (rand() % (SCREEN_HEIGHT / TILE_SIZE - 2) + 1) * TILE_SIZE;
        } while (mazes[currentMaze][zombies[i].y / TILE_SIZE][zombies[i].x / TILE_SIZE] == 1);
        zombies[i].active = true;
    }
}

bool checkCollision(int x, int y) {
    return mazes[currentMaze][y / TILE_SIZE][x / TILE_SIZE] == 1;
}

// Simple A* pathfinding
bool findPath(int startX, int startY, int goalX, int goalY, int* nextX, int* nextY) {
    Node open[SCREEN_WIDTH / TILE_SIZE * SCREEN_HEIGHT / TILE_SIZE];
    bool closed[SCREEN_HEIGHT / TILE_SIZE][SCREEN_WIDTH / TILE_SIZE] = {false};
    int openCount = 0;

    open[openCount++] = (Node){startX / TILE_SIZE, startY / TILE_SIZE, 0, abs(goalX / TILE_SIZE - startX / TILE_SIZE) + abs(goalY / TILE_SIZE - startY / TILE_SIZE), 0, -1, -1};
    while (openCount > 0) {
        int best = 0;
        for (int i = 1; i < openCount; i++) if (open[i].f < open[best].f) best = i;
        Node current = open[best];
        if (current.x == goalX / TILE_SIZE && current.y == goalY / TILE_SIZE) {
            while (current.parentX != -1) {
                int px = current.parentX * TILE_SIZE, py = current.parentY * TILE_SIZE;
                if (current.parentX == startX / TILE_SIZE && current.parentY == startY / TILE_SIZE) {
                    *nextX = px; *nextY = py;
                    return true;
                }
                for (int i = 0; i < openCount; i++) if (open[i].x == current.parentX && open[i].y == current.parentY) current = open[i];
            }
        }
        closed[current.y][current.x] = true;
        open[best] = open[--openCount];

        int dx[] = {0, 1, 0, -1}, dy[] = {-1, 0, 1, 0};
        for (int i = 0; i < 4; i++) {
            int nx = current.x + dx[i], ny = current.y + dy[i];
            if (nx < 0 || nx >= SCREEN_WIDTH / TILE_SIZE || ny < 0 || ny >= SCREEN_HEIGHT / TILE_SIZE || closed[ny][nx] || mazes[currentMaze][ny][nx] == 1) continue;
            int g = current.g + 1, h = abs(nx - goalX / TILE_SIZE) + abs(ny - goalY / TILE_SIZE);
            bool found = false;
            for (int j = 0; j < openCount; j++) {
                if (open[j].x == nx && open[j].y == ny) {
                    if (g < open[j].g) open[j] = (Node){nx, ny, g, h, g + h, current.x, current.y};
                    found = true;
                    break;
                }
            }
            if (!found) open[openCount++] = (Node){nx, ny, g, h, g + h, current.x, current.y};
        }
    }
    return false;
}

void updateZombies() {
    for (int i = 0; i < NUM_ZOMBIES; i++) {
        if (!zombies[i].active) continue;
        int distToPlayer = abs(zombies[i].x - player.x) + abs(zombies[i].y - player.y);
        int targetX = distraction.active ? distraction.x : player.x;
        int targetY = distraction.active ? distraction.y : player.y;
        if (distToPlayer < 5 * TILE_SIZE || distraction.active) {
            int nextX, nextY;
            if (findPath(zombies[i].x, zombies[i].y, targetX, targetY, &nextX, &nextY)) {
                if (zombies[i].x < nextX) zombies[i].x += ZOMBIE_SPEED;
                if (zombies[i].x > nextX) zombies[i].x -= ZOMBIE_SPEED;
                if (zombies[i].y < nextY) zombies[i].y += ZOMBIE_SPEED;
                if (zombies[i].y > nextY) zombies[i].y -= ZOMBIE_SPEED;
            }
        }
    }
}

void render(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Draw maze
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int y = 0; y < SCREEN_HEIGHT / TILE_SIZE; y++) {
        for (int x = 0; x < SCREEN_WIDTH / TILE_SIZE; x++) {
            if (mazes[currentMaze][y][x] == 1 || mazes[currentMaze][y][x] == 2) {
                SDL_Rect wall = {x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE};
                SDL_RenderFillRect(renderer, &wall);
            }
        }
    }

    // Draw keys
    for (int i = 0; i < NUM_KEYS; i++) {
        if (!keys[i].collected) {
            SDL_Rect keyRect = {keys[i].x, keys[i].y, TILE_SIZE / 2, TILE_SIZE / 2};
            SDL_RenderCopy(renderer, keyTex, NULL, &keyRect);
        }
    }

    // Draw player
    SDL_Rect playerRect = {player.x, player.y, TILE_SIZE, TILE_SIZE};
    SDL_RenderCopy(renderer, playerTex, NULL, &playerRect);

    // Draw zombies
    for (int i = 0; i < NUM_ZOMBIES; i++) {
        if (zombies[i].active) {
            SDL_Rect zombieRect = {zombies[i].x, zombies[i].y, TILE_SIZE, TILE_SIZE};
            SDL_RenderCopy(renderer, zombieTex, NULL, &zombieRect);
        }
    }

    // Draw distraction
    if (distraction.active) {
        SDL_Rect distRect = {distraction.x, distraction.y, TILE_SIZE / 2, TILE_SIZE / 2};
        SDL_RenderCopy(renderer, itemTex, NULL, &distRect);
    }

    SDL_RenderPresent(renderer);
}

int main() {
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;

    if (!initSDL(&window, &renderer)) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }

    spawnEntities();
    bool quit = false;
    SDL_Event e;

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
                    case SDLK_SPACE:
                        distraction.x = player.x + TILE_SIZE * 2;
                        distraction.y = player.y;
                        distraction.active = true;
                        break;
                }
                if (!checkCollision(newX, newY) && newX >= 0 && newX < SCREEN_WIDTH && newY >= 0 && newY < SCREEN_HEIGHT) {
                    player.x = newX;
                    player.y = newY;
                }
            }
        }

        // Check for key collection
        for (int i = 0; i < NUM_KEYS; i++) {
            if (!keys[i].collected && abs(player.x - keys[i].x) < TILE_SIZE && abs(player.y - keys[i].y) < TILE_SIZE) {
                keys[i].collected = true;
                keysCollected++;
            }
        }

        updateZombies();
        render(renderer);

        if (keysCollected == NUM_KEYS) {
            currentMaze++;
            if (currentMaze >= MAX_MAZES) {
                printf("You won the game!\n");
                quit = true;
            } else {
                printf("Level %d complete!\n", currentMaze);
                keysCollected = 0;
                player.x = player.y = TILE_SIZE;
                spawnEntities();
            }
        }

        SDL_Delay(16); // ~60 FPS
    }

    SDL_DestroyTexture(playerTex);
    SDL_DestroyTexture(zombieTex);
    SDL_DestroyTexture(keyTex);
    SDL_DestroyTexture(itemTex);
    SDL_Des
troyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
