#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <vector>
#include <iostream>
#include <string>

const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;
const int PLAYER_SPEED = 4;
const int JUMP_VELOCITY = -12;
const int GRAVITY = 1;
const int SCROLL_SPEED = 2;

// Structs for game objects
struct Vector2 {
    int x, y;
};

struct Player {
    Vector2 pos;
    Vector2 velocity;
    SDL_Texture* texture;
    int width = 64, height = 96;
    bool isJumping = false;
    int frame = 0; // Animation frame
    int character = 0; // 0 = Ax, 1 = Tyris, 2 = Gilius
    int magicPots = 0; // Magic meter
    int health = 3; // Lives
    bool onMount = false;
    SDL_Texture* mountTexture = nullptr;
    int attackCooldown = 0;
};

struct Enemy {
    Vector2 pos;
    SDL_Texture* texture;
    int width = 64, height = 96;
    int type = 0; // 0 = Grunt, 1 = Amazon, 2 = Skeleton, 3 = Knight, 4 = Bad Brother, 5 = Death Adder
    int health = 1;
    bool active = true;
    int frame = 0;
    int attackCooldown = 0;
};

struct Mount {
    Vector2 pos;
    SDL_Texture* texture;
    int width = 96, height = 96;
    int type = 0; // 0 = Chicken Leg, 1 = Red Dragon, 2 = Blue Dragon
    bool active = true;
};

struct Magic {
    Vector2 pos;
    SDL_Texture* texture;
    bool active = false;
    int lifetime = 30; // Frames
};

struct Background {
    SDL_Texture* texture;
    int x = 0;
    int width = SCREEN_WIDTH * 3; // Long level
    int height = SCREEN_HEIGHT;
};

// Global variables
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
Mix_Chunk* hitSound = nullptr;
Mix_Chunk* magicSound = nullptr;
Mix_Chunk* mountSound = nullptr;
Mix_Music* themeMusic = nullptr;
std::vector<Player> players(1); // Single-player by default, expandable to 2
std::vector<Enemy> enemies;
std::vector<Mount> mounts;
std::vector<Magic> magics;
Background background;
int score = 0;
int level = 1;

bool initSDL() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) return false;
    window = SDL_CreateWindow("Golden Axe Clone", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) return false;
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) return false;
    IMG_Init(IMG_INIT_PNG);
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
    return true;
}

SDL_Texture* loadTexture(const char* path) {
    SDL_Surface* surface = IMG_Load(path);
    if (!surface) {
        std::cerr << "Failed to load image: " << IMG_GetError() << std::endl;
        return nullptr;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

void loadAssets() {
    players[0].texture = loadTexture("ax.png"); // Default to Ax Battler
    players[0].pos = {100, SCREEN_HEIGHT - 160};
    players[0].magicPots = 2; // Ax starts with 2 pots

    background.texture = loadTexture("background.png");

    // Enemies for level 1
    enemies.push_back({{800, SCREEN_HEIGHT - 160}, loadTexture("grunt.png"), 64, 96, 0, 1, true});
    enemies.push_back({{900, SCREEN_HEIGHT - 160}, loadTexture("amazon.png"), 64, 96, 1, 1, true});

    // Mounts
    mounts.push_back({{600, SCREEN_HEIGHT - 160}, loadTexture("chicken_leg.png"), 96, 96, 0, true});

    hitSound = Mix_LoadWAV("hit.wav");
    magicSound = Mix_LoadWAV("magic.wav");
    mountSound = Mix_LoadWAV("mount.wav");
    themeMusic = Mix_LoadMUS("theme.wav");
    Mix_PlayMusic(themeMusic, -1);
}

void handleInput(bool& running) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) running = false;
    }

    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    for (auto& player : players) {
        player.velocity.x = 0;
        if (player.attackCooldown > 0) player.attackCooldown--;

        if (keys[SDL_SCANCODE_LEFT] && player.pos.x > 0) player.velocity.x = -PLAYER_SPEED;
        if (keys[SDL_SCANCODE_RIGHT] && player.pos.x < SCREEN_WIDTH - player.width) player.velocity.x = PLAYER_SPEED;
        if (keys[SDL_SCANCODE_UP] && !player.isJumping) {
            player.velocity.y = JUMP_VELOCITY;
            player.isJumping = true;
        }
        if (keys[SDL_SCANCODE_A] && player.attackCooldown == 0) { // Attack
            player.attackCooldown = 20;
            Mix_PlayChannel(-1, hitSound, 0);
        }
        if (keys[SDL_SCANCODE_S] && player.magicPots > 0) { // Magic
            player.magicPots--;
            Magic magic = {{player.pos.x, player.pos.y - 100}, loadTexture(player.character == 0 ? "magic_earth.png" : player.character == 1 ? "magic_fire.png" : "magic_thunder.png"), true};
            magics.push_back(magic);
            Mix_PlayChannel(-1, magicSound, 0);
        }
        if (keys[SDL_SCANCODE_D]) { // Mount/dismount
            for (auto& mount : mounts) {
                if (mount.active && abs(mount.pos.x - player.pos.x) < 50 && abs(mount.pos.y - player.pos.y) < 50) {
                    if (!player.onMount) {
                        player.onMount = true;
                        player.mountTexture = mount.texture;
                        player.width = mount.width;
                        player.height = mount.height;
                        mount.active = false;
                        Mix_PlayChannel(-1, mountSound, 0);
                    }
                }
            }
            if (player.onMount && keys[SDL_SCANCODE_D]) {
                player.onMount = false;
                player.mountTexture = nullptr;
                player.width = 64;
                player.height = 96;
            }
        }
    }
}

void update() {
    // Scroll background
    if (players[0].pos.x > SCREEN_WIDTH / 2 && background.x > -background.width + SCREEN_WIDTH) {
        background.x -= SCROLL_SPEED;
        for (auto& enemy : enemies) enemy.pos.x -= SCROLL_SPEED;
        for (auto& mount : mounts) mount.pos.x -= SCROLL_SPEED;
    }

    // Update players
    for (auto& player : players) {
        player.velocity.y += GRAVITY;
        player.pos.x += player.velocity.x;
        player.pos.y += player.velocity.y;

        if (player.pos.y > SCREEN_HEIGHT - player.height) {
            player.pos.y = SCREEN_HEIGHT - player.height;
            player.velocity.y = 0;
            player.isJumping = false;
        }

        // Animation
        if (player.velocity.x != 0) {
            static int frameCounter = 0;
            frameCounter++;
            if (frameCounter % 10 == 0) player.frame = (player.frame + 1) % 4; // 4 walk frames
        } else {
            player.frame = 0;
        }

        // Attack collision
        if (player.attackCooldown == 19) { // Hit frame
            SDL_Rect attackRect = {player.pos.x + (player.velocity.x >= 0 ? player.width : -32), player.pos.y, 32, player.height};
            for (auto& enemy : enemies) {
                if (!enemy.active) continue;
                SDL_Rect enemyRect = {enemy.pos.x, enemy.pos.y, enemy.width, enemy.height};
                if (SDL_HasIntersection(&attackRect, &enemyRect)) {
                    enemy.health--;
                    if (enemy.health <= 0) {
                        enemy.active = false;
                        score += (enemy.type < 4 ? 100 : enemy.type == 4 ? 500 : 1000); // Grunt/Amazon/Skeleton/Knight = 100, Bad Brother = 500, Death Adder = 1000
                    }
                }
            }
        }
    }

    // Update enemies
    for (auto& enemy : enemies) {
        if (!enemy.active) continue;
        enemy.pos.y = SCREEN_HEIGHT - enemy.height; // Stay on ground
        if (enemy.attackCooldown > 0) enemy.attackCooldown--;

        // Simple AI
        int targetX = players[0].pos.x;
        if (enemy.pos.x > targetX + 50) enemy.pos.x -= 2;
        if (enemy.pos.x < targetX - 50) enemy.pos.x += 2;
        if (abs(enemy.pos.x - targetX) < 60 && enemy.attackCooldown == 0) {
            enemy.attackCooldown = 30;
            SDL_Rect attackRect = {enemy.pos.x + (enemy.pos.x < targetX ? enemy.width : -32), enemy.pos.y, 32, enemy.height};
            for (auto& player : players) {
                SDL_Rect playerRect = {player.pos.x, player.pos.y, player.width, player.height};
                if (SDL_HasIntersection(&attackRect, &playerRect)) {
                    player.health--;
                    if (player.health <= 0) player.health = 0; // Game over condition
                }
            }
        }

        // Animation
        if (enemy.pos.x != targetX) enemy.frame = (enemy.frame + 1) % 4;
        else enemy.frame = 0;
    }

    // Update magic
    for (auto& magic : magics) {
        if (magic.active) {
            magic.lifetime--;
            if (magic.lifetime <= 0) magic.active = false;
            SDL_Rect magicRect = {magic.pos.x, magic.pos.y, 128, 128};
            for (auto& enemy : enemies) {
                if (!enemy.active) continue;
                SDL_Rect enemyRect = {enemy.pos.x, enemy.pos.y, enemy.width, enemy.height};
                if (SDL_HasIntersection(&magicRect, &enemyRect)) {
                    enemy.health -= (players[0].character == 0 ? 1 : players[0].character == 1 ? 2 : 3); // Earth=1, Fire=2, Thunder=3 damage
                    if (enemy.health <= 0) {
                        enemy.active = false;
                        score += (enemy.type < 4 ? 100 : enemy.type == 4 ? 500 : 1000);
                    }
                }
            }
        }
    }

    // Level progression
    static int enemyCount = 2;
    if (enemyCount == 0) {
        level++;
        enemyCount = level * 2;
        enemies.clear();
        if (level == 2) enemies.push_back({{800, SCREEN_HEIGHT - 160}, loadTexture("skeleton.png"), 64, 96, 2, 2, true});
        if (level == 3) enemies.push_back({{800, SCREEN_HEIGHT - 160}, loadTexture("knight.png"), 64, 96, 3, 3, true});
        if (level == 4) {
            enemies.push_back({{800, SCREEN_HEIGHT - 160}, loadTexture("bad_brother.png"), 64, 96, 4, 5, true});
            enemies.push_back({{900, SCREEN_HEIGHT - 160}, loadTexture("bad_brother.png"), 64, 96, 4, 5, true});
        }
        if (level == 5) enemies.push_back({{800, SCREEN_HEIGHT - 160}, loadTexture("death_adder.png"), 64, 96, 5, 10, true});
    } else {
        enemyCount = 0;
        for (const auto& enemy : enemies) if (enemy.active) enemyCount++;
    }
}

void render() {
    SDL_RenderClear(renderer);

    // Render background
    SDL_Rect bgRect = {background.x, 0, background.width, background.height};
    SDL_RenderCopy(renderer, background.texture, nullptr, &bgRect);

    // Render mounts
    for (const auto& mount : mounts) {
        if (mount.active) {
            SDL_Rect mountRect = {mount.pos.x, mount.pos.y, mount.width, mount.height};
            SDL_RenderCopy(renderer, mount.texture, nullptr, &mountRect);
        }
    }

    // Render enemies
    for (const auto& enemy : enemies) {
        if (enemy.active) {
            SDL_Rect srcRect = {enemy.frame * 64, 0, 64, 96};
            SDL_Rect dstRect = {enemy.pos.x, enemy.pos.y, enemy.width, enemy.height};
            SDL_RendererFlip flip = (enemy.pos.x < players[0].pos.x) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
            SDL_RenderCopyEx(renderer, enemy.texture, &srcRect, &dstRect, 0, nullptr, flip);
        }
    }

    // Render magic
    for (const auto& magic : magics) {
        if (magic.active) {
            SDL_Rect magicRect = {magic.pos.x, magic.pos.y, 128, 128};
            SDL_RenderCopy(renderer, magic.texture, nullptr, &magicRect);
        }
    }

    // Render players
    for (const auto& player : players) {
        SDL_Rect srcRect = {player.frame * 64, 0, 64, 96};
        SDL_Rect dstRect = {player.pos.x, player.pos.y, player.width, player.height};
        SDL_RendererFlip flip = (player.velocity.x < 0) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
        SDL_RenderCopyEx(renderer, player.onMount ? player.mountTexture : player.texture, &srcRect, &dstRect, 0, nullptr, flip);
    }

    SDL_RenderPresent(renderer);
}

void cleanup() {
    for (auto& player : players) {
        SDL_DestroyTexture(player.texture);
        if (player.mountTexture) SDL_DestroyTexture(player.mountTexture);
    }
    SDL_DestroyTexture(background.texture);
    for (auto& enemy : enemies) SDL_DestroyTexture(enemy.texture);
    for (auto& mount : mounts) SDL_DestroyTexture(mount.texture);
    for (auto& magic : magics) SDL_DestroyTexture(magic.texture);
    Mix_FreeChunk(hitSound);
    Mix_FreeChunk(magicSound);
    Mix_FreeChunk(mountSound);
    Mix_FreeMusic(themeMusic);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_Quit();
    IMG_Quit();
    SDL_Quit();
}

int main(int argc, char* args[]) {
    if (!initSDL()) return 1;
    loadAssets();

    bool running = true;
    while (running && players[0].health > 0) {
        handleInput(running);
        update();
        render();
        SDL_Delay(16); // ~60 FPS
    }

    cleanup();
    return 0;
}
