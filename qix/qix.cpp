#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include <queue>
#include <iostream>
#include <algorithm>

// Constants
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int GRID_WIDTH = 256;
const int GRID_HEIGHT = 256;
const int GRID_X = (SCREEN_WIDTH - GRID_WIDTH) / 2;
const int GRID_Y = (SCREEN_HEIGHT - GRID_HEIGHT) / 2;
const int MARKER_SIZE = 8;
const float MARKER_FAST_SPEED = 4.0f;
const float MARKER_SLOW_SPEED = 2.0f;
const float QIX_SPEED = 3.0f;
const float SPARX_SPEED = 2.0f;
const int WIN_THRESHOLD = 75; // 75% to win

// Point Structure
struct Point {
    int x, y;
    bool operator==(const Point& p) const { return x == p.x && y == p.y; }
};

// Stix Type
enum StixType {
    NONE, FAST, SLOW
};

// Game Class
class Game {
public:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    TTF_Font* font = nullptr;
    SDL_Texture* markerTexture = nullptr;
    SDL_Texture* sparkTexture = nullptr;
    bool grid[GRID_HEIGHT][GRID_WIDTH]; // Claimed (true) or unclaimed (false)
    Point marker = {GRID_WIDTH / 2, 0};
    std::vector<Point> stix;
    StixType stixMode = NONE;
    std::vector<Point> qix = {{GRID_WIDTH / 2, GRID_HEIGHT / 2}};
    std::vector<Point> sparx;
    int score = 0;
    int lives = 3;
    float areaClaimed = 0.0f;
    bool running = true;
    bool levelComplete = false;

    Game() {
        initGrid();
        sparx.push_back({0, 0}); // Initial Sparx on perimeter
        sparx.push_back({GRID_WIDTH - 1, 0});
    }

    bool init() {
        auto cleanupOnFailure = [this]() {
            if (markerTexture) SDL_DestroyTexture(markerTexture);
            if (sparkTexture) SDL_DestroyTexture(sparkTexture);
            if (font) TTF_CloseFont(font);
            if (renderer) SDL_DestroyRenderer(renderer);
            if (window) SDL_DestroyWindow(window);
            TTF_Quit();
            IMG_Quit();
            SDL_Quit();
        };

        if (SDL_Init(SDL_INIT_VIDEO) < 0 || IMG_Init(IMG_INIT_PNG) == 0 || TTF_Init() < 0) {
            std::cerr << "SDL Init failed: " << SDL_GetError() << std::endl;
            cleanupOnFailure();
            return false;
        }
        window = SDL_CreateWindow("QIX Clone", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!window || !renderer) {
            std::cerr << "Window/Renderer creation failed: " << SDL_GetError() << std::endl;
            cleanupOnFailure();
            return false;
        }

        font = TTF_OpenFont("font.ttf", 24);
        markerTexture = IMG_LoadTexture(renderer, "marker.png");
        sparkTexture = IMG_LoadTexture(renderer, "spark.png");

        if (!font || !markerTexture || !sparkTexture) {
            std::cerr << "Asset load failed: " << IMG_GetError() << std::endl;
            cleanupOnFailure();
            return false;
        }
        return true;
    }

    void initGrid() {
        for (int y = 0; y < GRID_HEIGHT; y++) {
            for (int x = 0; x < GRID_WIDTH; x++) {
                grid[y][x] = (x == 0 || x == GRID_WIDTH - 1 || y == 0 || y == GRID_HEIGHT - 1);
            }
        }
        areaClaimed = (float)(GRID_WIDTH * 2 + GRID_HEIGHT * 2 - 4) / (GRID_WIDTH * GRID_HEIGHT) * 100;
    }

    bool isOnPerimeter(int x, int y) {
        if (x < 0 || x >= GRID_WIDTH || y < 0 || y >= GRID_HEIGHT) return false;
        return grid[y][x] && (x == 0 || x == GRID_WIDTH - 1 || y == 0 || y == GRID_HEIGHT - 1 || 
                              (x > 0 && !grid[y][x - 1]) || (x < GRID_WIDTH - 1 && !grid[y][x + 1]) || 
                              (y > 0 && !grid[y - 1][x]) || (y < GRID_HEIGHT - 1 && !grid[y + 1][x]));
    }

    void handleInput() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
        }
        const Uint8* keys = SDL_GetKeyboardState(NULL);
        float speed = keys[SDL_SCANCODE_LSHIFT] ? MARKER_SLOW_SPEED : MARKER_FAST_SPEED;
        stixMode = keys[SDL_SCANCODE_LSHIFT] ? SLOW : (stix.empty() && isOnPerimeter(marker.x, marker.y) ? NONE : FAST);

        int newX = marker.x;
        int newY = marker.y;
        if (keys[SDL_SCANCODE_LEFT]) newX -= speed;
        if (keys[SDL_SCANCODE_RIGHT]) newX += speed;
        if (keys[SDL_SCANCODE_UP]) newY -= speed;
        if (keys[SDL_SCANCODE_DOWN]) newY += speed;

        if (newX >= 0 && newX < GRID_WIDTH && newY >= 0 && newY < GRID_HEIGHT) {
            if (stixMode == NONE && !grid[newY][newX]) {
                // Can't move off perimeter into unclaimed area without drawing
            } else {
                marker.x = newX;
                marker.y = newY;
                if (stixMode != NONE) stix.push_back({marker.x, marker.y});
            }
        }

        // Complete Stix if back on perimeter
        if (!stix.empty() && isOnPerimeter(marker.x, marker.y)) {
            fillArea();
            stix.clear();
            stixMode = NONE;
        }
    }

    void fillArea() {
        std::vector<std::vector<bool>> tempGrid(GRID_HEIGHT, std::vector<bool>(GRID_WIDTH, false));
        for (int y = 0; y < GRID_HEIGHT; y++) {
            for (int x = 0; x < GRID_WIDTH; x++) {
                tempGrid[y][x] = grid[y][x];
            }
        }
        for (const auto& p : stix) {
            tempGrid[p.y][p.x] = true;
        }

        // Flood fill from outside to find unclaimed areas
        std::queue<Point> q;
        std::vector<std::vector<bool>> visited(GRID_HEIGHT, std::vector<bool>(GRID_WIDTH, false));
        q.push({0, 0});
        visited[0][0] = true;
        while (!q.empty()) {
            Point p = q.front();
            q.pop();
            int dx[] = {-1, 1, 0, 0};
            int dy[] = {0, 0, -1, 1};
            for (int i = 0; i < 4; i++) {
                int nx = p.x + dx[i];
                int ny = p.y + dy[i];
                if (nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT && 
                    !tempGrid[ny][nx] && !visited[ny][nx]) {
                    visited[ny][nx] = true;
                    q.push({nx, ny});
                }
            }
        }

        // Claim areas not reached by flood fill
        int claimedPixels = 0;
        for (int y = 0; y < GRID_HEIGHT; y++) {
            for (int x = 0; x < GRID_WIDTH; x++) {
                if (!visited[y][x] && !grid[y][x]) {
                    grid[y][x] = true;
                    claimedPixels++;
                }
            }
        }
        areaClaimed = (float)((GRID_WIDTH * GRID_HEIGHT - claimedPixels) * 100) / (GRID_WIDTH * GRID_HEIGHT);
        score += claimedPixels * (stixMode == SLOW ? 2 : 1); // Double points for slow Stix

        if (areaClaimed >= WIN_THRESHOLD) levelComplete = true;
    }

    void update() {
        // Qix Movement
        for (auto& q : qix) {
            float dx = (rand() % 3 - 1) * QIX_SPEED;
            float dy = (rand() % 3 - 1) * QIX_SPEED;
            int newX = q.x + dx;
            int newY = q.y + dy;
            if (newX >= 0 && newX < GRID_WIDTH && newY >= 0 && newY < GRID_HEIGHT && !grid[newY][newX]) {
                q.x = newX;
                q.y = newY;
            }
            if (!stix.empty() && abs(q.x - marker.x) < MARKER_SIZE && abs(q.y - marker.y) < MARKER_SIZE) {
                lives--;
                stix.clear();
                marker = {GRID_WIDTH / 2, 0};
                if (lives <= 0) running = false;
            }
        }

        // Sparx Movement
        for (auto& s : sparx) {
            int dir = rand() % 4;
            int dx[] = {-1, 1, 0, 0};
            int dy[] = {0, 0, -1, 1};
            int newX = s.x + dx[dir] * SPARX_SPEED;
            int newY = s.y + dy[dir] * SPARX_SPEED;
            if (newX >= 0 && newX < GRID_WIDTH && newY >= 0 && newY < GRID_HEIGHT && 
                isOnPerimeter(newX, newY)) {
                s.x = newX;
                s.y = newY;
            }
            if (abs(s.x - marker.x) < MARKER_SIZE && abs(s.y - marker.y) < MARKER_SIZE) {
                lives--;
                stix.clear();
                marker = {GRID_WIDTH / 2, 0};
                if (lives <= 0) running = false;
            }
        }

        // Spawn new Sparx occasionally
        if (rand() % 300 == 0 && sparx.size() < 4) {
            sparx.push_back({0, 0});
        }
    }

    void render() {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Render Claimed Area
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // Blue fill
        for (int y = 0; y < GRID_HEIGHT; y++) {
            for (int x = 0; x < GRID_WIDTH; x++) {
                if (grid[y][x]) {
                    SDL_Rect rect = {GRID_X + x, GRID_Y + y, 1, 1};
                    SDL_RenderFillRect(renderer, &rect);
                }
            }
        }

        // Render Stix
        if (!stix.empty()) {
            SDL_SetRenderDrawColor(renderer, stixMode == FAST ? 0 : 255, 0, stixMode == FAST ? 255 : 0, 255);
            for (size_t i = 1; i < stix.size(); i++) {
                SDL_RenderDrawLine(renderer, GRID_X + stix[i-1].x, GRID_Y + stix[i-1].y, 
                                   GRID_X + stix[i].x, GRID_Y + stix[i].y);
            }
            SDL_RenderDrawLine(renderer, GRID_X + stix.back().x, GRID_Y + stix.back().y, 
                               GRID_X + marker.x, GRID_Y + marker.y);
        }

        // Render Marker
        SDL_Rect markerRect = {GRID_X + marker.x - MARKER_SIZE / 2, GRID_Y + marker.y - MARKER_SIZE / 2, MARKER_SIZE, MARKER_SIZE};
        SDL_RenderCopy(renderer, markerTexture, NULL, &markerRect);

        // Render Qix
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        for (const auto& q : qix) {
            SDL_Rect qixRect = {GRID_X + q.x - 2, GRID_Y + q.y - 2, 4, 4};
            SDL_RenderFillRect(renderer, &qixRect);
        }

        // Render Sparx
        for (const auto& s : sparx) {
            SDL_Rect sparkRect = {GRID_X + s.x - MARKER_SIZE / 2, GRID_Y + s.y - MARKER_SIZE / 2, MARKER_SIZE, MARKER_SIZE};
            SDL_RenderCopy(renderer, sparkTexture, NULL, &sparkRect);
        }

        // Render UI
        SDL_Color color = {255, 255, 255, 255};
        std::string text = "Score: " + std::to_string(score) + " Lives: " + std::to_string(lives) + 
                           " Area: " + std::to_string(int(areaClaimed)) + "%";
        if (levelComplete) text += " - Level Complete!";
        else if (lives <= 0) text += " - Game Over!";
        SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), color);
        if (surface) {
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            if (texture) {
                SDL_Rect dest = {10, 10, surface->w, surface->h};
                SDL_RenderCopy(renderer, texture, NULL, &dest);
                SDL_DestroyTexture(texture);
            }
            SDL_FreeSurface(surface);
        }

        SDL_RenderPresent(renderer);
    }

    void clean() {
        if (markerTexture) SDL_DestroyTexture(markerTexture);
        if (sparkTexture) SDL_DestroyTexture(sparkTexture);
        if (font) TTF_CloseFont(font);
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
        stix.clear();
        sparx.clear();
        qix.clear();
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
    }
};

int main() {
    Game game;
    if (!game.init()) {
        std::cerr << "Initialization failed: " << SDL_GetError() << std::endl;
        game.clean();
        return 1;
    }

    while (game.running) {
        game.handleInput();
        game.update();
        game.render();
        SDL_Delay(16); // ~60 FPS
    }

    game.clean();
    return 0;
}
