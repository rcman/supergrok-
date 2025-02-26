#include <SDL2/SDL.h>
#include <vector>
#include <cmath>
#include <iostream>

const int SCREEN_WIDTH = 1024;
const int SCREEN_HEIGHT = 768;
const int ROAD_WIDTH = 2000;  // World units
const int SEGMENT_LENGTH = 200;  // Length of each road segment
const float CAMERA_HEIGHT = 1000.0f;
const float CAMERA_DEPTH = 1.0f / tanf((60.0f / 2.0f) * M_PI / 180.0f);  // FOV
const int MAX_SPEED = 300;

struct Segment {
    float x, y, z;  // World position
    float curve;    // Curvature of this segment
    float scale;    // Scaling for pseudo-3D
    int screenY;    // Projected Y position
};

struct Player {
    float x = 0;           // Player's lateral position
    float speed = 0;       // Speed in world units
    float maxSpeed = MAX_SPEED;
    float accel = 200.0f;
    float offRoadDecel = -100.0f;
    float breaking = -300.0f;
    float turnSpeed = 2.0f;
};

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
bool quit = false;

std::vector<Segment> road;
Player player;

void initSDL() {
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("Out Run Clone", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
}

void generateRoad() {
    road.clear();
    for (int i = 0; i < 2000; i++) {  // 2000 segments for a long track
        Segment seg;
        seg.z = i * SEGMENT_LENGTH;
        seg.x = 0;
        seg.y = 0;
        seg.curve = (i > 300 && i < 500) ? 1.0f : (i > 800 && i < 1200) ? -1.0f : 0.0f;  // Simple curves
        road.push_back(seg);
    }
}

void projectSegment(Segment& seg, float camX, float camY, float camZ) {
    float worldX = seg.x - camX;
    float worldY = seg.y - camY;
    float worldZ = seg.z - camZ;

    if (worldZ <= 0) return;  // Behind camera

    seg.scale = CAMERA_DEPTH / worldZ;
    seg.screenY = (int)((1.0f - seg.scale * (worldY - CAMERA_HEIGHT)) * SCREEN_HEIGHT / 2);
}

void renderRoad(float camZ) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    int baseSegment = (int)(camZ / SEGMENT_LENGTH) % road.size();
    float camX = player.x;

    for (int i = 0; i < 300; i++) {  // Render 300 segments ahead
        int idx = (baseSegment + i) % road.size();
        projectSegment(road[idx], camX, 0, camZ);

        if (i == 0) continue;  // Skip first segment for rendering

        int prevIdx = (idx - 1 + road.size()) % road.size();
        Segment& prev = road[prevIdx];
        Segment& curr = road[idx];

        // Draw road trapezoid
        SDL_SetRenderDrawColor(renderer, (i % 2) ? 100 : 150, 100, 100, 255);  // Alternating colors
        SDL_Point points[4] = {
            { (int)(SCREEN_WIDTH / 2 + prev.scale * (prev.x - ROAD_WIDTH / 2)), prev.screenY },
            { (int)(SCREEN_WIDTH / 2 + prev.scale * (prev.x + ROAD_WIDTH / 2)), prev.screenY },
            { (int)(SCREEN_WIDTH / 2 + curr.scale * (curr.x + ROAD_WIDTH / 2)), curr.screenY },
            { (int)(SCREEN_WIDTH / 2 + curr.scale * (curr.x - ROAD_WIDTH / 2)), curr.screenY }
        };
        SDL_RenderDrawLines(renderer, points, 5);  // 5 to close the shape
    }

    // Draw player car (placeholder)
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_Rect carRect = { SCREEN_WIDTH / 2 - 20, SCREEN_HEIGHT - 100, 40, 80 };
    SDL_RenderFillRect(renderer, &carRect);

    SDL_RenderPresent(renderer);
}

void updatePlayer(float dt) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) quit = true;
    }

    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    float accel = 0;
    if (keys[SDL_SCANCODE_UP]) accel = player.accel;
    if (keys[SDL_SCANCODE_DOWN]) accel = player.breaking;
    if (keys[SDL_SCANCODE_LEFT]) player.x -= player.turnSpeed * player.speed * dt;
    if (keys[SDL_SCANCODE_RIGHT]) player.x += player.turnSpeed * player.speed * dt;

    // Speed update
    player.speed += accel * dt;
    if (player.x < -ROAD_WIDTH / 2 || player.x > ROAD_WIDTH / 2) {
        player.speed += player.offRoadDecel * dt;  // Slow down off-road
    }
    player.speed = std::max(0.0f, std::min(player.speed, player.maxSpeed));

    // Curve influence
    int currSegment = (int)(player.speed / SEGMENT_LENGTH) % road.size();
    player.x += road[currSegment].curve * player.speed * dt * 0.5f;
}

int main() {
    initSDL();
    generateRoad();

    float camZ = 0;
    Uint32 lastTime = SDL_GetTicks();

    while (!quit) {
        Uint32 currentTime = SDL_GetTicks();
        float dt = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        updatePlayer(dt);
        camZ += player.speed * dt;
        renderRoad(camZ);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
