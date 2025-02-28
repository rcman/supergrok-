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
#include <queue>
#include <map>
#include <unordered_map>
#include <iostream>

// Constants
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int TILE_SIZE = 32;
const int MAP_WIDTH = 20;
const int MAP_HEIGHT = 15;
const int GRID_CELL_SIZE = 4; // Initial spatial grid cell size

// Enums
enum TerrainType { GRASS, DIRT };
enum ComponentType { POSITION, RENDER, HEALTH, MOVEMENT, WORKER, ATTACK, BUILDING };
enum Faction { TERRAN, ZERG, PROTOSS };

// Components for ECS
struct Point { 
    int x, y; 
    bool operator==(const Point& p) const { return x == p.x && y == p.y; }
    bool operator<(const Point& p) const { return x < p.x || (x == p.x && y < p.y); }
};

struct PositionComponent { int x, y; float interpX, interpY; Uint32 lastUpdate; };
struct RenderComponent { SDL_Texture* texture; };
struct HealthComponent { int health; };
struct MovementComponent { std::vector<Point> path; size_t pathIndex = 0; };
struct WorkerComponent { bool isCarrying = false; int minerals = 0; size_t targetResource = 0; size_t base = 0; };
struct AttackComponent { int damage, range; };
struct BuildingComponent { std::vector<ComponentType> produceableUnits; std::map<ComponentType, std::vector<ComponentType>> techRequirements; };

using EntityID = size_t;

// Command for Multiplayer
struct Command {
    Uint32 timestamp;
    std::string type;
    EntityID id;
    int x, y;
};

// ECS System
class ECS {
public:
    std::unordered_map<EntityID, PositionComponent> positions;
    std::unordered_map<EntityID, RenderComponent> renders;
    std::unordered_map<EntityID, HealthComponent> healths;
    std::unordered_map<EntityID, MovementComponent> movements;
    std::unordered_map<EntityID, WorkerComponent> workers;
    std::unordered_map<EntityID, AttackComponent> attacks;
    std::unordered_map<EntityID, BuildingComponent> buildings;
    std::unordered_map<EntityID, Faction> factions;
    std::vector<EntityID> entities;
    EntityID nextID = 0;

    EntityID createEntity() {
        EntityID id = nextID++;
        entities.push_back(id);
        return id;
    }

    void destroyEntity(EntityID id) {
        positions.erase(id);
        renders.erase(id);
        healths.erase(id);
        movements.erase(id);
        workers.erase(id);
        attacks.erase(id);
        buildings.erase(id);
        factions.erase(id);
        entities.erase(std::remove(entities.begin(), entities.end(), id), entities.end());
    }
};

// Entity Configuration for Scalability
struct EntityConfig {
    Faction faction;
    int x, y;
    int health;
    bool isWorker;
    bool isBuilding;
    std::vector<ComponentType> produceableUnits;
    std::string textureName;
};

// Spatial Grid with Quadtree-like Optimization
class SpatialGrid {
public:
    std::vector<std::vector<std::vector<EntityID>>> grid;
    int cellSize;

    SpatialGrid(int mapWidth, int mapHeight) {
        cellSize = std::max(GRID_CELL_SIZE, std::max(mapWidth, mapHeight) / 10); // Dynamic resizing
        grid.resize(mapHeight / cellSize, std::vector<std::vector<EntityID>>(mapWidth / cellSize));
    }

    void update(const ECS& ecs) {
        for (auto& row : grid) for (auto& cell : row) cell.clear();
        for (auto id : ecs.entities) {
            if (ecs.positions.count(id)) {
                int gx = ecs.positions.at(id).x / cellSize;
                int gy = ecs.positions.at(id).y / cellSize;
                if (gx >= 0 && gx < grid[0].size() && gy >= 0 && gy < grid.size()) {
                    // Quadtree-like: Sort within cell by position for finer collision checks
                    grid[gy][gx].push_back(id);
                    std::sort(grid[gy][gx].begin(), grid[gy][gx].end(), [&](EntityID a, EntityID b) {
                        return ecs.positions[a].x + ecs.positions[a].y * MAP_WIDTH < 
                               ecs.positions[b].x + ecs.positions[b].y * MAP_WIDTH;
                    });
                }
            }
        }
    }

    std::vector<EntityID> getEntitiesAt(int x, int y) {
        int gx = x / cellSize;
        int gy = y / cellSize;
        if (gx >= 0 && gx < grid[0].size() && gy >= 0 && gy < grid.size())
            return grid[gy][gx];
        return {};
    }
};

// A* Pathfinding
std::vector<Point> findPath(int startX, int startY, int endX, int endY, 
                            const int (&map)[MAP_HEIGHT][MAP_WIDTH], const ECS& ecs) {
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
                for (auto id : ecs.entities) {
                    if (ecs.buildings.count(id) && ecs.positions.at(id).x == next.x && ecs.positions.at(id).y == next.y) {
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
    std::vector<EntityID> aiUnits;

    void update(ECS& ecs, const int (&map)[MAP_HEIGHT][MAP_WIDTH], int& minerals) {
        if (rand() % 100 < 10) {
            if (!aiUnits.empty()) {
                EntityID workerID = aiUnits[rand() % aiUnits.size()];
                if (ecs.workers.count(workerID) && !ecs.workers[workerID].isCarrying && ecs.workers[workerID].targetResource == 0) {
                    for (auto id : ecs.entities) {
                        if (!ecs.workers.count(id) && !ecs.buildings.count(id) && ecs.healths[id].health == 100) {
                            ecs.workers[workerID].targetResource = id;
                            ecs.movements[workerID].path = findPath(ecs.positions[workerID].x, ecs.positions[workerID].y, 
                                                                    ecs.positions[id].x, ecs.positions[id].y, map, ecs);
                            break;
                        }
                    }
                }
            }
        }
        for (auto id : aiUnits) {
            if (ecs.attacks.count(id) && rand() % 100 < 5) {
                for (auto target : ecs.entities) {
                    if (ecs.factions[id] != ecs.factions[target] && ecs.attacks.count(target)) {
                        ecs.attacks[id].damage = ecs.factions[id] == PROTOSS ? 8 : 6; // Protoss bonus
                        if (abs(ecs.positions[id].x - ecs.positions[target].x) + 
                            abs(ecs.positions[id].y - ecs.positions[target].y) <= ecs.attacks[id].range) {
                            ecs.healths[target].health -= ecs.attacks[id].damage;
                        }
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
    std::queue<Command> commandQueue;

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

    void sendCommand(const Command& cmd) {
        std::string data = "CMD " + std::to_string(cmd.timestamp) + " " + cmd.type + " " + 
                           std::to_string(cmd.id) + " " + std::to_string(cmd.x) + " " + std::to_string(cmd.y);
        TCPsocket sock = isServer && client ? client : server;
        if (sock) SDLNet_TCP_Send(sock, data.c_str(), data.size() + 1);
    }

    void sendState(const ECS& ecs) {
        std::string data = "STATE " + std::to_string(ecs.entities.size()) + " ";
        for (auto id : ecs.entities) {
            data += std::to_string(id) + "," +
                    std::to_string(ecs.positions[id].x) + "," + 
                    std::to_string(ecs.positions[id].y) + "," + 
                    std::to_string(static_cast<int>(ecs.factions[id])) + "," +
                    (ecs.workers.count(id) ? "W" : ecs.buildings.count(id) ? "B" : "R") + ";";
        }
        TCPsocket sock = isServer && client ? client : server;
        if (sock) SDLNet_TCP_Send(sock, data.c_str(), data.size() + 1);
    }

    void receiveData(ECS& ecs, SDL_Texture* terranUnitTex, SDL_Texture* zergUnitTex, SDL_Texture* protossUnitTex, 
                     SDL_Texture* resTex, SDL_Texture* terranCCTex, SDL_Texture* terranBarracksTex, 
                     SDL_Texture* zergHatcheryTex, SDL_Texture* zergSpawningPoolTex, SDL_Texture* protossNexusTex, 
                     SDL_Texture* protossGatewayTex) {
        char buffer[2048];
        TCPsocket sock = isServer ? server : client;
        if (SDLNet_CheckSockets(set, 0) > 0 && SDLNet_SocketReady(sock)) {
            int received = SDLNet_TCP_Recv(sock, buffer, 2048);
            if (received > 0) {
                if (strncmp(buffer, "STATE", 5) == 0) {
                    int entityCount;
                    sscanf(buffer + 6, "%d", &entityCount);
                    char* token = strtok(buffer + 8, ";");
                    std::map<EntityID, bool> updated;
                    while (token) {
                        EntityID id;
                        int x, y, fac;
                        char type;
                        sscanf(token, "%zu,%d,%d,%d,%c", &id, &x, &y, &fac, &type);
                        Faction faction = static_cast<Faction>(fac);
                        if (!ecs.positions.count(id)) {
                            ecs.createEntity();
                            ecs.entities.back() = id;
                            if (type == 'W') {
                                ecs.workers[id] = WorkerComponent{};
                                ecs.attacks[id] = {faction == PROTOSS ? 8 : 6, 1};
                                ecs.renders[id] = {faction == TERRAN ? terranUnitTex : faction == ZERG ? zergUnitTex : protossUnitTex};
                            } else if (type == 'B') {
                                ecs.buildings[id] = BuildingComponent{};
                                ecs.renders[id] = {faction == TERRAN && x == 5 ? terranCCTex : faction == TERRAN ? terranBarracksTex : 
                                                  faction == ZERG && x == 15 ? zergHatcheryTex : zergSpawningPoolTex};
                            } else {
                                ecs.renders[id] = {resTex};
                            }
                            ecs.healths[id] = {type == 'B' ? 200 : type == 'R' ? 100 : 40};
                        }
                        ecs.positions[id] = {x, y, static_cast<float>(x), static_cast<float>(y), SDL_GetTicks()};
                        ecs.factions[id] = faction;
                        updated[id] = true;
                        token = strtok(NULL, ";");
                    }
                    for (auto it = ecs.entities.begin(); it != ecs.entities.end();) {
                        if (!updated.count(*it)) ecs.destroyEntity(*it);
                        else ++it;
                    }
                } else if (strncmp(buffer, "CMD", 3) == 0) {
                    Command cmd;
                    sscanf(buffer + 4, "%u %s %zu %d %d", &cmd.timestamp, buffer + 20, &cmd.id, &cmd.x, &cmd.y);
                    cmd.type = std::string(buffer + 20, strchr(buffer + 20, ' ') - (buffer + 20));
                    commandQueue.push(cmd);
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
        if (!music || !effect) std::cerr << "Audio load failed: " << Mix_GetError() << std::endl;
        Mix_PlayMusic(music, -1);
    }

    void playEffect() { if (effect) Mix_PlayChannel(-1, effect, 0); }

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
    ECS ecs;
    std::vector<EntityID> selectedUnits;
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
    SpatialGrid spatialGrid;
    bool isServer = true;

    Game() : spatialGrid(MAP_WIDTH, MAP_HEIGHT) {
        srand(time(nullptr));
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                map[y][x] = rand() % 2;
            }
        }
        std::vector<EntityConfig> configs = {
            {TERRAN, 5, 5, 200, false, true, {}, "terran_command_center.png"},
            {TERRAN, 10, 10, 100, false, false, {}, "minerals.png"},
            {TERRAN, 6, 6, 40, true, false, {}, "terran_marine.png"},
            {TERRAN, 7, 7, 200, false, true, {UNIT}, "terran_barracks.png"},
            {ZERG, 15, 15, 200, false, true, {}, "zerg_hatchery.png"},
            {ZERG, 16, 16, 40, true, false, {}, "zerg_zergling.png"}
        };
        setupEntities(configs);
    }

    void setupEntities(const std::vector<EntityConfig>& configs) {
        for (const auto& config : configs) {
            EntityID id = ecs.createEntity();
            ecs.positions[id] = {config.x, config.y, static_cast<float>(config.x), static_cast<float>(config.y), SDL_GetTicks()};
            ecs.healths[id] = {config.health};
            ecs.factions[id] = config.faction;
            ecs.renders[id] = {nullptr};
            if (config.isWorker) {
                ecs.workers[id] = WorkerComponent{.base = 0}; // Base set later
                ecs.attacks[id] = {config.faction == PROTOSS ? 8 : 6, 1};
                ecs.movements[id] = {};
            } else if (config.isBuilding) {
                ecs.buildings[id] = BuildingComponent{.produceableUnits = config.produceableUnits};
            }
            if (config.faction == TERRAN && config.x == 5) {
                for (auto& worker : ecs.workers) worker.second.base = id; // Set base for Terran workers
            } else if (config.faction == ZERG && config.x == 15) {
                for (auto& worker : ecs.workers) {
                    if (ecs.factions[worker.first] == ZERG) worker.second.base = id; // Set base for Zerg workers
                    ai.aiUnits.push_back(worker.first);
                }
            }
        }
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
            !protossNexusTexture || !protossGatewayTexture) {
            std::cerr << "Asset load failed: " << IMG_GetError() << std::endl;
            return false;
        }

        for (auto id : ecs.entities) {
            if (ecs.workers.count(id)) ecs.renders[id].texture = ecs.factions[id] == TERRAN ? terranUnitTexture : 
                                                                 ecs.factions[id] == ZERG ? zergUnitTexture : protossUnitTexture;
            else if (!ecs.buildings.count(id)) ecs.renders[id].texture = resourceTexture;
            else if (ecs.factions[id] == TERRAN && ecs.positions[id].x == 5) ecs.renders[id].texture = terranCCTexture;
            else if (ecs.factions[id] == TERRAN) ecs.renders[id].texture = terranBarracksTexture;
            else if (ecs.factions[id] == ZERG) ecs.renders[id].texture = zergHatcheryTexture;
        }

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
            for (auto id : spatialGrid.getEntitiesAt(mx, my)) {
                if (ecs.workers.count(id) || ecs.attacks.count(id)) {
                    selectedUnits.push_back(id);
                    audio.playEffect();
                }
            }
        } else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT) {
            int mx = event.button.x / TILE_SIZE;
            int my = event.button.y / TILE_SIZE;
            for (auto id : selectedUnits) {
                if (ecs.workers.count(id)) {
                    for (auto res : ecs.entities) {
                        if (!ecs.workers.count(res) && !ecs.buildings.count(res) && ecs.positions[res].x == mx && ecs.positions[res].y == my) {
                            Command cmd{SDL_GetTicks(), "MOVE", id, mx, my};
                            network.sendCommand(cmd);
                            network.commandQueue.push(cmd);
                            break;
                        }
                    }
                }
            }
        } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_p) {
            for (auto id : ecs.entities) {
                if (ecs.buildings.count(id) && !ecs.buildings[id].produceableUnits.empty()) {
                    EntityID newUnit = ecs.createEntity();
                    ecs.positions[newUnit] = {ecs.positions[id].x + 1, ecs.positions[id].y, 
                                             static_cast<float>(ecs.positions[id].x + 1), static_cast<float>(ecs.positions[id].y), SDL_GetTicks()};
                    ecs.healths[newUnit] = {40};
                    ecs.factions[newUnit] = TERRAN;
                    ecs.renders[newUnit] = {terranUnitTexture};
                    ecs.attacks[newUnit] = {6, 1};
                    ecs.movements[newUnit] = {};
                    minerals -= 50;
                    Command cmd{SDL_GetTicks(), "PRODUCE", newUnit, ecs.positions[id].x + 1, ecs.positions[id].y};
                    network.sendCommand(cmd);
                    network.commandQueue.push(cmd);
                    break;
                }
            }
        }
    }

    void update() {
        network.acceptConnection();
        network.receiveData(ecs, terranUnitTexture, zergUnitTexture, protossUnitTexture, resourceTexture, 
                            terranCCTexture, terranBarracksTexture, zergHatcheryTexture, zergSpawningPoolTexture, 
                            protossNexusTexture, protossGatewayTexture);

        spatialGrid.update(ecs);

        while (!network.commandQueue.empty()) {
            Command cmd = network.commandQueue.front();
            network.commandQueue.pop();
            if (cmd.type == "MOVE" && ecs.workers.count(cmd.id)) {
                ecs.workers[cmd.id].targetResource = 0;
                for (auto res : ecs.entities) {
                    if (!ecs.workers.count(res) && !ecs.buildings.count(res) && ecs.positions[res].x == cmd.x && ecs.positions[res].y == cmd.y) {
                        ecs.workers[cmd.id].targetResource = res;
                        break;
                    }
                }
                ecs.movements[cmd.id].path = findPath(ecs.positions[cmd.id].x, ecs.positions[cmd.id].y, cmd.x, cmd.y, map, ecs);
                ecs.movements[cmd.id].pathIndex = 0;
            } else if (cmd.type == "PRODUCE" && !ecs.positions.count(cmd.id)) {
                ecs.createEntity();
                ecs.entities.back() = cmd.id;
                ecs.positions[cmd.id] = {cmd.x, cmd.y, static_cast<float>(cmd.x), static_cast<float>(cmd.y), cmd.timestamp};
                ecs.healths[cmd.id] = {40};
                ecs.factions[cmd.id] = TERRAN;
                ecs.renders[cmd.id] = {terranUnitTexture};
                ecs.attacks[cmd.id] = {6, 1};
                ecs.movements[cmd.id] = {};
            }
        }

        Uint32 now = SDL_GetTicks();
        for (auto id : ecs.entities) {
            if (ecs.movements.count(id) && !ecs.movements[id].path.empty() && ecs.movements[id].pathIndex < ecs.movements[id].path.size()) {
                float t = (now - ecs.positions[id].lastUpdate) / 100.0f; // Interpolation factor
                int nextX = ecs.movements[id].path[ecs.movements[id].pathIndex].x;
                int nextY = ecs.movements[id].path[ecs.movements[id].pathIndex].y;
                ecs.positions[id].interpX = ecs.positions[id].x + (nextX - ecs.positions[id].x) * t;
                ecs.positions[id].interpY = ecs.positions[id].y + (nextY - ecs.positions[id].y) * t;
                if (t >= 1.0f) {
                    ecs.positions[id].x = nextX;
                    ecs.positions[id].y = nextY;
                    ecs.positions[id].interpX = nextX;
                    ecs.positions[id].interpY = nextY;
                    ecs.positions[id].lastUpdate = now;
                    ecs.movements[id].pathIndex++;
                    if (ecs.movements[id].pathIndex >= ecs.movements[id].path.size()) ecs.movements[id].path.clear();
                }
            }
            if (ecs.workers.count(id)) {
                if (ecs.workers[id].targetResource && !ecs.workers[id].isCarrying) {
                    EntityID res = ecs.workers[id].targetResource;
                    if (ecs.positions[id].x == ecs.positions[res].x && ecs.positions[id].y == ecs.positions[res].y && ecs.healths[res].health > 0) {
                        ecs.healths[res].health -= 8;
                        ecs.workers[id].minerals += 8;
                        ecs.workers[id].isCarrying = true;
                    }
                } else if (ecs.workers[id].isCarrying) {
                    EntityID base = ecs.workers[id].base;
                    if (ecs.positions[id].x == ecs.positions[base].x && ecs.positions[id].y == ecs.positions[base].y) {
                        minerals += ecs.workers[id].minerals;
                        ecs.workers[id].minerals = 0;
                        ecs.workers[id].isCarrying = false;
                        ecs.workers[id].targetResource = 0;
                    }
                }
            }
        }

        for (auto it = ecs.entities.begin(); it != ecs.entities.end();) {
            if (ecs.healths[*it].health <= 0) {
                ecs.destroyEntity(*it);
                network.sendState(ecs);
                it = ecs.entities.begin();
            } else ++it;
        }
        ai.update(ecs, map, minerals);
        if (isServer) network.sendState(ecs);
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

        for (auto id : ecs.entities) {
            if (ecs.renders.count(id)) {
                SDL_Rect dest = {static_cast<int>(ecs.positions[id].interpX * TILE_SIZE), 
                                 static_cast<int>(ecs.positions[id].interpY * TILE_SIZE), TILE_SIZE, TILE_SIZE};
                SDL_RenderCopy(renderer, ecs.renders[id].texture, NULL, &dest);
            }
        }

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
