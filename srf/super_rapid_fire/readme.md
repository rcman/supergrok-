# added

<br>

Key changes:

/    7 Enemy Types with Unique Graphics and Patterns:
        STRAIGHT (enemy1.png): Moves straight down at 100 speed
        ZIGZAG (enemy2.png): Zigzags horizontally at 150 speed
        SINE (enemy3.png): Moves in a sine wave pattern at 120 speed
        CIRCULAR (enemy4.png): Moves in a circular pattern around screen center at 2 rad/s
        DIAGONAL (enemy5.png): Moves diagonally toward center at 130 speed
        FAST (enemy6.png): Moves straight down at 200 speed
        SPIRAL (enemy7.png): Moves in a shrinking spiral pattern at 1.5 rad/s
    Enemy Behavior:
        Each enemy type uses a unique texture (enemy1.png through enemy7.png)
        Enemies spawn from either top-left (-ENEMY_WIDTH) or top-right (VIRTUAL_WIDTH)
        Circular and Spiral patterns center around the screen middle
        Sine and Zigzag use amplitude for horizontal movement width
        Spiral shrinks until radius reaches 10, then disappears

You'll need to provide 7 different enemy ship images (32x32 pixels each) named:

 /   enemy1.png
    enemy2.png
    enemy3.png
    enemy4.png
    enemy5.png
    enemy6.png
    enemy7.png

The movement patterns add variety to the gameplay:

  /  Straight and Fast are simple vertical descents at different speeds
    Zigzag and Sine add horizontal variation
    Circular and Spiral introduce orbital paths
    Diagonal provides angled approaches
