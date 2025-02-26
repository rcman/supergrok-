#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <stdbool.h>
#include <stdio.h>

#define SCREEN_WIDTH 256   // Arcade/NES resolution scaled
#define SCREEN_HEIGHT 224
#define PLAYER_WIDTH 16
#define PLAYER_HEIGHT 24
#define GHOST_WIDTH 16
#define GHOST_HEIGHT 16
#define TILE_SIZE 8
#define GRAVITY 0.2f
#define JUMP_FORCE -5.0f
#define MOVE_SPEED 1.0f
#define FALL_DAMAGE_HEIGHT 16 // Pixels for lethal fall

typedef struct {
    float x, y;
    float dx, dy;
    int width, height;
    bool isJumping;
    bool onRope;
    bool onLadder;
    int lives;
    float lastY; // For fall damage calculation
} Player;

typedef struct {
    float x, y;
    int width, height;
    bool active;
} Ghost;

typedef struct {
    int x, y;
    int width, height;
} Platform;

typedef struct {
    int x, y;
    int height;
} Rope;

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
    SDL_Window* window = SDL_CreateWindow("Spelunker Clone",
                                        SDL_WINDOWPOS_CENTERED,
                                        SDL_WINDOWPOS_CENTERED,
                                        SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2, // Scaled for modern displays
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

    // Load textures (placeholders - replace with actual assets)
    SDL_Texture* playerTex = IMG_LoadTexture(renderer, "spelunker.png");
    SDL_Texture* ghostTex = IMG_LoadTexture(renderer, "ghost.png");
    SDL_Texture* bgTex = IMG_LoadTexture(renderer, "cave_bg.png");
    SDL_Texture* platformTex = IMG_LoadTexture(renderer, "platform.png");
    SDL_Texture* ropeTex = IMG_LoadTexture(renderer, "rope.png");

    // Load sounds (placeholders - replace with actual assets)
    Mix_Chunk* jumpSound = Mix_LoadWAV("jump.wav");
    Mix_Chunk* deathSound = Mix_LoadWAV("death.wav");
    Mix_Chunk* ghostSound = Mix_LoadWAV("ghost.wav");
    Mix_Music* bgMusic = Mix_LoadMUS("cave_music.mp3");

    // Player setup
    Player player = {SCREEN_WIDTH / 2, SCREEN_HEIGHT - PLAYER_HEIGHT - TILE_SIZE, 0, 0, PLAYER_WIDTH, PLAYER_HEIGHT, false, false, false, 3, SCREEN_HEIGHT - PLAYER_HEIGHT - TILE_SIZE};

    // Ghost (demo: 1 ghost)
    Ghost ghost = {200, 50, GHOST_WIDTH, GHOST_HEIGHT, false};
    Uint32 ghostTimer = SDL_GetTicks();

    // Platforms (demo: ground and one platform)
    Platform platforms[2] = {
        {0, SCREEN_HEIGHT - TILE_SIZE, SCREEN_WIDTH, TILE_SIZE}, // Ground
        {100, SCREEN_HEIGHT - TILE_SIZE - 50, 100, TILE_SIZE}    // Elevated platform
    };

    // Rope (demo: one rope)
    Rope rope = {150, 50, 100};

    int score = 0;
    bool running = true;
    SDL_Event event;

    Mix_PlayMusic(bgMusic, -1); // Loop background music

    while (running) {
        // Event handling
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_RIGHT: player.dx = MOVE_SPEED; break;
                    case SDLK_LEFT: player.dx = -MOVE_SPEED; break;
                    case SDLK_UP:
                        if (!player.isJumping && !player.onRope && !player.onLadder) {
                            player.dy = JUMP_FORCE;
                            player.isJumping = true;
                            Mix_PlayChannel(-1, jumpSound, 0);
                        } else if (player.onRope || player.onLadder) {
                            player.dy = -MOVE_SPEED;
                        }
                        break;
                    case SDLK_DOWN:
                        if (player.onRope || player.onLadder) player.dy = MOVE_SPEED;
                        break;
                    case SDLK_SPACE:
                        if (player.onRope || player.onLadder) {
                            player.onRope = false;
                            player.onLadder = false;
                            player.dy = JUMP_FORCE;
                            player.isJumping = true;
                            Mix_PlayChannel(-1, jumpSound, 0);
                        }
                        break;
                }
            }
            if (event.type == SDL_KEYUP) {
                if (event.key.keysym.sym == SDLK_RIGHT || event.key.keysym.sym == SDLK_LEFT) player.dx = 0;
                if (event.key.keysym.sym == SDLK_UP || event.key.keysym.sym == SDLK_DOWN) player.dy = (player.onRope || player.onLadder) ? 0 : player.dy;
            }
        }

        // Update player
        player.x += player.dx;
        player.y += player.dy;
        if (!player.onRope && !player.onLadder) player.dy += GRAVITY;

        // Check rope collision
        SDL_Rect playerRect = {(int)player.x, (int)player.y, player.width, player.height};
        SDL_Rect ropeRect = {rope.x, rope.y, TILE_SIZE, rope.height};
        if (SDL_HasIntersection(&playerRect, &ropeRect) && !player.isJumping) {
            player.onRope = true;
            player.dx = 0;
            player.dy = 0;
            player.x = rope.x - (player.width - TILE_SIZE) / 2;
        } else if (!player.onRope && !player.onLadder) {
            player.onRope = false;
        }

        // Platform collision
        bool onGround = false;
        for (int i = 0; i < 2; i++) {
            SDL_Rect platRect = {platforms[i].x, platforms[i].y, platforms[i].width, platforms[i].height};
            if (SDL_HasIntersection(&playerRect, &platRect) && player.dy > 0) {
                player.y = platforms[i].y - player.height;
                player.dy = 0;
                player.isJumping = false;
                onGround = true;
            }
        }

        // Fall damage
        if (onGround && player.lastY - player.y > FALL_DAMAGE_HEIGHT) {
            player.lives--;
            Mix_PlayChannel(-1, deathSound, 0);
            player.x = SCREEN_WIDTH / 2;
            player.y = SCREEN_HEIGHT - PLAYER_HEIGHT - TILE_SIZE;
            if (player.lives <= 0) running = false;
        }
        if (onGround) player.lastY = player.y;

        // Boundary checks
        if (player.x < 0) player.x = 0;
        if (player.x + player.width > SCREEN_WIDTH) player.x = SCREEN_WIDTH - player.width;
        if (player.y < 0) player.y = 0;

        // Update ghost
        if (SDL_GetTicks() - ghostTimer > 5000 && !ghost.active) { // Spawn every 5 seconds
            ghost.active = true;
            Mix_PlayChannel(-1, ghostSound, 0);
        }
        if (ghost.active) {
            ghost.x += (player.x > ghost.x) ? 1 : -1;
            ghost.y += (player.y > ghost.y) ? 1 : -1;
            if (SDL_HasIntersection(&playerRect, &(SDL_Rect){(int)ghost.x, (int)ghost.y, ghost.width, ghost.height})) {
                player.lives--;
                Mix_PlayChannel(-1, deathSound, 0);
                player.x = SCREEN_WIDTH / 2;
                player.y = SCREEN_HEIGHT - PLAYER_HEIGHT - TILE_SIZE;
                ghost.active = false;
                if (player.lives <= 0) running = false;
            }
        }

        // Rendering
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, bgTex, NULL, NULL); // Background

        // Draw platforms
        for (int i = 0; i < 2; i++) {
            SDL_Rect platRect = {platforms[i].x, platforms[i].y, platforms[i].width, platforms[i].height};
            SDL_RenderCopy(renderer, platformTex, NULL, &platRect);
        }

        // Draw rope
        SDL_Rect ropeRectRender = {rope.x, rope.y, TILE_SIZE, rope.height};
        SDL_RenderCopy(renderer, ropeTex, NULL, &ropeRectRender);

        // Draw ghost
        if (ghost.active) {
            SDL_Rect ghostRect = {(int)ghost.x, (int)ghost.y, ghost.width, ghost.height};
            SDL_RenderCopy(renderer, ghostTex, NULL, &ghostRect);
        }

        // Draw player
        SDL_Rect playerRectRender = {(int)player.x, (int)player.y, player.width, player.height};
        SDL_RenderCopy(renderer, playerTex, NULL, &playerRectRender);

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60 FPS
    }

    // Cleanup
    Mix_FreeChunk(jumpSound);
    Mix_FreeChunk(deathSound);
    Mix_FreeChunk(ghostSound);
    Mix_FreeMusic(bgMusic);
    SDL_DestroyTexture(playerTex);
    SDL_DestroyTexture(ghostTex);
    SDL_DestroyTexture(bgTex);
    SDL_DestroyTexture(platformTex);
    SDL_DestroyTexture(ropeTex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_Quit();
    IMG_Quit();
    SDL_Quit();

    printf("Game Over! Final Score: %d\n", score);
    return 0;
}
