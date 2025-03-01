
#include <SDL2/SDL.h>
#include <math.h>
#include <stdio.h>

#define WINDOW_WIDTH 800   // Screen width
#define WINDOW_HEIGHT 600  // Screen height
#define WORLD_WIDTH 2000   // Total world width (larger than screen)
#define WORLD_HEIGHT 2000  // Total world height
#define PLAYER_SPEED 5.0f  // Player movement speed

// Player structure
typedef struct {
    float x, y;    // Position in world coordinates
    float angle;   // Facing direction in degrees
} Player;

// Camera structure to track the view
typedef struct {
    float x, y;    // Top-left corner of the viewport
} Camera;

int main(int argc, char* argv[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Textured Background Game",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          WINDOW_WIDTH, WINDOW_HEIGHT,
                                          SDL_WINDOW_SHOWN);
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Load the background image
    SDL_Surface* bgSurface = SDL_LoadBMP("background.bmp");
    if (!bgSurface) {
        fprintf(stderr, "SDL_LoadBMP Error: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_Texture* bgTexture = SDL_CreateTextureFromSurface(renderer, bgSurface);
    SDL_FreeSurface(bgSurface); // Free the surface after creating the texture
    if (!bgTexture) {
        fprintf(stderr, "SDL_CreateTextureFromSurface Error: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Initialize player and camera
    Player player = {WORLD_WIDTH / 2.0f, WORLD_HEIGHT / 2.0f, 0.0f};
    Camera camera = {0, 0};
    int running = 1;
    SDL_Event event;

    while (running) {
        // Handle input
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = 0;
        }

        const Uint8* keys = SDL_GetKeyboardState(NULL);
        float moveX = 0, moveY = 0;
        if (keys[SDL_SCANCODE_UP]) {
            moveX += cos(player.angle * M_PI / 180.0f);
            moveY += sin(player.angle * M_PI / 180.0f);
        }
        if (keys[SDL_SCANCODE_DOWN]) {
            moveX -= cos(player.angle * M_PI / 180.0f);
            moveY -= sin(player.angle * M_PI / 180.0f);
        }
        if (keys[SDL_SCANCODE_LEFT]) player.angle -= 5.0f;
        if (keys[SDL_SCANCODE_RIGHT]) player.angle += 5.0f;

        // Normalize movement speed
        float magnitude = sqrt(moveX * moveX + moveY * moveY);
        if (magnitude > 0) {
            moveX = (moveX / magnitude) * PLAYER_SPEED;
            moveY = (moveY / magnitude) * PLAYER_SPEED;
        }
        player.x += moveX;
        player.y += moveY;

        // Keep player within world bounds
        if (player.x < 0) player.x = 0;
        if (player.x > WORLD_WIDTH) player.x = WORLD_WIDTH;
        if (player.y < 0) player.y = 0;
        if (player.y > WORLD_HEIGHT) player.y = WORLD_HEIGHT;

        // Update camera to follow player (centered on screen)
        camera.x = player.x - WINDOW_WIDTH / 2.0f;
        camera.y = player.y - WINDOW_HEIGHT / 2.0f;
        if (camera.x < 0) camera.x = 0;
        if (camera.x > WORLD_WIDTH - WINDOW_WIDTH) camera.x = WORLD_WIDTH - WINDOW_WIDTH;
        if (camera.y < 0) camera.y = 0;
        if (camera.y > WORLD_HEIGHT - WINDOW_HEIGHT) camera.y = WORLD_HEIGHT - WINDOW_HEIGHT;

        // Render
        SDL_RenderClear(renderer);

        // Render the background with scrolling
        SDL_Rect bgRect = {(int)-camera.x, (int)-camera.y, WORLD_WIDTH, WORLD_HEIGHT};
        SDL_RenderCopy(renderer, bgTexture, NULL, &bgRect);

        // Draw player (triangle) relative to camera
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_Point playerPoints[4] = {
            {(int)(player.x - camera.x + 20 * cos(player.angle * M_PI / 180.0f)), (int)(player.y - camera.y + 20 * sin(player.angle * M_PI / 180.0f))},
            {(int)(player.x - camera.x + 10 * cos((player.angle + 135) * M_PI / 180.0f)), (int)(player.y - camera.y + 10 * sin((player.angle + 135) * M_PI / 180.0f))},
            {(int)(player.x - camera.x + 10 * cos((player.angle - 135) * M_PI / 180.0f)), (int)(player.y - camera.y + 10 * sin((player.angle - 135) * M_PI / 180.0f))},
            {(int)(player.x - camera.x + 20 * cos(player.angle * M_PI / 180.0f)), (int)(player.y - camera.y + 20 * sin(player.angle * M_PI / 180.0f))}
        };
        SDL_RenderDrawLines(renderer, playerPoints, 4);

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60 FPS
    }

    // Cleanup
    SDL_DestroyTexture(bgTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
