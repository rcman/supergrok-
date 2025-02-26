#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <iostream>

// Screen dimensions
const int SCREEN_WIDTH = 1920;
const int SCREEN_HEIGHT = 1080;
const int VIRTUAL_WIDTH = 640;   // Original game width
const int VIRTUAL_HEIGHT = 480;  // Original game height
const float SCALE_FACTOR = 2.25f; // 1080 / 480
const int OFFSET_X = 240;        // (1920 - 1440) / 2

// Player properties
const float PLAYER_SPEED = 300.0f;
const int PLAYER_WIDTH = 32;
const int PLAYER_HEIGHT = 32;

// Bullet properties
const float BULLET_SPEED = 500.0f;
const int BULLET_WIDTH = 8;
const int BULLET_HEIGHT = 16;

// Enemy properties
const float ENEMY_SPEED = 100.0f;
const int ENEMY_WIDTH = 32;
const int ENEMY_HEIGHT = 32;

// Power-up properties
const int POWERUP_WIDTH = 16;
const int POWERUP_HEIGHT = 16;

struct Entity {
    float x, y;  // Virtual coordinates
    int w, h;    // Virtual size
    SDL_Texture* texture;
};

struct Player : Entity {
    int shootCooldown;
    int powerLevel; // 0 = single shot, 1 = double shot
};

struct Bullet : Entity {
    bool active;
};

struct Enemy : Entity {
    bool active;
};

struct PowerUp : Entity {
    bool active;
};

// Load texture helper
SDL_Texture* loadTexture(const std::string& path, SDL_Renderer* renderer) {
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) {
        std::cerr << "Failed to load image: " << IMG_GetError() << std::endl;
        return nullptr;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

int main(int argc, char* argv[]) {
    srand(time(NULL));

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0 || IMG_Init(IMG_INIT_PNG) == 0 || Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0 || TTF_Init() < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("Super Rapid Fire Clone", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // Load assets
    SDL_Texture* playerTexture = loadTexture("player.png", renderer);       // 32x32px
    SDL_Texture* bulletTexture = loadTexture("bullet.png", renderer);       // 8x16px
    SDL_Texture* enemyTexture = loadTexture("enemy.png", renderer);         // 32x32px
    SDL_Texture* powerUpTexture = loadTexture("powerup.png", renderer);     // 16x16px
    SDL_Texture* bgTexture = loadTexture("background.png", renderer);       // 640x960px
    Mix_Chunk* shootSound = Mix_LoadWAV("shoot.wav");
    Mix_Chunk* explosionSound = Mix_LoadWAV("explosion.wav");
    TTF_Font* font = TTF_OpenFont("arial.ttf", 24);

    // Initialize player
    Player player = { VIRTUAL_WIDTH / 2.0f - PLAYER_WIDTH / 2.0f, VIRTUAL_HEIGHT - PLAYER_HEIGHT - 20, PLAYER_WIDTH, PLAYER_HEIGHT, playerTexture, 10, 0 };

    // Game objects
    std::vector<Bullet> bullets;
    std::vector<Enemy> enemies;
    std::vector<PowerUp> powerUps;
    float bgY = 0.0f; // Virtual scroll position
    int score = 0;
    int enemySpawnTimer = 0;

    // Game loop
    bool quit = false;
    SDL_Event e;
    Uint32 lastTime = SDL_GetTicks();
    const Uint8* keyboardState = SDL_GetKeyboardState(NULL);

    while (!quit) {
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        // Input handling
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) quit = true;
        }

        // Player movement (virtual coordinates)
        if (keyboardState[SDL_SCANCODE_LEFT]) player.x -= PLAYER_SPEED * deltaTime;
        if (keyboardState[SDL_SCANCODE_RIGHT]) player.x += PLAYER_SPEED * deltaTime;
        if (player.x < 0) player.x = 0;
        if (player.x + PLAYER_WIDTH > VIRTUAL_WIDTH) player.x = VIRTUAL_WIDTH - PLAYER_WIDTH;

        // Shooting
        if (keyboardState[SDL_SCANCODE_SPACE] && player.shootCooldown <= 0) {
            Bullet bullet = { player.x + PLAYER_WIDTH / 2 - BULLET_WIDTH / 2, player.y - BULLET_HEIGHT, BULLET_WIDTH, BULLET_HEIGHT, bulletTexture, true };
            bullets.push_back(bullet);
            if (player.powerLevel >= 1) { // Double shot
                Bullet bullet2 = { player.x + PLAYER_WIDTH / 2 - BULLET_WIDTH / 2 - 20, player.y - BULLET_HEIGHT, BULLET_WIDTH, BULLET_HEIGHT, bulletTexture, true };
                bullets.push_back(bullet2);
            }
            Mix_PlayChannel(-1, shootSound, 0);
            player.shootCooldown = 10;
        }
        if (player.shootCooldown > 0) player.shootCooldown--;

        // Update bullets
        for (auto& bullet : bullets) {
            if (!bullet.active) continue;
            bullet.y -= BULLET_SPEED * deltaTime;
            if (bullet.y + BULLET_HEIGHT < 0) bullet.active = false;
        }

        // Spawn enemies
        enemySpawnTimer--;
        if (enemySpawnTimer <= 0) {
            Enemy enemy = { static_cast<float>(rand() % (VIRTUAL_WIDTH - ENEMY_WIDTH)), -ENEMY_HEIGHT, ENEMY_WIDTH, ENEMY_HEIGHT, enemyTexture, true };
            enemies.push_back(enemy);
            enemySpawnTimer = 30 + (rand() % 20);
        }

        // Update enemies
        for (auto& enemy : enemies) {
            if (!enemy.active) continue;
            enemy.y += ENEMY_SPEED * deltaTime;
            if (enemy.y > VIRTUAL_HEIGHT) enemy.active = false;

            // Collision with player
            SDL_Rect playerRect = { static_cast<int>(player.x), static_cast<int>(player.y), PLAYER_WIDTH, PLAYER_HEIGHT };
            SDL_Rect enemyRect = { static_cast<int>(enemy.x), static_cast<int>(enemy.y), ENEMY_WIDTH, ENEMY_HEIGHT };
            if (SDL_HasIntersection(&playerRect, &enemyRect)) {
                std::cout << "Game Over! Score: " << score << std::endl;
                quit = true;
            }

            // Collision with bullets
            for (auto& bullet : bullets) {
                if (!bullet.active) continue;
                SDL_Rect bulletRect = { static_cast<int>(bullet.x), static_cast<int>(bullet.y), BULLET_WIDTH, BULLET_HEIGHT };
                if (SDL_HasIntersection(&bulletRect, &enemyRect)) {
                    bullet.active = false;
                    enemy.active = false;
                    Mix_PlayChannel(-1, explosionSound, 0);
                    score += 10;
                    if (rand() % 100 < 10) {
                        PowerUp powerUp = { enemy.x, enemy.y, POWERUP_WIDTH, POWERUP_HEIGHT, powerUpTexture, true };
                        powerUps.push_back(powerUp);
                    }
                }
            }
        }

        // Update power-ups
        for (auto& powerUp : powerUps) {
            if (!powerUp.active) continue;
            powerUp.y += ENEMY_SPEED * deltaTime;
            if (powerUp.y > VIRTUAL_HEIGHT) powerUp.active = false;

            SDL_Rect powerUpRect = { static_cast<int>(powerUp.x), static_cast<int>(powerUp.y), POWERUP_WIDTH, POWERUP_HEIGHT };
            SDL_Rect playerRect = { static_cast<int>(player.x), static_cast<int>(player.y), PLAYER_WIDTH, PLAYER_HEIGHT };
            if (SDL_HasIntersection(&powerUpRect, &playerRect)) {
                powerUp.active = false;
                if (player.powerLevel < 1) player.powerLevel++;
            }
        }

        // Scroll background (virtual coordinates)
        bgY += 100.0f * deltaTime;
        if (bgY >= VIRTUAL_HEIGHT) bgY -= VIRTUAL_HEIGHT;

        // Render
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Render background
        SDL_Rect bgSrc1 = { 0, static_cast<int>(bgY), VIRTUAL_WIDTH, VIRTUAL_HEIGHT - static_cast<int>(bgY) };
        SDL_Rect bgDst1 = { OFFSET_X, 0, static_cast<int>(VIRTUAL_WIDTH * SCALE_FACTOR), static_cast<int>((VIRTUAL_HEIGHT - bgY) * SCALE_FACTOR) };
        SDL_Rect bgSrc2 = { 0, 0, VIRTUAL_WIDTH, static_cast<int>(bgY) };
        SDL_Rect bgDst2 = { OFFSET_X, static_cast<int>((VIRTUAL_HEIGHT - bgY) * SCALE_FACTOR), static_cast<int>(VIRTUAL_WIDTH * SCALE_FACTOR), static_cast<int>(bgY * SCALE_FACTOR) };
        SDL_RenderCopy(renderer, bgTexture, &bgSrc1, &bgDst1);
        SDL_RenderCopy(renderer, bgTexture, &bgSrc2, &bgDst2);

        // Render player
        SDL_Rect playerDst = { static_cast<int>(player.x * SCALE_FACTOR) + OFFSET_X, static_cast<int>(player.y * SCALE_FACTOR), static_cast<int>(PLAYER_WIDTH * SCALE_FACTOR), static_cast<int>(PLAYER_HEIGHT * SCALE_FACTOR) };
        SDL_RenderCopy(renderer, player.texture, NULL, &playerDst);

        // Render bullets
        for (const auto& bullet : bullets) {
            if (bullet.active) {
                SDL_Rect bulletDst = { static_cast<int>(bullet.x * SCALE_FACTOR) + OFFSET_X, static_cast<int>(bullet.y * SCALE_FACTOR), static_cast<int>(BULLET_WIDTH * SCALE_FACTOR), static_cast<int>(BULLET_HEIGHT * SCALE_FACTOR) };
                SDL_RenderCopy(renderer, bullet.texture, NULL, &bulletDst);
            }
        }

        // Render enemies
        for (const auto& enemy : enemies) {
            if (enemy.active) {
                SDL_Rect enemyDst = { static_cast<int>(enemy.x * SCALE_FACTOR) + OFFSET_X, static_cast<int>(enemy.y * SCALE_FACTOR), static_cast<int>(ENEMY_WIDTH * SCALE_FACTOR), static_cast<int>(ENEMY_HEIGHT * SCALE_FACTOR) };
                SDL_RenderCopy(renderer, enemy.texture, NULL, &enemyDst);
            }
        }

        // Render power-ups
        for (const auto& powerUp : powerUps) {
            if (powerUp.active) {
                SDL_Rect powerUpDst = { static_cast<int>(powerUp.x * SCALE_FACTOR) + OFFSET_X, static_cast<int>(powerUp.y * SCALE_FACTOR), static_cast<int>(POWERUP_WIDTH * SCALE_FACTOR), static_cast<int>(POWERUP_HEIGHT * SCALE_FACTOR) };
                SDL_RenderCopy(renderer, powerUp.texture, NULL, &powerUpDst);
            }
        }

        // Render score
        std::string scoreText = "Score: " + std::to_string(score);
        SDL_Surface* textSurface = TTF_RenderText_Solid(font, scoreText.c_str(), {255, 255, 255});
        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        SDL_Rect textDst = { OFFSET_X + 10, 10, textSurface->w, textSurface->h };
        SDL_RenderCopy(renderer, textTexture, NULL, &textDst);
        SDL_FreeSurface(textSurface);
        SDL_DestroyTexture(textTexture);

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60 FPS
    }

    // Cleanup
    SDL_DestroyTexture(playerTexture);
    SDL_DestroyTexture(bulletTexture);
    SDL_DestroyTexture(enemyTexture);
    SDL_DestroyTexture(powerUpTexture);
    SDL_DestroyTexture(bgTexture);
    Mix_FreeChunk(shootSound);
    Mix_FreeChunk(explosionSound);
    TTF_CloseFont(font);
    Mix_CloseAudio();
    TTF_Quit();
    IMG_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
