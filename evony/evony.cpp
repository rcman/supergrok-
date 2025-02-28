#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <string>

// Define building types
enum BuildingType { EMPTY, FARM, BARRACKS };

int main(int argc, char* argv[]) {
    // Initialize SDL, SDL_image, and SDL_ttf
    if (SDL_Init(SDL_INIT_VIDEO) < 0 || !(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) || TTF_Init() < 0) {
        std::cerr << "Initialization failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Create window and renderer
    SDL_Window* window = SDL_CreateWindow("Evony-like Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!window || !renderer) {
        std::cerr << "Window/Renderer creation failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Load font and textures
    TTF_Font* font = TTF_OpenFont("path/to/font.ttf", 24); // Replace with your font path
    SDL_Surface* farmSurface = IMG_Load("path/to/farm.png"); // Replace with your farm image path
    SDL_Texture* farmTexture = SDL_CreateTextureFromSurface(renderer, farmSurface);
    SDL_FreeSurface(farmSurface);
    SDL_Surface* barracksSurface = IMG_Load("path/to/barracks.png"); // Replace with your barracks image path
    SDL_Texture* barracksTexture = SDL_CreateTextureFromSurface(renderer, barracksSurface);
    SDL_FreeSurface(barracksSurface);
    if (!font || !farmTexture || !barracksTexture) {
        std::cerr << "Asset loading failed" << std::endl;
        return 1;
    }

    // Game variables
    bool running = true;
    int resources[4] = {100, 100, 100, 100}; // Food, Wood, Stone, Iron
    int troops = 0;
    const int GRID_SIZE = 5;
    BuildingType cityGrid[GRID_SIZE][GRID_SIZE] = {EMPTY};
    cityGrid[2][2] = FARM; // Initial farm placement
    bool worldMapView = false;
    const int MAP_SIZE = 10;
    int worldMap[MAP_SIZE][MAP_SIZE] = {0};
    worldMap[5][5] = 1; // Resource tile on world map
    int generalLevel = 1;
    bool questActive = false;
    int questProgress = 0;
    BuildingType selectedBuilding = FARM; // Default selected building

    // Main game loop
    while (running) {
        // Check for barracks presence
        bool hasBarracks = false;
        for (int y = 0; y < GRID_SIZE; y++) {
            for (int x = 0; x < GRID_SIZE; x++) {
                if (cityGrid[y][x] == BARRACKS) {
                    hasBarracks = true;
                    goto foundBarracks;
                }
            }
        }
    foundBarracks:

        // Event handling
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_1:
                        selectedBuilding = FARM;
                        break;
                    case SDLK_2:
                        selectedBuilding = BARRACKS;
                        break;
                    case SDLK_t:
                        if (hasBarracks && resources[0] >= 100) {
                            troops += 1;
                            resources[0] -= 100;
                        }
                        break;
                    case SDLK_m:
                        worldMapView = !worldMapView;
                        break;
                    case SDLK_g:
                        if (resources[0] >= 200) {
                            generalLevel += 1;
                            resources[0] -= 200;
                        }
                        break;
                    case SDLK_q:
                        questActive = true;
                        questProgress = 0;
                        break;
                    case SDLK_p:
                        if (questActive) {
                            questProgress += 1;
                            if (questProgress >= 3) {
                                resources[0] += 100;
                                questActive = false;
                            }
                        }
                        break;
                }
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN) {
                if (!worldMapView) { // City view: Place buildings
                    int x = event.button.x / 100;
                    int y = event.button.y / 100;
                    if (x >= 0 && x < GRID_SIZE && y >= 0 && y < GRID_SIZE && cityGrid[y][x] == EMPTY) {
                        if (selectedBuilding == FARM && resources[0] >= 50 && resources[1] >= 50) {
                            cityGrid[y][x] = FARM;
                            resources[0] -= 50; // Food cost
                            resources[1] -= 50; // Wood cost
                        }
                        else if (selectedBuilding == BARRACKS && resources[1] >= 100 && resources[2] >= 50) {
                            cityGrid[y][x] = BARRACKS;
                            resources[1] -= 100; // Wood cost
                            resources[2] -= 50;  // Stone cost
                        }
                    }
                }
                else { // World map view: Gather resources
                    int x = event.button.x / 80;
                    int y = event.button.y / 60;
                    if (x >= 0 && x < MAP_SIZE && y >= 0 && y < MAP_SIZE && worldMap[y][x] == 1 && troops > 0) {
                        resources[0] += 100; // Gain food
                        troops -= 1;         // Lose a troop
                    }
                }
            }
        }

        // Update resources based on farms
        for (int y = 0; y < GRID_SIZE; y++) {
            for (int x = 0; x < GRID_SIZE; x++) {
                if (cityGrid[y][x] == FARM) {
                    resources[0] += 1; // Farms produce 1 food per frame
                }
            }
        }

        // Render
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        if (worldMapView) {
            // Render world map
            for (int y = 0; y < MAP_SIZE; y++) {
                for (int x = 0; x < MAP_SIZE; x++) {
                    SDL_Rect tile = {x * 80, y * 60, 80, 60};
                    SDL_SetRenderDrawColor(renderer,
                                           worldMap[y][x] == 1 ? 0 : 100,   // Red component
                                           worldMap[y][x] == 1 ? 255 : 100, // Green component (green for resources)
                                           100, 255);                       // Blue and alpha
                    SDL_RenderFillRect(renderer, &tile);
                }
            }
        }
        else {
            // Render city grid
            for (int y = 0; y < GRID_SIZE; y++) {
                for (int x = 0; x < GRID_SIZE; x++) {
                    SDL_Rect tile = {x * 100, y * 100, 100, 100};
                    if (cityGrid[y][x] == FARM) {
                        SDL_RenderCopy(renderer, farmTexture, NULL, &tile);
                    }
                    else if (cityGrid[y][x] == BARRACKS) {
                        SDL_RenderCopy(renderer, barracksTexture, NULL, &tile);
                    }
                    else {
                        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255); // Gray for empty tiles
                        SDL_RenderFillRect(renderer, &tile);
                    }
                }
            }
        }

        // Render UI
        SDL_Color white = {255, 255, 255, 255};

        // Resources
        std::string resourceText = "Food: " + std::to_string(resources[0]) + " Wood: " + std::to_string(resources[1]) +
                                   " Stone: " + std::to_string(resources[2]) + " Iron: " + std::to_string(resources[3]);
        SDL_Surface* textSurface = TTF_RenderText_Solid(font, resourceText.c_str(), white);
        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        SDL_Rect textRect = {10, 10, textSurface->w, textSurface->h};
        SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
        SDL_FreeSurface(textSurface);
        SDL_DestroyTexture(textTexture);

        // Troops
        std::string troopText = "Troops: " + std::to_string(troops);
        SDL_Surface* troopSurface = TTF_RenderText_Solid(font, troopText.c_str(), white);
        SDL_Texture* troopTexture = SDL_CreateTextureFromSurface(renderer, troopSurface);
        SDL_Rect troopRect = {10, 40, troopSurface->w, troopSurface->h};
        SDL_RenderCopy(renderer, troopTexture, NULL, &troopRect);
        SDL_FreeSurface(troopSurface);
        SDL_DestroyTexture(troopTexture);

        // General level
        std::string generalText = "General Level: " + std::to_string(generalLevel);
        SDL_Surface* genSurface = TTF_RenderText_Solid(font, generalText.c_str(), white);
        SDL_Texture* genTexture = SDL_CreateTextureFromSurface(renderer, genSurface);
        SDL_Rect genRect = {10, 70, genSurface->w, genSurface->h};
        SDL_RenderCopy(renderer, genTexture, NULL, &genRect);
        SDL_FreeSurface(genSurface);
        SDL_DestroyTexture(genTexture);

        // Quest progress
        if (questActive) {
            std::string questText = "Quest Progress: " + std::to_string(questProgress) + "/3";
            SDL_Surface* qSurface = TTF_RenderText_Solid(font, questText.c_str(), white);
            SDL_Texture* qTexture = SDL_CreateTextureFromSurface(renderer, qSurface);
            SDL_Rect qRect = {10, 100, qSurface->w, qSurface->h};
            SDL_RenderCopy(renderer, qTexture, NULL, &qRect);
            SDL_FreeSurface(qSurface);
            SDL_DestroyTexture(qTexture);
        }

        // Selected building
        std::string selectedText = "Selected: ";
        if (selectedBuilding == FARM) selectedText += "FARM";
        else if (selectedBuilding == BARRACKS) selectedText += "BARRACKS";
        SDL_Surface* selSurface = TTF_RenderText_Solid(font, selectedText.c_str(), white);
        SDL_Texture* selTexture = SDL_CreateTextureFromSurface(renderer, selSurface);
        SDL_Rect selRect = {10, 130, selSurface->w, selSurface->h};
        SDL_RenderCopy(renderer, selTexture, NULL, &selRect);
        SDL_FreeSurface(selSurface);
        SDL_DestroyTexture(selTexture);

        SDL_RenderPresent(renderer);
    }

    // Cleanup
    SDL_DestroyTexture(farmTexture);
    SDL_DestroyTexture(barracksTexture);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();

    return 0;
}
