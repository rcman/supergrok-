#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <iostream>

const int SCREEN_WIDTH = 384;  // Arcade resolution (scaled up)
const int SCREEN_HEIGHT = 512;
const int PLAYER_SIZE = 32;
const int ENEMY_SIZE = 24;
const int BULLET_SIZE = 8;
const float SCROLL_SPEED = 2.0f;

struct Player {
    float x, y;
    int jetType;  // 0=F-14, 1=MiG-29, 2=F-16, 3=Su-27
    int health;
    int shootCooldown;  // Frames until next shot
};

struct Enemy {
    float x, y;
    bool alive;
    float speed;
};

struct Bullet {
    float x, y;
    float velX, velY;  // Support homing bullets
    bool active;
    int damage;
};

int main(int argc, char* argv[]) {
    // Notes: Using original Storm Blade assets requires owning the arcade board or licensing them.
    // Recreating assets from scratch is an alternative. Extract from MAME ROM (stormbla.zip) if legal.

    // Initialize SDL2 and SDL2_mixer
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL Init failed: " << SDL_GetError() << std::endl;
        return 1;
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "Mixer Init failed: " << Mix_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Storm Blade", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Window failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Renderer failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_RenderSetScale(renderer, 2.0f, 2.0f);  // Scale up for modern screens

    // Load placeholder textures (replace with Storm Blade assets)
    // Note: Extract sprites from ROM using tools like Tile Molester; load jetX.bmp for each jet.
    SDL_Texture* playerTex[4] = {nullptr};
    for (int i = 0; i < 4; i++) {
        SDL_Surface* surf = SDL_CreateRGBSurface(0, PLAYER_SIZE, PLAYER_SIZE, 32, 0, 0, 0, 0);
        SDL_FillRect(surf, NULL, SDL_MapRGB(surf->format, 0, 255 - i * 60, i * 60, 255));
        playerTex[i] = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_FreeSurface(surf);
        if (!playerTex[i]) {
            std::cerr << "Player texture " << i << " failed: " << SDL_GetError() << std::endl;
        }
    }

    SDL_Surface* enemySurf = SDL_CreateRGBSurface(0, ENEMY_SIZE, ENEMY_SIZE, 32, 0, 0, 0, 0);
    SDL_FillRect(enemySurf, NULL, SDL_MapRGB(enemySurf->format, 255, 0, 0));
    SDL_Texture* enemyTex = SDL_CreateTextureFromSurface(renderer, enemySurf);
    SDL_FreeSurface(enemySurf);
    if (!enemyTex) std::cerr << "Enemy texture failed: " << SDL_GetError() << std::endl;

    SDL_Surface* bulletSurf = SDL_CreateRGBSurface(0, BULLET_SIZE, BULLET_SIZE, 32, 0, 0, 0, 0);
    SDL_FillRect(bulletSurf, NULL, SDL_MapRGB(bulletSurf->format, 255, 255, 0));
    SDL_Texture* bulletTex = SDL_CreateTextureFromSurface(renderer, bulletSurf);
    SDL_FreeSurface(bulletSurf);
    if (!bulletTex) std::cerr << "Bullet texture failed: " << SDL_GetError() << std::endl;

    // Load background (placeholder; replace with stormbla background.bmp)
    SDL_Surface* bgSurf = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT * 2, 32, 0, 0, 0, 0);
    for (int y = 0; y < SCREEN_HEIGHT * 2; y++) {
        Uint8 gray = 50 + (y * 205 / (SCREEN_HEIGHT * 2));
        SDL_FillRect(bgSurf, &SDL_Rect{0, y, SCREEN_WIDTH, 1}, SDL_MapRGB(bgSurf->format, gray, gray, gray));
    }
    SDL_Texture* bgTex = SDL_CreateTextureFromSurface(renderer, bgSurf);
    SDL_FreeSurface(bgSurf);
    if (!bgTex) std::cerr << "Background texture failed: " << SDL_GetError() << std::endl;

    // Load sounds (placeholders; replace with original WAVs from ES5506 chip)
    Mix_Chunk* shootSound[4] = {nullptr};  // One per jet
    Mix_Music* bgMusic = nullptr;
    for (int i = 0; i < 4; i++) {
        shootSound[i] = Mix_LoadWAV("shoot.wav");  // Replace with jet-specific sounds (e.g., jet0_shot.wav)
        if (!shootSound[i]) std::cerr << "Shoot sound " << i << " failed: " << Mix_GetError() << std::endl;
    }
    bgMusic = Mix_LoadMUS("bgm.mp3");  // Replace with storm_blade_bgm.wav
    if (!bgMusic) std::cerr << "Music failed: " << Mix_GetError() << std::endl;
    else Mix_PlayMusic(bgMusic, -1);

    // Game state
    Player player = {SCREEN_WIDTH / 2.0f - PLAYER_SIZE / 2.0f, SCREEN_HEIGHT - PLAYER_SIZE - 10, 0, 100, 0};
    std::vector<Enemy> enemies;
    std::vector<Bullet> bullets;
    float bgOffset = 0.0f;
    srand(time(0));
    bool running = true;
    SDL_Event event;

    // Jet selection (simplified menu; expand to graphical)
    std::cout << "Select jet (0=F-14, 1=MiG-29, 2=F-16, 3=Su-27): ";
    std::cin >> player.jetType;
    if (player.jetType < 0 || player.jetType > 3) player.jetType = 0;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
        }
        const Uint8* keys = SDL_GetKeyboardState(NULL);
        if (keys[SDL_SCANCODE_LEFT]) player.x -= 5;
        if (keys[SDL_SCANCODE_RIGHT]) player.x += 5;
        if (keys[SDL_SCANCODE_UP]) player.y -= 5;
        if (keys[SDL_SCANCODE_DOWN]) player.y += 5;
        if (player.x < 0) player.x = 0;
        if (player.x > SCREEN_WIDTH - PLAYER_SIZE) player.x = SCREEN_WIDTH - PLAYER_SIZE;
        if (player.y < 0) player.y = 0;
        if (player.y > SCREEN_HEIGHT - PLAYER_SIZE) player.y = SCREEN_HEIGHT - PLAYER_SIZE;

        // Shooting (arcade-style, jet-specific)
        if (keys[SDL_SCANCODE_Z] && player.shootCooldown <= 0) {
            switch (player.jetType) {
                case 0:  // F-14: Spread shot (3 bullets)
                    bullets.push_back({player.x + PLAYER_SIZE / 2.0f - BULLET_SIZE / 2.0f - 10, player.y, -2.0f, -10.0f, true, 10});
                    bullets.push_back({player.x + PLAYER_SIZE / 2.0f - BULLET_SIZE / 2.0f, player.y, 0, -10.0f, true, 10});
                    bullets.push_back({player.x + PLAYER_SIZE / 2.0f - BULLET_SIZE / 2.0f + 10, player.y, 2.0f, -10.0f, true, 10});
                    player.shootCooldown = 10;
                    break;
                case 1:  // MiG-29: Laser (fast, narrow)
                    bullets.push_back({player.x + PLAYER_SIZE / 2.0f - BULLET_SIZE / 2.0f, player.y, 0, -15.0f, true, 15});
                    player.shootCooldown = 5;
                    break;
                case 2:  // F-16: Homing (tracks nearest enemy)
                    bullets.push_back({player.x + PLAYER_SIZE / 2.0f - BULLET_SIZE / 2.0f, player.y, 0, -10.0f, true, 10});
                    player.shootCooldown = 15;
                    break;
                case 3:  // Su-27: Powerful shot (slow, high damage)
                    bullets.push_back({player.x + PLAYER_SIZE / 2.0f - BULLET_SIZE / 2.0f, player.y, 0, -8.0f, true, 25});
                    player.shootCooldown = 20;
                    break;
            }
            if (shootSound[player.jetType]) Mix_PlayChannel(-1, shootSound[player.jetType], 0);
        }
        if (player.shootCooldown > 0) player.shootCooldown--;

        // Spawn enemies
        if (rand() % 60 == 0) {
            enemies.push_back({float(rand() % (SCREEN_WIDTH - ENEMY_SIZE)), -ENEMY_SIZE, true, SCROLL_SPEED + 1.0f});
        }

        // Update
        bgOffset += SCROLL_SPEED;
        if (bgOffset >= SCREEN_HEIGHT) bgOffset -= SCREEN_HEIGHT;

        for (auto& bullet : bullets) {
            if (bullet.active) {
                bullet.y += bullet.velY;
                bullet.x += bullet.velX;
                if (bullet.y < -BULLET_SIZE || bullet.x < -BULLET_SIZE || bullet.x > SCREEN_WIDTH) bullet.active = false;
                if (player.jetType == 2) {  // F-16 homing
                    float minDist = SCREEN_WIDTH * SCREEN_HEIGHT;
                    Enemy* target = nullptr;
                    for (auto& enemy : enemies) {
                        if (enemy.alive) {
                            float dx = enemy.x - bullet.x, dy = enemy.y - bullet.y;
                            float dist = dx * dx + dy * dy;
                            if (dist < minDist) {
                                minDist = dist;
                                target = &enemy;
                            }
                        }
                    }
                    if (target) {
                        float dx = target->x - bullet.x, dy = target->y - bullet.y;
                        float mag = sqrt(dx * dx + dy * dy);
                        if (mag > 0) { bullet.velX = dx / mag * 5; bullet.velY = dy / mag * 5; }
                    }
                }
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

        for (auto& enemy : enemies) {
            if (enemy.alive) {
                enemy.y += enemy.speed;
                if (enemy.y > SCREEN_HEIGHT) enemy.alive = false;
                if (enemy.x + ENEMY_SIZE > player.x && enemy.x < player.x + PLAYER_SIZE &&
                    enemy.y + ENEMY_SIZE > player.y && enemy.y < player.y + PLAYER_SIZE) {
                    player.health -= 10;
                    enemy.alive = false;
                    if (player.health <= 0) running = false;
                }
            }
        }
        enemies.erase(std::remove_if(enemies.begin(), enemies.end(), [](const Enemy& e) { return !e.alive; }), enemies.end());

        // Render
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_Rect bgSrc1 = {0, (int)bgOffset, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_Rect bgDst1 = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_Rect bgSrc2 = {0, (int)bgOffset - SCREEN_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_Rect bgDst2 = {0, -SCREEN_HEIGHT + (int)bgOffset, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_RenderCopy(renderer, bgTex, &bgSrc1, &bgDst1);
        SDL_RenderCopy(renderer, bgTex, &bgSrc2, &bgDst2);

        SDL_Rect playerRect = {(int)player.x, (int)player.y, PLAYER_SIZE, PLAYER_SIZE};
        SDL_RenderCopy(renderer, playerTex[player.jetType], NULL, &playerRect);

        for (const auto& enemy : enemies) {
            if (enemy.alive) {
                SDL_Rect enemyRect = {(int)enemy.x, (int)enemy.y, ENEMY_SIZE, ENEMY_SIZE};
                SDL_RenderCopy(renderer, enemyTex, NULL, &enemyRect);
            }
        }
        for (const auto& bullet : bullets) {
            if (bullet.active) {
                SDL_Rect bulletRect = {(int)bullet.x, (int)bullet.y, BULLET_SIZE, BULLET_SIZE};
                SDL_RenderCopy(renderer, bulletTex, NULL, &bulletRect);
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);  // Note: Runs at ~60 FPS; optimize with delta timing for consistency.
    }

    // Cleanup
    for (int i = 0; i < 4; i++) {
        if (playerTex[i]) SDL_DestroyTexture(playerTex[i]);
        if (shootSound[i]) Mix_FreeChunk(shootSound[i]);
    }
    if (enemyTex) SDL_DestroyTexture(enemyTex);
    if (bulletTex) SDL_DestroyTexture(bulletTex);
    if (bgTex) SDL_DestroyTexture(bgTex);
    if (bgMusic) Mix_FreeMusic(bgMusic);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_CloseAudio();
    SDL_Quit();

    // Note: Expand with bosses, levels (land/sea), HUD using SDL_ttf for score/health.
    return 0;
}
