#include <SDL2/SDL.h>
#include <vector>
#include <cmath>
#include <ctime>

// Constants
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int MAP_WIDTH = 2000;
const int MAP_HEIGHT = 2000;
const int PLAYER_SIZE = 20;
const int CAR_SIZE = 40;
const int BULLET_SIZE = 5;
const int PED_SIZE = 20;

// Game states
enum PlayerState { ON_FOOT, IN_VEHICLE };
enum GameState { PLAYING };

// Player class
struct Player {
    float x = 100.0f, y = 100.0f;
    float angle = 0.0f;
    float speed = 0.0f;
    float maxSpeed = 5.0f;
    float acceleration = 0.1f;
    float deceleration = 0.05f;
    float turnSpeed = 0.05f;
    PlayerState state = ON_FOOT;
    int animFrame = 0;
    int animDelay = 0;
    int wantedLevel = 0;
};

// Building structure
struct Building {
    SDL_Rect rect;
};

// Pedestrian structure
struct Pedestrian {
    float x, y;
    float angle;
    float speed = 1.0f;
    int animFrame = 0;
    int animDelay = 0;
};

// Police structure
struct Police {
    float x, y;
    float angle;
    float speed = 2.0f;
    bool active = false;
};

// Bullet structure
struct Bullet {
    float x, y;
    float angle;
    float speed = 10.0f;
    bool active = false;
};

// Game objects
Player player;
std::vector<Building> buildings;
std::vector<Pedestrian> pedestrians;
std::vector<Police> policeCars;
std::vector<Bullet> bullets;
bool keys[4] = {false};
bool shootPressed = false;
float cameraX = 0.0f, cameraY = 0.0f;

// Collision detection
bool collidesWithBuilding(float x, float y, float size) {
    SDL_FRect playerRect = {x - size / 2, y - size / 2, size, size};
    for (const auto& building : buildings) {
        SDL_FRect bRect = {(float)building.rect.x, (float)building.rect.y, (float)building.rect.w, (float)building.rect.h};
        if (SDL_HasIntersectionF(&playerRect, &bRect)) return true;
    }
    return false;
}

// Initialize map
void initMap() {
    srand((unsigned)time(NULL));
    for (int y = 0; y < MAP_HEIGHT; y += 200) {
        for (int x = 0; x < MAP_WIDTH; x += 200) {
            if ((x / 200 + y / 200) % 2 == 0) {
                buildings.push_back({{x + 50, y + 50, 100, 100}});
            }
        }
    }
    for (int i = 0; i < 10; i++) {
        pedestrians.push_back({(float)(rand() % MAP_WIDTH), (float)(rand() % MAP_HEIGHT), (float)(rand() % 360) * M_PI / 180.0f});
    }
    policeCars.push_back({1000.0f, 1000.0f, 0.0f});
}

// Update game logic
void update(float deltaTime) {
    // Player update
    float size = (player.state == IN_VEHICLE) ? CAR_SIZE : PLAYER_SIZE;
    if (player.state == IN_VEHICLE) {
        if (keys[2]) player.speed += player.acceleration * deltaTime;
        if (keys[3]) player.speed -= player.acceleration * deltaTime;
        if (!keys[2] && !keys[3]) {
            if (player.speed > 0) player.speed -= player.deceleration * deltaTime;
            else if (player.speed < 0) player.speed += player.deceleration * deltaTime;
            if (fabs(player.speed) < player.deceleration) player.speed = 0;
        }
        player.speed = std::max(-player.maxSpeed / 2, std::min(player.maxSpeed, player.speed));
        if (keys[0]) player.angle += player.turnSpeed * (player.speed != 0) * deltaTime;
        if (keys[1]) player.angle -= player.turnSpeed * (player.speed != 0) * deltaTime;

        float newX = player.x + cos(player.angle) * player.speed * deltaTime;
        float newY = player.y - sin(player.angle) * player.speed * deltaTime;
        if (!collidesWithBuilding(newX, newY, size)) {
            player.x = newX;
            player.y = newY;
        } else {
            player.speed = 0;
        }
    } else {
        float moveSpeed = 3.0f;
        float dx = (keys[1] - keys[0]) * moveSpeed * deltaTime;
        float dy = (keys[3] - keys[2]) * moveSpeed * deltaTime;
        float newX = player.x + dx;
        float newY = player.y + dy;
        if (!collidesWithBuilding(newX, newY, size)) {
            player.x = newX;
            player.y = newY;
        }
        if (dx != 0 || dy != 0) {
            player.animDelay++;
            if (player.animDelay > 5) {
                player.animFrame = (player.animFrame + 1) % 4;
                player.animDelay = 0;
            }
            player.angle = atan2(-dy, dx);
        } else {
            player.animFrame = 0;
        }
    }

    // Shooting
    if (shootPressed && bullets.size() < 10) {
        bullets.push_back({player.x, player.y, player.angle, 10.0f, true});
        if (player.wantedLevel < 3) player.wantedLevel++;
        shootPressed = false;
    }

    // Update bullets
    for (auto it = bullets.begin(); it != bullets.end();) {
        if (it->active) {
            it->x += cos(it->angle) * it->speed * deltaTime;
            it->y -= sin(it->angle) * it->speed * deltaTime;
            if (it->x < 0 || it->x > MAP_WIDTH || it->y < 0 || it->y > MAP_HEIGHT || collidesWithBuilding(it->x, it->y, BULLET_SIZE)) {
                it->active = false;
            }
            if (!it->active) it = bullets.erase(it);
            else ++it;
        } else {
            ++it;
        }
    }

    // Update pedestrians
    for (auto& ped : pedestrians) {
        float newX = ped.x + cos(ped.angle) * ped.speed * deltaTime;
        float newY = ped.y - sin(ped.angle) * ped.speed * deltaTime;
        if (!collidesWithBuilding(newX, newY, PED_SIZE) && newX > 0 && newX < MAP_WIDTH && newY > 0 && newY < MAP_HEIGHT) {
            ped.x = newX;
            ped.y = newY;
        } else {
            ped.angle += (rand() % 180 - 90) * M_PI / 180.0f;
        }
        ped.animDelay++;
        if (ped.animDelay > 10) {
            ped.animFrame = (ped.animFrame + 1) % 4;
            ped.animDelay = 0;
        }

        for (auto& bullet : bullets) {
            if (bullet.active && SDL_HasIntersectionF(&(SDL_FRect){bullet.x, bullet.y, BULLET_SIZE, BULLET_SIZE},
                                                      &(SDL_FRect){ped.x - PED_SIZE / 2, ped.y - PED_SIZE / 2, PED_SIZE, PED_SIZE})) {
                ped.x = (float)(rand() % MAP_WIDTH);
                ped.y = (float)(rand() % MAP_HEIGHT);
                bullet.active = false;
                if (player.wantedLevel < 3) player.wantedLevel++;
            }
        }
    }

    // Update police
    for (auto& police : policeCars) {
        if (player.wantedLevel > 0 && !police.active) police.active = true;
        if (police.active) {
            float dx = player.x - police.x;
            float dy = player.y - police.y;
            police.angle = atan2(-dy, dx);
            float newX = police.x + cos(police.angle) * police.speed * deltaTime;
            float newY = police.y - sin(police.angle) * police.speed * deltaTime;
            if (!collidesWithBuilding(newX, newY, CAR_SIZE)) {
                police.x = newX;
                police.y = newY;
            }
        }
    }

    // Update camera
    cameraX = player.x - WINDOW_WIDTH / 2;
    cameraY = player.y - WINDOW_HEIGHT / 2;
    cameraX = std::max(0.0f, std::min(cameraX, (float)(MAP_WIDTH - WINDOW_WIDTH)));
    cameraY = std::max(0.0f, std::min(cameraY, (float)(MAP_HEIGHT - WINDOW_HEIGHT)));
}

// Render function
void render(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderClear(renderer);

    // Render buildings in view
    for (const auto& building : buildings) {
        SDL_Rect drawRect = {building.rect.x - (int)cameraX, building.rect.y - (int)cameraY, building.rect.w, building.rect.h};
        if (drawRect.x + drawRect.w > 0 && drawRect.x < WINDOW_WIDTH && drawRect.y + drawRect.h > 0 && drawRect.y < WINDOW_HEIGHT) {
            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
            SDL_RenderFillRect(renderer, &drawRect);
            SDL_SetRenderDrawColor(renderer, 150, 150, 255, 255);
            SDL_Rect window = {drawRect.x + 10, drawRect.y + 10, 20, 20};
            SDL_RenderFillRect(renderer, &window);
        }
    }

    // Render pedestrians
    for (const auto& ped : pedestrians) {
        int px = (int)ped.x - (int)cameraX - PED_SIZE / 2;
        int py = (int)ped.y - (int)cameraY - PED_SIZE / 2;
        if (px + PED_SIZE > 0 && px < WINDOW_WIDTH && py + PED_SIZE > 0 && py < WINDOW_HEIGHT) {
            SDL_SetRenderDrawColor(renderer, 200, 150, 100, 255);
            SDL_Rect body = {px, py, PED_SIZE, PED_SIZE};
            SDL_RenderFillRect(renderer, &body);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_Rect legL = {body.x + (ped.animFrame % 2 ? 5 : 0), body.y + 15, 5, 5};
            SDL_Rect legR = {body.x + (ped.animFrame % 2 ? 0 : 5), body.y + 15, 5, 5};
            SDL_RenderFillRect(renderer, &legL);
            SDL_RenderFillRect(renderer, &legR);
        }
    }

    // Render police
    for (const auto& police : policeCars) {
        if (police.active) {
            int px = (int)police.x - (int)cameraX - CAR_SIZE / 2;
            int py = (int)police.y - (int)cameraY - CAR_SIZE / 2;
            SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
            SDL_Rect carBody = {px, py, CAR_SIZE, CAR_SIZE};
            SDL_RenderFillRect(renderer, &carBody);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_Rect stripe = {carBody.x + 5, carBody.y + 15, 30, 10};
            SDL_RenderFillRect(renderer, &stripe);
        }
    }

    // Render bullets
    for (const auto& bullet : bullets) {
        if (bullet.active) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
            SDL_Rect bulletRect = {(int)bullet.x - (int)cameraX, (int)bullet.y - (int)cameraY, BULLET_SIZE, BULLET_SIZE};
            SDL_RenderFillRect(renderer, &bulletRect);
        }
    }

    // Render player
    if (player.state == IN_VEHICLE) {
        int px = (int)player.x - (int)cameraX - CAR_SIZE / 2;
        int py = (int)player.y - (int)cameraY - CAR_SIZE / 2;
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_Rect carBody = {px, py, CAR_SIZE, CAR_SIZE};
        SDL_RenderFillRect(renderer, &carBody);
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
        SDL_Rect window = {carBody.x + 10, carBody.y + 5, 20, 10};
        SDL_RenderFillRect(renderer, &window);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        float wheelOffset = cos(player.angle) * 5 * (player.speed > 0 ? 1 : -1);
        SDL_Rect wheel1 = {(int)(carBody.x + 5 + wheelOffset), carBody.y + 30, 10, 10};
        SDL_Rect wheel2 = {(int)(carBody.x + 25 - wheelOffset), carBody.y + 30, 10, 10};
        SDL_RenderFillRect(renderer, &wheel1);
        SDL_RenderFillRect(renderer, &wheel2);
    } else {
        int px = (int)player.x - (int)cameraX - PLAYER_SIZE / 2;
        int py = (int)player.y - (int)cameraY - PLAYER_SIZE / 2;
        SDL_SetRenderDrawColor(renderer, 200, 150, 100, 255);
        SDL_Rect body = {px, py, PLAYER_SIZE, PLAYER_SIZE};
        SDL_RenderFillRect(renderer, &body);
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        SDL_Rect shirt = {body.x + 5, body.y + 5, 10, 10};
        SDL_RenderFillRect(renderer, &shirt);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_Rect legL = {body.x + (player.animFrame % 2 ? 5 : 0), body.y + 15, 5, 5};
        SDL_Rect legR = {body.x + (player.animFrame % 2 ? 0 : 5), body.y + 15, 5, 5};
        SDL_RenderFillRect(renderer, &legL);
        SDL_RenderFillRect(renderer, &legR);
    }

    // Render wanted level
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int i = 0; i < player.wantedLevel; i++) {
        SDL_Rect star = {WINDOW_WIDTH - 40 - i * 40, 10, 30, 30};
        SDL_RenderFillRect(renderer, &star);
    }

    SDL_RenderPresent(renderer);
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("GTA Arcade", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if (!window) {
        SDL_Log("Window creation failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        SDL_Log("Renderer creation failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    initMap();
    bool running = true;
    Uint32 lastTime = SDL_GetTicks();

    while (running) {
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f * 60.0f; // Still assuming 60 FPS for simplicity
        lastTime = currentTime;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_LEFT: keys[0] = true; break;
                    case SDLK_RIGHT: keys[1] = true; break;
                    case SDLK_UP: keys[2] = true; break;
                    case SDLK_DOWN: keys[3] = true; break;
                    case SDLK_SPACE: shootPressed = true; break;
                    case SDLK_e: player.state = (player.state == ON_FOOT) ? IN_VEHICLE : ON_FOOT; break;
                }
            } else if (event.type == SDL_KEYUP) {
                switch (event.key.keysym.sym) {
                    case SDLK_LEFT: keys[0] = false; break;
                    case SDLK_RIGHT: keys[1] = false; break;
                    case SDLK_UP: keys[2] = false; break;
                    case SDLK_DOWN: keys[3] = false; break;
                }
            }
        }

        update(deltaTime);
        render(renderer);
        SDL_Delay(16); // ~60 FPS
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
