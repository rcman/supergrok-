#include <SDL2/SDL.h>
#include <stdio.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define PLAYER_WIDTH 32
#define PLAYER_HEIGHT 48
#define ELEVATOR_WIDTH 64
#define ELEVATOR_HEIGHT 16
#define GRAVITY 0.5f
#define JUMP_FORCE -10.0f
#define MOVE_SPEED 4.0f

typedef struct {
    float x, y;
    float dy; // Vertical velocity
    int onGround;
} Player;

typedef struct {
    float x, y;
    float dy; // Elevator movement speed
    int direction; // 1 = up, -1 = down
    float topY, bottomY; // Movement bounds
} Elevator;

int main(int argc, char* argv[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Elevator Action Clone", SDL_WINDOWPOS_UNDEFINED, 
                                          SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Game objects
    Player player = {SCREEN_WIDTH / 2 - PLAYER_WIDTH / 2, SCREEN_HEIGHT - PLAYER_HEIGHT, 0, 0};
    Elevator elevator = {SCREEN_WIDTH / 2 - ELEVATOR_WIDTH / 2, SCREEN_HEIGHT / 2, 1.0f, 1, SCREEN_HEIGHT / 4, SCREEN_HEIGHT * 3 / 4};

    int running = 1;
    SDL_Event event;

    // Game loop
    while (running) {
        // Handle input
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = 0;
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_SPACE && player.onGround) {
                    player.dy = JUMP_FORCE; // Jump
                    player.onGround = 0;
                }
            }
        }

        const Uint8* keystate = SDL_GetKeyboardState(NULL);
        if (keystate[SDL_SCANCODE_LEFT]) player.x -= MOVE_SPEED;
        if (keystate[SDL_SCANCODE_RIGHT]) player.x += MOVE_SPEED;

        // Keep player in bounds
        if (player.x < 0) player.x = 0;
        if (player.x + PLAYER_WIDTH > SCREEN_WIDTH) player.x = SCREEN_WIDTH - PLAYER_WIDTH;

        // Apply gravity
        player.dy += GRAVITY;
        player.y += player.dy;

        // Update elevator
        elevator.y += elevator.dy * elevator.direction;
        if (elevator.y <= elevator.topY) elevator.direction = 1;
        if (elevator.y + ELEVATOR_HEIGHT >= elevator.bottomY) elevator.direction = -1;

        // Collision with ground
        if (player.y + PLAYER_HEIGHT >= SCREEN_HEIGHT) {
            player.y = SCREEN_HEIGHT - PLAYER_HEIGHT;
            player.dy = 0;
            player.onGround = 1;
        }

        // Collision with elevator
        SDL_Rect playerRect = {(int)player.x, (int)player.y, PLAYER_WIDTH, PLAYER_HEIGHT};
        SDL_Rect elevRect = {(int)elevator.x, (int)elevator.y, ELEVATOR_WIDTH, ELEVATOR_HEIGHT};
        if (SDL_HasIntersection(&playerRect, &elevRect) && player.dy > 0 && 
            player.y + PLAYER_HEIGHT - elevator.y <= player.dy) {
            player.y = elevator.y - PLAYER_HEIGHT;
            player.dy = elevator.dy; // Match elevator speed
            player.onGround = 1;
        } else if (player.onGround && !SDL_HasIntersection(&playerRect, &elevRect)) {
            player.onGround = 0; // Fall off if not on elevator
        }

        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Draw player (red)
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &playerRect);

        // Draw elevator (blue)
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        SDL_RenderFillRect(renderer, &elevRect);

        // Update screen
        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60 FPS
    }

    // Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​