#  How to Compile and Run
<br>
/    Install Dependencies:<br>
        Ubuntu: sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev libsdl2-net-dev<br>
        macOS: brew install sdl2 sdl2_image sdl2_ttf sdl2_mixer sdl2_net<br>
        Windows: Link libraries in your IDE.<br>
    Provide Assets:<br>
        Terrain: terrain0.png (grass), terrain1.png (dirt) - 32x32 pixels<br>
        Units: terran_marine.png, zerg_zergling.png, protoss_zealot.png - 32x32 pixels<br>
        Buildings:<br>
            Terran: terran_command_center.png, terran_barracks.png<br>
            Zerg: zerg_hatchery.png, zerg_spawning_pool.png<br>
            Protoss: protoss_nexus.png, protoss_gateway.png - All 32x32 pixels<br>
        Resources: minerals.png - 32x32 pixels<br>
        Audio: font.ttf, background.mp3, effect.wav<br>
    Compile:<br>
    bash<br>


 /   g++ -o starcraft_game main.cpp `sdl2-config --cflags --libs` -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lSDL2_net<br>
    Run:<br>
        Server: ./starcraft_game<br>
        Client: Change isServer = false in Game constructor, run a second instance.<br>
<br>


Enhanced Features from Notes<br>


  /  Scalability:<br>
        Config-Based Entities: The setupEntities function uses a vector<EntityConfig> to define initial entities, making it trivial to add more:<br>
        cpp<br>


   /     configs.push_back({TERRAN, 8, 8, 60, false, false, {}, "terran_tank.png"});<br>
        configs.push_back({ZERG, 17, 17, 200, false, true, {UNIT}, "zerg_spawning_pool.png"});
        Added to Game constructor for easy expansion.
    Optimization:
        Dynamic Spatial Grid: Cell size adjusts to map size (max(mapWidth, mapHeight) / 10), ensuring efficiency for larger maps.
        Quadtree-like Within Cells: Entities within each grid cell are sorted by position, improving collision detection performance.
    Multiplayer:
        Timestamped Commands: Each action (move, produce) includes a timestamp for ordering.
        Interpolation: Unit movement uses linear interpolation between path points, smoothing networked updates.
        Command Queue: Both server and client process commands in order, reducing desync.
        State Sync: Full entity state is sent periodically, with commands for real-time updates.
    Assets:
        Verification: Added error checking for asset loading with console output.
        Complete Integration: All faction-specific assets are loaded and assigned dynamically based on entity configuration.

Gameplay Instructions


/    Select Units: Left-click a unit.
    Harvest: Right-click a mineral patch with a worker (uses A* pathfinding).
    Produce Units: Press P near a barracks (costs 50 minerals).
    Combat: Units auto-attack enemies of different factions.
    Multiplayer: Server (Terran) and client (syncs Zerg AI) share state and commands.
    Sound: Music loops; effects on selection.

This code now fully reflects the scalability, optimization, multiplayer enhancements, and asset integration from the notes, delivering a robust StarCraft-like experience!
