#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <memory>
#include <vector>
#include <stdexcept>

constexpr int TILE_SIZE = 128;
constexpr int VIEWPORT_WIDTH = 1280;
constexpr int VIEWPORT_HEIGHT = 1280;
constexpr int SCREEN_WIDTH = 1920;
constexpr int SCREEN_HEIGHT = 1080;
constexpr int MAP_WIDTH = 19200;  // in tiles
constexpr int MAP_HEIGHT = 10800; // in tiles
constexpr int HELI_SIZE = 64;
constexpr float PLAYER_SPEED = 300.0f;
constexpr Uint32 TARGET_FPS = 60;
constexpr Uint32 FRAME_DELAY = 1000 / TARGET_FPS;

// RAII wrappers for SDL resources
struct SDLWindowDeleter { void operator()(SDL_Window* ptr) { if (ptr) SDL_DestroyWindow(ptr); } };
struct SDLRendererDeleter { void operator()(SDL_Renderer* ptr) { if (ptr) SDL_DestroyRenderer(ptr); } };
struct SDLTextureDeleter { void operator()(SDL_Texture* ptr) { if (ptr) SDL_DestroyTexture(ptr); } };

using UniqueWindow = std::unique_ptr<SDL_Window, SDLWindowDeleter>;
using UniqueRenderer = std::unique_ptr<SDL_Renderer, SDLRendererDeleter>;
using UniqueTexture = std::unique_ptr<SDL_Texture, SDLTextureDeleter>;

struct Vector2 {
    float x, y;
    bool operator!=(const Vector2& other) const { return x != other.x || y != other.y; }
};

class Helicopter {
public:
    Vector2 position;
    float speed = PLAYER_SPEED;
    
    Helicopter(float x, float y) : position{x, y} {}
};

class Game {
private:
    UniqueWindow window;
    UniqueRenderer renderer;
    bool running = false;
    Helicopter player{MAP_WIDTH * TILE_SIZE / 2.0f, MAP_HEIGHT * TILE_SIZE / 2.0f};
    Vector2 camera{0, 0};
    std::vector<Helicopter> enemies;
    
    UniqueTexture playerTexture;
    UniqueTexture enemyTexture;
    UniqueTexture tileTexture;

    SDL_Texture* loadTexture(const char* path) {
        SDL_Surface* surface = IMG_Load(path);
        if (!surface) {
            throw std::runtime_error("Failed to load image " + std::string(path) + ": " + IMG_GetError());
        }
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer.get(), surface);
        SDL_FreeSurface(surface);
        if (!texture) {
            throw std::runtime_error("Failed to create texture from surface: " + SDL_GetError());
        }
        return texture;
    }

public:
    Game() {
        if (SDL_Init(SDL_INIT_VIDEO) < 0 || IMG_Init(IMG_INIT_PNG) == 0) {
            throw std::runtime_error("SDL Initialization failed: " + std::string(SDL_GetError()));
        }

        window = UniqueWindow(SDL_CreateWindow("Helicopter Game",
                                             SDL_WINDOWPOS_CENTERED,
                                             SDL_WINDOWPOS_CENTERED,
                                             SCREEN_WIDTH, SCREEN_HEIGHT,
                                             SDL_WINDOW_SHOWN));
        if (!window) {
            throw std::runtime_error("Window creation failed: " + std::string(SDL_GetError()));
        }

        renderer = UniqueRenderer(SDL_CreateRenderer(window.get(), -1, SDL_RENDERER_ACCELERATED));
        if (!renderer) {
            throw std::runtime_error("Renderer creation failed: " + std::string(SDL_GetError()));
        }

        playerTexture = UniqueTexture(loadTexture("player_heli.png"));
        enemyTexture = UniqueTexture(loadTexture("enemy_heli.png"));
        tileTexture = UniqueTexture(loadTexture("ground_tile.png"));

        running = true;
        enemies.emplace_back(1000, 1000);
        enemies.emplace_back(1200, 800);
    }

    void handleInput(float deltaTime) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
        }

        const Uint8* keys = SDL_GetKeyboardState(NULL);
        if (keys[SDL_SCANCODE_W]) player.position.y -= speed * deltaTime;
        if (keys[SDL_SCANCODE_S]) player.position.y += speed * deltaTime;
        if (keys[SDL_SCANCODE_A]) player.position.x -= speed * deltaTime;
        if (keys[SDL_SCANCODE_D]) player.position.x += speed * deltaTime;

        // Keep player within map bounds
        player.position.x = std::max(0.0f, std::min(player.position.x, MAP_WIDTH * TILE_SIZE - HELI_SIZE));
        player.position.y = std::max(0.0f, std::min(player.position.y, MAP_HEIGHT * TILE_SIZE - HELI_SIZE));
    }

    void updateCamera() {
        static Vector2 lastPlayerPos{-1, -1};
        if (lastPlayerPos != player.position) {
            camera.x = player.position.x - VIEWPORT_WIDTH / 2;
            camera.y = player.position.y - VIEWPORT_HEIGHT / 2;
            
            camera.x = std::max(0.0f, std::min(camera.x, MAP_WIDTH * TILE_SIZE - VIEWPORT_WIDTH));
            camera.y = std::max(0.0f, std::min(camera.y, MAP_HEIGHT * TILE_SIZE - VIEWPORT_HEIGHT));
            lastPlayerPos = player.position;
        }
    }

    void render() {
        SDL_RenderClear(renderer.get());

        // Pre-calculate visible tile range
        const int tilesX = VIEWPORT_WIDTH / TILE_SIZE + 2;
        const int tilesY = VIEWPORT_HEIGHT / TILE_SIZE + 2;
        const int startX = static_cast<int>(camera.x) / TILE_SIZE;
        const int startY = static_cast<int>(camera.y) / TILE_SIZE;

        SDL_Rect srcRect{0, 0, TILE_SIZE, TILE_SIZE};
        SDL_Rect dstRect{0, 0, TILE_SIZE, TILE_SIZE};

        // Render tiles
        for (int y = startY; y < startY + tilesY; ++y) {
            for (int x = startX; x < startX + tilesX; ++x) {
                if (x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_HEIGHT) {
                    dstRect.x = x * TILE_SIZE - camera.x + (SCREEN_WIDTH - VIEWPORT_WIDTH) / 2;
                    dstRect.y = y * TILE_SIZE - camera.y + (SCREEN_HEIGHT - VIEWPORT_HEIGHT) / 2;
                    SDL_RenderCopy(renderer.get(), tileTexture.get(), &srcRect, &dstRect);
                }
            }
        }

        // Render player
        SDL_Rect playerRect{
            static_cast<int>(player.position.x - camera.x + (SCREEN_WIDTH - VIEWPORT_WIDTH) / 2),
            static_cast<int>(player.position.y - camera.y + (SCREEN_HEIGHT - VIEWPORT_HEIGHT) / 2),
            HELI_SIZE, HELI_SIZE
        };
        SDL_RenderCopy(renderer.get(), playerTexture.get(), nullptr, &playerRect);

        // Render enemies
        for (const auto& enemy : enemies) {
            SDL_Rect enemyRect{
                static_cast<int>(enemy.position.x - camera.x + (SCREEN_WIDTH - VIEWPORT_WIDTH) / 2),
                static_cast<int>(enemy.position.y - camera.y + (SCREEN_HEIGHT - VIEWPORT_HEIGHT) / 2),
                HELI_SIZE, HELI_SIZE
            };
            SDL_RenderCopy(renderer.get(), enemyTexture.get(), nullptr, &enemyRect);
        }

        SDL_RenderPresent(renderer.get());
    }

    void run() {
        Uint32 lastTime = SDL_GetTicks();
        while (running) {
            Uint32 currentTime = SDL_GetTicks();
            float deltaTime = (currentTime - lastTime) / 1000.0f;
            lastTime = currentTime;

            handleInput(deltaTime);
            updateCamera();
            render();

            Uint32 frameTime = SDL_GetTicks() - currentTime;
            if (frameTime < FRAME_DELAY) {
                SDL_Delay(FRAME_DELAY - frameTime);
            }
        }
    }

    ~Game() {
        // Smart pointers handle cleanup automatically
        IMG_Quit();
        SDL_Quit();
    }
};

int main() {
    try {
        Game game;
        game.run();
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
