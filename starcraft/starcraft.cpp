#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_net.h>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <cstring>
#include <queue>
#include <map>

// Constants
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int TILE_SIZE = 32;
const int MAP_WIDTH = 20;
const int MAP_HEIGHT = 15;

// Enums
enum TerrainType { GRASS, DIRT };
enum EntityType { UNIT, BUILDING, RESOURCE };
enum Faction { TERRAN, ZERG, PROTOSS };

// Point structure
struct Point { 
    int x, y; 
    bool operator==(const Point& p) const { return x == p.x && y == p.y; }
    bool operator<(const Point& p) const { return x < p.x || (x == p.x && y < p.y); }
};

// Base Entity class
class Entity {
public:
    EntityType type;
    Faction faction;
    int x, y;
    SDL_Texture* texture;
    int health;
    int damage;
    int range;

    Entity(EntityType t, Faction f, int px, int py, SDL_Texture* tex, int h = 40, int d = 6, int r = 1) 
        : type(t), faction(f), x(px), y(py), texture(tex), health(h), damage(d), range(r) {}
    virtual void update() {}
    virtual void render(SDL_Renderer* renderer) {
        SDL_Rect dest = {x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE};
        SDL_RenderCopy(renderer, texture, NULL, &dest);
    }
    void attack(Entity* target) {
        if (abs(x - target->x) + abs(y - target->y) <= range && target->health > 0 && faction != target->faction) {
            target->health -= damage;
        }
    }
    virtual ~Entity() {}
};

// Resource class
class Resource : public Entity {
public:
    int amount;
    Resource(int px, int py, SDL_Texture* tex, int a) 
        : Entity(RESOURCE, TERRAN, px, py, tex, 100), amount(a) {}
};

// Building class
class Building : public Entity {
public:
    std::vector<EntityType> produceableUnits;
    std::map<EntityType, std::vector<EntityType>> techRequirements;

    Building(int px, int py, SDL_Texture* tex, Faction f, int h = 100) 
        : Entity(BUILDING, f, px, py, tex, h) {}
    bool canProduce(EntityType type, const std::vector<Entity*>& entities) {
        if (minerals < 50 || std::find(produceableUnits.begin(), produceableUnits.end(), type) == produceableUnits.end()) 
            return false;
        if (techRequirements.find(type) == techRequirements.end()) return true;
        for (auto req : techRequirements[type]) {
            bool hasReq = false;
            for (auto* e : entities) {
                if (e->type == BUILDING && dynamic_cast<Building*>(e)->produceableUnits == std::vector<EntityType>{req}) {
                    hasReq = true;
                    break;
                }
            }
            if (!hasReq) return false;
        }
        return true;
    }
    void produceUnit(EntityType type, SDL_Texture* unitTex, std::vector<Entity*>& entities, int& minerals) {
        if (canProduce(type, entities)) {
            entities.push_back(new Entity(type, faction, x + 1, y, unitTex));
            minerals -= 50;
        }
    }
};

// Worker class
class Worker : public Entity {
public:
    bool isCarrying = false;
    Resource* targetResource = nullptr;
    Building* base = nullptr;
    int minerals = 0;
    std::vector<Point> path;
    size_t pathIndex = 0;

    Worker(int px, int py, SDL_Texture* tex, Building* b, Faction f) 
        : Entity(UNIT, f, px, py, tex), base(b) {}

    void moveTo(int tx, int ty, const int (&map)[MAP_HEIGHT][MAP_WIDTH], const std::vector<Entity*>& entities) {
        path = findPath(x, y, tx, ty, map, entities);
        pathIndex = 0;
    }

    void update() override {
        if (!path.empty() && pathIndex < path.size()) {
            x = path[pathIndex].x;
            y = path[pathIndex].y;
            pathIndex++;
            if (pathIndex >= path.size()) path.clear();
        }
        if (targetResource && !isCarrying) {
            if (x == targetResource->x && y == targetResource->y && targetResource->amount > 0) {
                targetResource->amount -= 8;
                minerals += 8;
                isCarrying = true;
            }
        } else if (isCarrying) {
            if (x == base->x && y == base->y) {
                Game* game = static_cast<Game*>(SDL_GetWindowData(SDL_GetWindowFromID(1), "game"));
                game->minerals += minerals;
                minerals = 0;
                isCarrying = false;
                targetResource = nullptr;
            }
        }
    }
};

// A* Pathfinding
std::vector<Point> findPath(int startX, int startY, int endX, int endY, 
                            const int (&map)[MAP_HEIGHT][MAP_WIDTH], const std::vector<Entity*>& entities) {
    std::vector<Point> path;
    std::map<Point, Point> cameFrom;
    std::map<Point, int> costSoFar;
    auto heuristic = [](Point a, Point b) { return abs(a.x - b.x) + abs(a.y - b.y); };

    Point start = {startX, startY};
    Point goal = {endX, endY};
    std::priority_queue<std::pair<int, Point>, std::vector<std::pair<int, Point>>, std::greater<>> frontier;
    frontier.push({0, start});
    cameFrom[start] = start;
    costSoFar[start] = 0;

    while (!frontier.empty()) {
        Point current = frontier.top().second;
        frontier.pop();

        if (current == goal) break;

        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                if (dx == 0 && dy == 0) continue;
                Point next = {current.x + dx, current.y + dy};
                if (next.x < 0 || next.x >= MAP_WIDTH || next.y < 0 || next.y >= MAP_HEIGHT) continue;

                bool isObstacle = false;
                for (auto* e : entities) {
                    if (e->type == BUILDING && e->x == next.x && e->y == next.y) {
                        isObstacle = true;
                        break;
                    }
                }
                if (isObstacle) continue;

                int newCost = costSoFar[current] + 1;
                if (costSoFar.find(next) == costSoFar.end() || newCost < costSoFar[next]) {
                    costSoFar[next] = newCost;
                    int priority = newCost + heuristic(next, goal);
                    frontier.push({priority, next});
                    cameFrom[next] = current;
                }
            }
        }
    }

    Point current = goal;
    while (!(current == start)) {
        path.push_back(current);
        current = cameFrom[current];
    }
    std::reverse(path.begin(), path.end());
    return path;
}

// AI Controller
class AIController {
public:
    std::vector<Entity*> aiUnits;

    void update(std::vector<Entity*>& entities, const int (&map)[MAP_HEIGHT][MAP_WIDTH]) {
        if (rand() % 100 < 10) {
            if (!aiUnits.empty()) {
                Worker* worker = dynamic_cast<Worker*>(aiUnits[rand() % aiUnits.size()]);
                if (worker && !worker->targetResource) {
                    for (auto* entity : entities) {
                        if (entity->type == RESOURCE && dynamic_cast<Resource*>(entity)->amount > 0) {
                            worker->targetResource = static_cast<Resource*>(entity);
                            worker->moveTo(entity->x, entity->y, map, entities);
                            break;
                        }
                    }
                }
            }
        }
        for (auto* unit : aiUnits) {
            if (rand() % 100 < 5) {
                for (auto* target : entities) {
                    if (target != unit && target->faction != unit->faction && target->type == UNIT) {
                        unit->attack(target);
                    }
                }
            }
        }
    }
};

// Network
class Network {
public:
    TCPsocket server, client;
    SDLNet_SocketSet set;
    bool isServer = false;

    Network() {
        SDLNet_Init();
        set = SDLNet_AllocSocketSet(2);
    }

    void initServer() {
        IPaddress ip;
        SDLNet_ResolveHost(&ip, nullptr, 12345);
        server = SDLNet_TCP_Open(&ip);
        SDLNet_TCP_AddSocket(set, server);
        isServer = true;
    }

    void initClient(const char* host) {
        IPaddress ip;
        SDLNet_ResolveHost(&ip, host, 12345);
        client = SDLNet_TCP_Open(&ip);
        SDLNet_TCP_AddSocket(set, client);
    }

    void sendState(const std::vector<Entity*>& entities) {
        std::string data = "STATE ";
        for (auto* e : entities) {
            data += std::to_string(e->x) + "," + std::to_string(e->y) + "," + 
                    std::to_string(static_cast<int>(e->type)) + "," + 
                    std::to_string(static_cast<int>(e->faction)) + ";";
        }
        if (isServer && client) SDLNet_TCP_Send(client, data.c_str(), data.size() + 1);
        else if (!isServer && server) SDLNet_TCP_Send(server, data.c_str(), data.size() + 1);
    }

    void receiveState(std::vector<Entity*>& entities, SDL_Texture* unitTex, SDL_Texture* resTex, 
                      SDL_Texture* ccTex, SDL_Texture* barTex) {
        char buffer[1024];
        TCPsocket sock = isServer ? server : client;
        if (SDLNet_CheckSockets(set, 0) > 0 && SDLNet_SocketReady(sock)) {
            int received = SDLNet_TCP_Recv(sock, buffer, 1024);
            if (received > 0 && strncmp(buffer, "STATE", 5) == 0) {
                entities.clear();
                char* token = strtok(buffer + 6, ";");
                while (token) {
                    int x, y, type, fac;
                    sscanf(token, "%d,%d,%d,%d", &x, &y, &type, &fac);
                    EntityType eType = static_cast<EntityType>(type);
                    Faction faction = static_cast<Faction>(fac);
                    if (eType == UNIT) entities.push_back(new Entity(UNIT, faction, x, y, unitTex));
                    else if (eType == RESOURCE) entities.push_back(new Resource(x, y, resTex, 1000));
                    else if (eType == BUILDING) {
                        Building* b = new Building(x, y, faction == TERRAN ? ccTex : barTex, faction);
                        if (faction == TERRAN && x != 5) b->produceableUnits = {UNIT}; // Barracks
                        entities.push_back(b);
                    }
                    token = strtok(NULL, ";");
                }
            }
        }
    }

    void acceptConnection() {
        if (isServer && !client) {
            client = SDLNet_TCP_Accept(server);
            if (client) SDLNet_TCP_AddSocket(set, client);
        }
    }

    ~Network() {
        if (server) SDLNet_TCP_Close(server);
        if (client) SDLNet_TCP_Close(client);
        SDLNet_FreeSocketSet(set);
        SDLNet_Quit();
    }
};

// Audio
class Audio {
public:
    Mix_Music* music;
    Mix_Chunk* effect;

    Audio() {
        Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
        music = Mix_LoadMUS("background.mp3");
        effect = Mix_LoadWAV("effect.wav");
        Mix_PlayMusic(music, -1);
    }

    void playEffect() { Mix_PlayChannel(-1, effect, 0); }

    ~Audio() {
        Mix_FreeMusic(music);
        Mix_FreeChunk(effect);
        Mix_CloseAudio();
    }
};

// Game class
class Game {
public:
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font;
    int map[MAP_HEIGHT][MAP_WIDTH];
    std::vector<Entity*> entities;
    std::vector<Entity*> selectedUnits;
    SDL_Texture* terrainTextures[2];
    SDL_Texture* terranUnitTexture, *zergUnitTexture, *protossUnitTexture;
    SDL_Texture* resourceTexture;
    SDL_Texture* terranCCTexture, *terranBarracksTexture;
    SDL_Texture* zergHatcheryTexture, *zergSpawningPoolTexture;
    SDL_Texture* protossNexusTexture, *protossGatewayTexture;
    int minerals = 50;
    AIController ai;
    Network network;
    Audio audio;
    bool isServer = true;

    Game() {
        srand(time(nullptr));
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                map[y][x] = rand() % 2;
            }
        }
        Building* base = new Building(5, 5, nullptr, TERRAN, 200);
        entities.push_back(base);
        entities.push_back(new Resource(10, 10, nullptr, 1000));
        Worker* worker = new Worker(6, 6, nullptr, base, TERRAN);
        entities.push_back(worker);
        Building* barracks = new Building(7, 7, nullptr, TERRAN);
        barracks->produceableUnits = {UNIT};
        barracks->techRequirements[UNIT] = {};
        entities.push_back(barracks);
        Building* zergBase = new Building(15, 15, nullptr, ZERG, 200);
        entities.push_back(zergBase);
        ai.aiUnits.push_back(new Worker(16, 16, nullptr, zergBase, ZERG));
    }

    bool init() {
        if (SDL_Init(SDL_INIT_EVERYTHING) < 0 || IMG_Init(IMG_INIT_PNG) == 0 || TTF_Init() < 0 || Mix_Init(MIX_INIT_MP3) == 0) return false;
        window = SDL_CreateWindow("Starcraft-like", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!window || !renderer) return false;

        font = TTF_OpenFont("font.ttf", 24);
        terrainTextures[GRASS] = IMG_LoadTexture(renderer, "terrain0.png");
        terrainTextures[DIRT] = IMG_LoadTexture(renderer, "terrain1.png");
        terranUnitTexture = IMG_LoadTexture(renderer, "terran_marine.png");
        zergUnitTexture = IMG_LoadTexture(renderer, "zerg_zergling.png");
        protossUnitTexture = IMG_LoadTexture(renderer, "protoss_zealot.png");
        resourceTexture = IMG_LoadTexture(renderer, "minerals.png");
        terranCCTexture = IMG_LoadTexture(renderer, "terran_command_center.png");
        terranBarracksTexture = IMG_LoadTexture(renderer, "terran_barracks.png");
        zergHatcheryTexture = IMG_LoadTexture(renderer, "zerg_hatchery.png");
        zergSpawningPoolTexture = IMG_LoadTexture(renderer, "zerg_spawning_pool.png");
        protossNexusTexture = IMG_LoadTexture(renderer, "protoss_nexus.png");
        protossGatewayTexture = IMG_LoadTexture(renderer, "protoss_gateway.png");

        if (!font || !terrainTextures[GRASS] || !terrainTextures[DIRT] || !terranUnitTexture || 
            !zergUnitTexture || !protossUnitTexture || !resourceTexture || !terranCCTexture || 
            !terranBarracksTexture || !zergHatcheryTexture || !zergSpawningPoolTexture || 
            !protossNexusTexture || !protossGatewayTexture) return false;

        for (auto* entity : entities) {
            if (entity->type == UNIT) entity->texture = (entity->faction == TERRAN ? terranUnitTexture : 
                                                       entity->faction == ZERG ? zergUnitTexture : protossUnitTexture);
            else if (entity->type == RESOURCE) entity->texture = resourceTexture;
            else if (entity->faction == TERRAN && dynamic_cast<Building*>(entity)->produceableUnits.empty()) entity->texture = terranCCTexture;
            else if (entity->faction == TERRAN) entity->texture = terranBarracksTexture;
            else if (entity->faction == ZERG) entity->texture = zergHatcheryTexture;
        }
        for (auto* unit : ai.aiUnits) unit->texture = zergUnitTexture;

        SDL_SetWindowData(window, "game", this);
        if (isServer) network.initServer();
        else network.initClient("localhost");
        return true;
    }

    void handleInput(SDL_Event& event) {
        if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
            int mx = event.button.x / TILE_SIZE;
            int my = event.button.y / TILE_SIZE;
            selectedUnits.clear();
            for (auto* entity : entities) {
                if (entity->type == UNIT && entity->x == mx && entity->y == my) {
                    selectedUnits.push_back(entity);
                    audio.playEffect();
                }
            }
        } else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT) {
            int mx = event.button.x / TILE_SIZE;
            int my = event.button.y / TILE_SIZE;
            for (auto* entity : selectedUnits) {
                Worker* worker = dynamic_cast<Worker*>(entity);
                if (worker) {
                    for (auto* res : entities) {
                        if (res->type == RESOURCE && res->x == mx && res->y == my) {
                            worker->targetResource = static_cast<Resource*>(res);
                            worker->moveTo(mx, my, map, entities);
                            network.sendState(entities);
                            break;
                        }
                    }
                }
            }
        } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_p) {
            for (auto* entity : entities) {
                if (Building* b = dynamic_cast<Building*>(entity); b && !b->produceableUnits.empty()) {
                    b->produceUnit(UNIT, terranUnitTexture, entities, minerals);
                    network.sendState(entities);
                    break;
                }
            }
        }
    }

    void update() {
        network.acceptConnection();
        network.receiveState(entities, terranUnitTexture, resourceTexture, terranCCTexture, terranBarracksTexture);

        for (auto it = entities.begin(); it != entities.end();) {
            (*it)->update();
            if ((*it)->health <= 0) {
                delete *it;
                it = entities.erase(it);
                network.sendState(entities);
            } else ++it;
        }
        ai.update(entities, map);
    }

    void render() {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                SDL_Rect dest = {x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE};
                SDL_RenderCopy(renderer, terrainTextures[map[y][x]], NULL, &dest);
            }
        }

        for (auto* entity : entities) entity->render(renderer);
        for (auto* unit : ai.aiUnits) unit->render(renderer);

        SDL_Color color = {255, 255, 255, 255};
        std::string mineralText = "Minerals: " + std::to_string(minerals);
        SDL_Surface* surface = TTF_RenderText_Solid(font, mineralText.c_str(), color);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect dest = {10, 10, surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, NULL, &dest);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);

        SDL_RenderPresent(renderer);
    }

    void clean() {
        for (auto* entity : entities) delete entity;
        for (auto* unit : ai.aiUnits) delete unit;
        for (int i = 0; i < 2; i++) SDL_DestroyTexture(terrainTextures[i]);
        SDL_DestroyTexture(terranUnitTexture);
        SDL_DestroyTexture(zergUnitTexture);
        SDL_DestroyTexture(protossUnitTexture);
        SDL_DestroyTexture(resourceTexture);
        SDL_DestroyTexture(terranCCTexture);
        SDL_DestroyTexture(terranBarracksTexture);
        SDL_DestroyTexture(zergHatcheryTexture);
        SDL_DestroyTexture(zergSpawningPoolTexture);
        SDL_DestroyTexture(protossNexusTexture);
        SDL_DestroyTexture(protossGatewayTexture);
        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        IMG_Quit();
        Mix_Quit();
        SDL_Quit();
    }
};

int main() {
    Game game;
    if (!game.init()) {
        SDL_Log("Initialization failed: %s", SDL_GetError());
        return 1;
    }

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            game.handleInput(event);
        }
        game.update();
        game.render();
    }

    game.clean();
    return 0;
}
