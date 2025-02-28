How to Compile and Run

/    Install Dependencies:
        Ubuntu: sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev
        macOS: brew install sdl2 sdl2_image sdl2_ttf
        Windows: Link libraries in your IDE.
    Provide Assets:
        marker.png (8x8 white square), spark.png (8x8 yellow square), font.ttf.
        Place in the executable directory.
    Compile:
    bash

g++ -o qix main.cpp `sdl2-config --cflags --libs` -lSDL2_image -lSDL2_ttf
Run:
bash
./qix
