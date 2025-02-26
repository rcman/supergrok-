Notes

//
    Assets: Ensure all PNGs (60x60), font.ttf, and craft.wav are present in the executable directory. Missing files will halt initialization with an error.
    Performance: The game runs at ~60 FPS with SDL_Delay(16), but survival updates occur every second for consistency.
    Further Enhancements:
        Add animation by cycling sprite frames (e.g., alternate textures every few frames).
        Save/load state with file I/O.
        Expand crafting (e.g., tools from the forge).

This code is tested conceptually and should compile and run with the proper setup. Let me know if you encounter issues or want to add more features!
