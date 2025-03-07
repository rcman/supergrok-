// v2
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
#define MAZE_ROWS (SCREEN_HEIGHT / TILE_SIZE)  // 18
#define MAZE_COLS (SCREEN_WIDTH / TILE_SIZE)   // 32
#define MAX_PATH 100

typedef struct { int x, y; bool active; } Entity;
typedef struct { int x, y; bool collected; } Item;
typedef struct { int x, y; int g, h, f; int parentX, parentY; } Node;
typedef struct { int x, y; } Point;

int currentMaze = 0;
int keysCollected = 0;

// Maze layout (1 = wall, 0 = path, 2 = room)
int mazes[MAX_MAZES][MAZE_ROWS][MAZE_COLS] = {
    { // Maze 1
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,0,0,0,1,0,0,0,0,1,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,2,0,0,0,0,1},
        {1,0,1,0,1,0,1,1,0,1,0,1,1,1,1,0,1,0,1,1,1,1,0,1,1,1,0,1,1,1,0,1},
        {1,0,1,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,0,0,0,1,0,0,0,1,0,0,0,0,0,1},
        {1,0,1,1,1,1,1,1,0,0,1,1,1,0,1,0,1,1,1,1,0,1,1,1,0,1,2,1,1,1,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,1,1,1,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,0,1,1,1,1,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,1,1,1,1,1,0,0,0,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,1,1,1,1,1,0,0,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,1,1,1,1,0,0,1,1,0,0,1,1,1,1,1,1,0,1,1,1,0,1,1,1,1,1,1,1,1,1,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,1,1,1,1,1,0,0,1,1,1,1,0,1,0,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
    },
    { // Maze 2
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,0,0,1,0,0,0,1,0,0,0,0,0,1,0,0,0,1,0,0,0,0,0,1,0,0,2,0,0,0,0,1},
        {1,0,1,1,1,0,1,1,0,1,1,1,0,1,0,1,1,1,0,1,1,1,0,1,0,1,0,1,1,1,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,1,1,1,1,0,1,1,1,1,1,1,0,1,1,1,0,1,1,1,1,1,0,1,1,1,2,1,1,1,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
    },
    { // Maze 3
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,0,0,0,0,1,0,0,0,1,0,0,0,0,0,1,0,0,0,1,0,0,0,0,0,1,2,0,0,0,0,1},
        {1,0,1,1,0,1,0,1,1,1,0,1,1,1,0,1,0,1,1,1,0,1,1,1,0,1,0,1,1,1,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,1,1,1,0,1,1,1,1,1,0,1,1,1,0,1,0,1,1,1,1,1,0,1,1,1,2,1,1,1,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
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
    if (!*renderer) {
        SDL_DestroyWindow(*window);
        return false;
    }
    playerTex = IMG_LoadTexture(*renderer, "player.png");
    zombieTex = IMG_LoadTexture(*renderer, "zombie.png");
    keyTex = IMG_LoadTexture(*renderer, "key.png");
    itemTex = IMG_LoadTexture(*renderer, "item.png");
    if (!playerTex || !zombieTex || !keyTex || !itemTex) {
        if (playerTex) SDL_DestroyTexture(playerTex);
        if (zombieTex) SDL_DestroyTexture(zombieTex);
        if (keyTex) SDL_DestroyTexture(keyTex);
        if (itemTex) SDL_DestroyTexture(itemTex);
        SDL_DestroyRenderer(*renderer);
        SDL_DestroyWindow(*window);
        printf("Texture loading failed: %s\n", SDL_GetError());
        return false;
    }
    return true;
}

void spawnEntities() {
    srand(time(NULL));
    int roomCount = 0;
    int roomX[NUM_KEYS], roomY[NUM_KEYS];
    for (int y = 0; y < MAZE_ROWS && roomCount < NUM_KEYS; y++) {
        for (int x = 0; x < MAZE_COLS; x++) {
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
            zombies[i].x = (rand() % (MAZE_COLS - 2) + 1) * TILE_SIZE;
            zombies[i].y = (rand() % (MAZE_ROWS - 2) + 1) * TILE_SIZE;
        } while (mazes[currentMaze][zombies[i].y / TILE_SIZE][zombies[i].x / TILE_SIZE] == 1);
        zombies[i].active = true;
    }
}

bool checkCollision(int x, int y, int width, int height) {
    int corners[4][2] = {{x, y}, {x + width - 1, y}, {x, y + height - 1}, {x + width - 1, y + height - 1}};
    for (int i = 0; i < 4; i++) {
        if (mazes[currentMaze][corners[i][1] / TILE_SIZE][corners[i][0] / TILE_SIZE] == 1) return true;
    }
    return false;
}

bool findPath(int startX, int startY, int goalX, int goalY, Point path[], int* pathLen) {
    Node open[MAZE_COLS * MAZE_ROWS];
    bool closed[MAZE_ROWS][MAZE_COLS] = {false};
    int openCount = 0;
    Point parents[MAZE_ROWS][MAZE_COLS];
    for (int y = 0; y < MAZE_ROWS; y++) for (int x = 0; x < MAZE_COLS; x++) parents[y][x] = (Point){-1, -1};

    open[openCount++] = (Node){startX / TILE_SIZE, startY / TILE_SIZE, 0, abs(goalX / TILE_SIZE - startX / TILE_SIZE) + abs(goalY / TILE_SIZE - startY / TILE_SIZE), 0, -1, -1};
    while (openCount > 0) {
        int best = 0;
        for (int i = 1; i < openCount; i++) if (open[i].f < open[best].f) best = i;
        Node current = open[best];
        if (current.x == goalX / TILE_SIZE && current.y == goalY / TILE_SIZE) {
            *pathLen = 0;
            while (current.x != -1) {
                path[(*pathLen)++] = (Point){current.x * TILE_SIZE, current.y * TILE_SIZE};
                int px = current.parentX, py = current.parentY;
                current.x = px; current.y = py;
                if (px != -1) { current.parentX = parents[py][px].x; current.parentY = parents[py][px].y; }
            }
            return true;
        }
        closed[current.y][current.x] = true;
        open[best] = open[--openCount];

        int dx[] = {0, 1, 0, -1}, dy[] = {-1, 0, 1, 0};
        for (int i = 0; i < 4; i++) {
            int nx = current.x + dx[i], ny = current.y + dy[i];
            if (nx < 0 || nx >= MAZE_COLS || ny < 0 || ny >= MAZE_ROWS || closed[ny][nx] || mazes[currentMaze][ny][nx] == 1) continue;
            int g = current.g + 1, h = abs(nx - goalX / TILE_SIZE) + abs(ny - goalY / TILE_SIZE);
            bool found = false;
            for (int j = 0; j < openCount; j++) {
                if (open[j].x == nx && open[j].y == ny) {
                    if (g < open[j].g) {
                        open[j] = (Node){nx, ny, g, h, g + h, current.x, current.y};
                        parents[ny][nx] = (Point){current.x, current.y};
                    }
                    found = true;
                    break;
                }
            }
            if (!found) {
                open[openCount++] = (Node){nx, ny, g, h, g + h, current.x, current.y};
                parents[ny][nx] = (Point){current.x, current.y};
            }
        }
    }
    return false;
}

void updateZombies() {
    static Point paths[NUM_ZOMBIES][MAX_PATH];
    static int pathLens[NUM_ZOMBIES] = {0};
    static int pathSteps[NUM_ZOMBIES] = {0};

    for (int i = 0; i < NUM_ZOMBIES; i++) {
        if (!zombies[i].active) continue;
        int distToPlayer = abs(zombies[i].x - player.x) + abs(zombies[i].y - player.y);
        int targetX = distraction.active ? distraction.x : player.x;
        int targetY = distraction.active ? distraction.y : player.y;
        if ((distToPlayer < 5 * TILE_SIZE || distraction.active) && (pathLens[i] == 0 || rand() % 60 == 0)) {
            if (findPath(zombies[i].x, zombies[i].y, targetX, targetY, paths[i], &pathLens[i])) {
                pathSteps[i] = pathLens[i] - 1;
            }
        }
        if (pathLens[i] > 0 && pathSteps[i] >= 0) {
            int nextX = paths[i][pathSteps[i]].x, nextY = paths[i][pathSteps[i]].y;
            if (zombies[i].x < nextX) zombies[i].x += ZOMBIE_SPEED;
            if (zombies[i].x > nextX) zombies[i].x -= ZOMBIE_SPEED;
            if (zombies[i].y < nextY) zombies[i].y += ZOMBIE_SPEED;
            if (zombies[i].y > nextY) zombies[i].y -= ZOMBIE_SPEED;
            if (abs(zombies[i].x - nextX) < ZOMBIE_SPEED && abs(zombies[i].y - nextY) < ZOMBIE_SPEED) pathSteps[i]--;
            if (pathSteps[i] < 0) pathLens[i] = 0;
        }
    }
}

void render(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Draw maze
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int y = 0; y < MAZE_ROWS; y++) {
        for (int x = 0; x < MAZE_COLS; x++) {
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
    int distractionTimer = 0;

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
                        distractionTimer = 0;
                        break;
                }
                if (!checkCollision(newX, newY, TILE_SIZE, TILE_SIZE) && newX >= 0 && newX < SCREEN_WIDTH && newY >= 0 && newY < SCREEN_HEIGHT) {
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

        // Distraction timeout
        if (distraction.active) {
            distractionTimer++;
            if (distractionTimer > 180) { // ~3 seconds at 60 FPS
                distraction.active = false;
                distractionTimer = 0;
            }
        }

        render(renderer);

        if (keysCollected == NUM_KEYS) {
            currentMaze++;
            if (currentMaze >= MAX_MAZES) {
                printf("You won the game!\n");
                quit = true;
            } else {
                printf("Level %d complete!\n", currentMaze + 1);
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
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
