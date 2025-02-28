How to Use

/    Building Placement:
        Press 1 to select Farm (costs 50 food, 50 wood) or 2 to select Barracks (costs 100 wood, 50 stone).
        Click an empty tile on the city grid (gray tiles) to place the selected building if you have enough resources.
    Troop Training:
        Build at least one Barracks.
        Press T to train a troop (costs 100 food) if a Barracks exists.
    World Map Interaction:
        Press M to toggle between city view (5x5 grid) and world map view (10x10 grid).
        In world map view, click a green tile (e.g., at position [5][5]) to gather 100 food, consuming 1 troop.
    Other Features:
        Press G to upgrade your general (costs 200 food).
        Press Q to start a quest, then P three times to complete it and earn 100 food.

Notes

 /   Assets: Replace "path/to/font.ttf", "path/to/farm.png", and "path/to/barracks.png" with actual file paths to a font and image files (100x100 pixels recommended for buildings).
    Resource Production: Farms produce 1 food per frame, which may need adjustment (e.g., using a timer) for balanced gameplay in a real application.
    Scalability: This code can be further expanded with additional building types (e.g., Lumber Mill, Quarry), troop types, or combat mechanics by extending the BuildingType enum and adding corresponding logic.

This updated code adds strategic depth to the game while remaining straightforward to compile and run with SDL2, SDL2_image, and SDL2_ttf libraries. Compile it with a command like:
bash
g++ main.cpp -lSDL2 -lSDL2_image -lSDL2_ttf -o game
