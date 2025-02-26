#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define SCREEN_WIDTH 320   // C64 resolution
#define SCREEN_HEIGHT 200
#define PLAYER_WIDTH 16
#define PLAYER_HEIGHT 24
#define ROBOT_WIDTH 16
#define ROBOT_HEIGHT 24
#define OBJECT_WIDTH 16
#define OBJECT_HEIGHT 16
#define LIFT_WIDTH 32
#define LIFT_HEIGHT 8
#define GRAVITY 0.3f
#define JUMP_FORCE -6.0f
#define MOVE_SPEED 2.0f
#define TIME_LIMIT (6 * 60 * 60) // 6 hours in-game (seconds)

typedef struct {
    float x, y;
    float dx, dy;
    int width, height;
    bool isJumping;
    bool somersaulting;
    int lives;
} Player;

typedef struct {
    float x, y;
    int width, height;
    bool active;
    bool movesLeft; // Direction of patrol
    int zapTimer;   // Frames until zap
} Robot;

typedef struct {
    int x, y;
    int width, height;
    bool searchable;
    bool searched;
    int item; // 0: none, 1: lift reset, 2: password
} Object;

typedef struct {
    int x, y;
    int width, height;
    bool movingUp;
} Lift;

int main(int argc, char* argv[]) {
    // Initialize SDL2
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }

    if (IMG_Init(IMG_INIT_PNG) == 0) {
        printf("IMG initialization failed: %s\n", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("Mixer initialization failed: %s\n", Mix_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Create window and renderer
    SDL_Window* window = SDL_CreateWindow("Impossible Mission Clone",
                                        SDL_WINDOWPOS_CENTERED,
                                        SDL_WINDOWPOS_CENTERED,
                                        SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2,
                                        SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        Mix_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        Mix_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);

    // Load textures (placeholders)
    SDL_Texture* playerTex = IMG_LoadTexture(renderer, "agent.png");      // Player sprite
    SDL_Texture* robotTex = IMG_LoadTexture(renderer, "robot.png");      // Robot sprite
    SDL_Texture* objectTex = IMG_LoadTexture(renderer, "object.png");    // Furniture sprite
    SDL_Texture* liftTex = IMG_LoadTexture(renderer, "lift.png");        // Lift sprite
    SDL_Texture* bgTex = IMG_LoadTexture(renderer, "room_bg.png");       // Room background

    // Load sounds (placeholders)
    Mix_Chunk* jumpSound = Mix_LoadWAV("jump.wav");
    Mix_Chunk* zapSound = Mix_LoadWAV("zap.wav");
    Mix_Chunk* searchSound = Mix_LoadWAV("search.wav");
    Mix_Chunk* deathSound = Mix_LoadWAV("death.wav");
    Mix_Chunk* welcomeSound = Mix_LoadWAV("welcome.wav"); // "Stay a while..."
    Mix_Music* bgMusic = Mix_LoadMUS("bg_music.mp3");

    // Game state
    Player player = {SCREEN_WIDTH / 2 - PLAYER_WIDTH / 2, SCREEN_HEIGHT - PLAYER_HEIGHT - 8, 0, 0, PLAYER_WIDTH, PLAYER_HEIGHT, false, false, 6}; // 6 lives
    Robot robots[2] = {
        {100, SCREEN_HEIGHT - ROBOT_HEIGHT - 8, ROBOT_WIDTH, ROBOT_HEIGHT, true, true, 0},
        {200, SCREEN_HEIGHT - ROBOT_HEIGHT - 8, ROBOT_WIDTH, ROBOT_HEIGHT, true, false, 0}
    };
    Object objects[2] = {
        {150, SCREEN_HEIGHT - OBJECT_HEIGHT - 8, OBJECT_WIDTH, OBJECT_HEIGHT, true, false, 1}, // Lift reset
        {250, SCREEN_HEIGHT - OBJECT_HEIGHT - 8, OBJECT_WIDTH, OBJECT_HEIGHT, true, false, 2}  // Password
    };
    Lift lifts[2] = {
        {50, SCREEN_HEIGHT - LIFT_HEIGHT - 40, LIFT_WIDTH, LIFT_HEIGHT, false},
        {SCREEN_WIDTH - LIFT_WIDTH - 50, SCREEN_HEIGHT - LIFT_HEIGHT - 40, LIFT_WIDTH, LIFT_HEIGHT, false}
    };
    int score = 0;
    int timeRemaining = TIME_LIMIT;
    int collectedPasswords = 0;
    bool running = true;
    SDL_Event event;

    Mix_PlayChunk(Mix_LoadWAV("welcome.wav"), 0); // Play "Stay a while..." at start
    Mix_PlayMusic(bgMusic, -1);

    while (running) {
        // Event handling
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_RIGHT: player.dx = MOVE_SPEED; player.somersaulting = true; break;
                    case SDLK_LEFT: player.dx = -MOVE_SPEED; player.somersaulting = true; break;
                    case SDLK_UP:
                        if (!player.isJumping) {
                            player.dy = JUMP_FORCE;
                            player.isJumping = true;
                            player.somersaulting = true;
                            Mix_PlayChannel(-1, jumpSound, 0);
                        }
                        break;
                    case SDLK_DOWN: // Search object
                        for (int i = 0; i < 2; i++) {
                            if (objects[i].searchable && !objects[i].searched) {
                                SDL_Rect playerRect = {(int)player.x, (int)player.y, player.width, player.height};
                                SDL_Rect objRect = {objects[i].x, objects[i].y, objects[i].width, objects[i].height};
                                if (SDL_HasIntersection(&playerRect, &objRect)) {
                                    objects[i].searched = true;
                                    Mix_PlayChannel(-1, searchSound, 0);
                                    if (objects[i].item == 1) {
                                        for (int j = 0; j < 2; j++) lifts[j].y = SCREEN_HEIGHT - LIFT_HEIGHT - 40; // Reset lifts
                                    } else if (objects[i].item == 2) {
                                        collectedPasswords++;
                                        score += 100; // Arbitrary points
                                        if (collectedPasswords >= 36) running = false; // Win condition
                                    }
                                }
                            }
                        }
                        break;
                }
            }
            if (event.type == SDL_KEYUP) {
                if (event.key.keysym.sym == SDLK_RIGHT || event.key.keysym.sym == SDLK_LEFT) {
                    player.dx = 0;
                    player.somersaulting = false;
                }
            }
        }

        // Update time
        timeRemaining--;
        if (timeRemaining <= 0) {
            running = false; // Game over
        }

        // Update player
        player.x += player.dx;
        player.y += player.dy;
        player.dy += GRAVITY;

        // Lift collision and movement
        bool onLift = false;
        for (int i = 0; i < 2; i++) {
            SDL_Rect playerRect = {(int)player.x, (int)player.y, player.width, player.height};
            SDL_Rect liftRect = {lifts[i].x, lifts[i].y, lifts[i].width, lifts[i].height};
            if (SDL_HasIntersection(&playerRect, &liftRect) && player.dy > 0) {
                player.y = lifts[i].y - player.height;
                player.dy = 0;
                player.isJumping = false;
                onLift = true;
            }
            // Lift movement (simple oscillation)
            lifts[i].y += lifts[i].movingUp ? -1 : 1;
            if (lifts[i].y < 50) lifts[i].movingUp = false;
            if (lifts[i].y > SCREEN_HEIGHT - LIFT_HEIGHT - 40) lifts[i].movingUp = true;
        }

        // Platform collision (ground only in this demo)
        if (!onLift && player.y + player.height > SCREEN_HEIGHT - 8) {
            player.y = SCREEN_HEIGHT - player.height - 8;
            player.dy = 0;
            player.isJumping = false;
            player.somersaulting = false;
        }

        // Boundary checks
        if (player.x < 0) player.x = 0;
        if (player.x + player.width > SCREEN_WIDTH) player.x = SCREEN_WIDTH - player.width;

        // Update robots
        for (int i = 0; i < 2; i++) {
            if (robots[i].active) {
                robots[i].x += robots[i].movesLeft ? -1 : 1;
                if (robots[i].x < 50 || robots[i].x + robots[i].width > SCREEN_WIDTH - 50) robots[i].movesLeft = !robots[i].movesLeft;

                // Zap behavior
                robots[i].zapTimer++;
                if (robots[i].zapTimer >= 60 && fabs(robots[i].x - player.x) < 100) { // Zap every ~1 sec if close
                    SDL_Rect playerRect = {(int)player.x, (int)player.y, player.width, player.height};
                    SDL_Rect robotRect = {(int)robots[i].x, (int)robots[i].y, robots[i].width, robots[i].height};
                    if (SDL_HasIntersection(&playerRect, &robotRect) || (player.y == robots[i].y && fabs(player.x - robots[i].x) < 50)) {
                        player.lives--;
                        Mix_PlayChannel(-1, deathSound, 0);
                        player.x = SCREEN_WIDTH / 2 - PLAYER_WIDTH / 2;
                        player.y = SCREEN_HEIGHT - PLAYER_HEIGHT - 8;
                        if (player.lives <= 0) running = false;
                    }
                    Mix_PlayChannel(-1, zapSound, 0);
                    robots[i].zapTimer = 0;
                }
            }
        }

        // Rendering
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, bgTex, NULL, NULL); // Background

        // Draw objects
        for (int i = 0; i < 2; i++) {
            if (objects[i].searchable && !objects[i].searched) {
                SDL_Rect objRect = {objects[i].x, objects[i].y, objects[i].width, objects[i].height};
                SDL_RenderCopy(renderer, objectTex, NULL, &objRect);
            }
        }

        // Draw lifts
        for (int i = 0; i < 2; i++) {
            SDL_Rect liftRect = {lifts[i].x, lifts[i].y, lifts[i].width, lifts[i].height};
            SDL_RenderCopy(renderer, liftTex, NULL, &liftRect);
        }

        // Draw robots
        for (int i = 0; i < 2; i++) {
            if (robots[i].active) {
                SDL_Rect robotRect = {(int)robots[i].x, (int)robots[i].y, robots[i].width, robots[i].height};
                SDL_RenderCopy(renderer, robotTex, NULL, &robotRect);
            }
        }

        // Draw player
        SDL_Rect playerRect = {(int)player.x, (int)player.y, player.width, player.height};
        SDL_RenderCopy(renderer, playerTex, NULL, &playerRect);

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60 FPS, C64 ran at 50 Hz but adjusted for modern feel
    }

    // Cleanup
    Mix_FreeChunk(jumpSound);
    Mix_FreeChunk(zapSound);
    Mix_FreeChunk(searchSound);
    Mix_FreeChunk(deathSound);
    Mix_FreeChunk(welcomeSound);
    Mix_FreeMusic(bgMusic);
    SDL_DestroyTexture(playerTex);
    SDL_DestroyTexture(robotTex);
    SDL_DestroyTexture(objectTex);
    SDL_DestroyTexture(liftTex);
    SDL_DestroyTexture(bgTex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_Quit();
    IMG_Quit();
    SDL_Quit();

    if (collectedPasswords >= 36) {
        printf("Mission Complete! Final Score: %d\n", score);
    } else {
        printf("Game Over! Final Score: %d, Passwords: %d\n", score, collectedPasswords);
    }
    return 0;
}
