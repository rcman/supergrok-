#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>
#include <stdlib.h> // For rand()

#define SCREEN_WIDTH 1920  // Updated to full HD
#define SCREEN_HEIGHT 1080 // Updated to full HD
#define PLAYER_SPEED 5
#define BULLET_SPEED 10
#define BG_SPEED 2
#define ENEMY_SPEED 3
#define SPAWN_INTERVAL 2000 // 2 seconds in milliseconds
#define MAX_ENEMIES 10

// Player structure
typedef struct {
    int x, y;
    SDL_Texture* texture;
    bool isHelicopter;
} Player;

// Enemy structure
typedef struct {
    int x, y;
    SDL_Texture* texture;
    bool active;
} Enemy;

// Bullet structure
typedef struct {
    int x, y;
    bool active;
} Bullet;

// Load texture helper function
SDL_Texture* loadTexture(SDL_Renderer* renderer, const char* file) {
    SDL_Surface* surface = IMG_Load(file);
    if (!surface) {
        printf("Failed to load %s! SDL_image Error: %s\n", file, IMG_GetError());
        return NULL;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

int main(int argc, char* argv[]) {
    // Initialize SDL and SDL_image
    if (SDL_Init(SDL_INIT_VIDEO) < 0 || IMG_Init(IMG_INIT_PNG) == 0) {
        printf("SDL could not initialize! Error: %s\n", SDL_GetError());
        return 1;
    }

    // Create window and renderer
    SDL_Window* window = SDL_CreateWindow("Silkworm Clone", SDL_WINDOWPOS_UNDEFINED, 
                                          SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, 
                                          SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!window || !renderer) {
        printf("Window/Renderer failed! Error: %s\n", SDL_GetError());
        return 1;
    }

    // Load textures
    SDL_Texture* bgTexture = loadTexture(renderer, "background.png");
    SDL_Texture* heliTexture = loadTexture(renderer, "player_heli.png");
    SDL_Texture* jeepTexture = loadTexture(renderer, "player_jeep.png");
    SDL_Texture* enemyTexture = loadTexture(renderer, "enemy.png");
    if (!bgTexture || !heliTexture || !jeepTexture || !enemyTexture) {
        return 1; // Exit if any texture fails to load
    }

    // Game objects
    Player heli = {100, 540, heliTexture, true}; // Centered vertically
    Player jeep = {100, SCREEN_HEIGHT - 64, jeepTexture, false};
    Enemy enemies[MAX_ENEMIES] = {0};
    int enemyCount = 0;
    Bullet bullets[50] = {0};
    int bulletCount = 0;
    int bgX = 0; // Background scroll position
    Uint32 lastSpawn = 0; // Enemy spawn timer
    bool running = true;
    SDL_Event e;

    // Game loop
    while (running) {
        // Handle events
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) running = false;
        }

        // Input handling
        const Uint8* keys = SDL_GetKeyboardState(NULL);
        // Helicopter (WASD)
        if (keys[SDL_SCANCODE_W] && heli.y > 0) heli.y -= PLAYER_SPEED;
        if (keys[SDL_SCANCODE_S] && heli.y < SCREEN_HEIGHT - 64) heli.y += PLAYER_SPEED;
        if (keys[SDL_SCANCODE_A] && heli.x > 0) heli.x -= PLAYER_SPEED;
        if (keys[SDL_SCANCODE_D] && heli.x < SCREEN_WIDTH - 64) heli.x += PLAYER_SPEED;
        // Jeep (Arrow keys)
        if (keys[SDL_SCANCODE_UP] && jeep.y > SCREEN_HEIGHT / 2) jeep.y -= PLAYER_SPEED;
        if (keys[SDL_SCANCODE_DOWN] && jeep.y < SCREEN_HEIGHT - 64) jeep.y += PLAYER_SPEED;
        if (keys[SDL_SCANCODE_LEFT] && jeep.x > 0) jeep.x -= PLAYER_SPEED;
        if (keys[SDL_SCANCODE_RIGHT] && jeep.x < SCREEN_WIDTH - 64) jeep.x += PLAYER_SPEED;

        // Shooting
        if (keys[SDL_SCANCODE_SPACE] && bulletCount < 50) {
            bullets[bulletCount] = (Bullet){heli.x + 64, heli.y + 32, true};
            bulletCount++;
        }
        if (keys[SDL_SCANCODE_RETURN] && bulletCount < 50) {
            bullets[bulletCount] = (Bullet){jeep.x + 64, jeep.y + 32, true};
            bulletCount++;
        }

        // Update bullets
        for (int i = 0; i < bulletCount; i++) {
            if (bullets[i].active) {
                bullets[i].x += BULLET_SPEED;
                if (bullets[i].x > SCREEN_WIDTH) bullets[i].active = false;
            }
        }

        // Enemy spawning
        Uint32 now = SDL_GetTicks();
        if (now - lastSpawn >= SPAWN_INTERVAL && enemyCount < MAX_ENEMIES) {
            for (int i = 0; i < MAX_ENEMIES; i++) {
                if (!enemies[i].active) {
                    enemies[i] = (Enemy){SCREEN_WIDTH, rand() % (SCREEN_HEIGHT - 64), enemyTexture, true};
                    enemyCount++;
                    lastSpawn = now;
                    break;
                }
            }
        }

        // Update enemies
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (enemies[i].active) {
                enemies[i].x -= ENEMY_SPEED;
                if (enemies[i].x < -64) {
                    enemies[i].active = false;
                    enemyCount--;
                }
            }
        }

        // Scroll background
        bgX -= BG_SPEED;
        if (bgX <= -SCREEN_WIDTH) bgX = 0; // Loop at 1920

        // Render
        SDL_RenderClear(renderer);

        // Draw background (two copies for seamless scrolling)
        SDL_Rect bgRect1 = {bgX, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_Rect bgRect2 = {bgX + SCREEN_WIDTH, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_RenderCopy(renderer, bgTexture, NULL, &bgRect1);
        SDL_RenderCopy(renderer, bgTexture, NULL, &bgRect2);

        // Draw players
        SDL_Rect heliRect = {heli.x, heli.y, 64, 64};
        SDL_Rect jeepRect = {jeep.x, jeep.y, 64, 64};
        SDL_RenderCopy(renderer, heli.texture, NULL, &heliRect);
        SDL_RenderCopy(renderer, jeep.texture, NULL, &jeepRect);

        // Draw enemies
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (enemies[i].active) {
                SDL_Rect enemyRect = {enemies[i].x, enemies[i].y, 64, 64};
                SDL_RenderCopy(renderer, enemyTexture, NULL, &enemyRect);
            }
        }

        // Draw bullets
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        for (int i = 0; i < bulletCount; i++) {
            if (bullets[i].active) {
                SDL_Rect bulletRect = {bullets[i].x, bullets[i].y, 10, 5};
                SDL_RenderFillRect(renderer, &bulletRect);
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60 FPS
    }

    // Cleanup
    SDL_DestroyTexture(bgTexture);
    SDL_DestroyTexture(heliTexture);
    SDL_DestroyTexture(jeepTexture);
    SDL_DestroyTexture(enemyTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
