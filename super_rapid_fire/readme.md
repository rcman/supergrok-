# Added

<br>
Key changes and additions:

 /   HUD:
        Added Score (already present), Lives, Level, Hi-Score displays
        Added health bar showing player's health (0-100)
        Added shield timer showing remaining seconds
    Enemy Types:
        Basic: Moves diagonally toward center at 100 speed
        Fast: Moves diagonally toward center at 200 speed
        Zigzag: Moves in a zigzag pattern at 150 speed
        Enemies spawn from top-left or top-right randomly
    Player Movement:
        Added up/down movement with arrow keys
        Retained left/right movement and space bar shooting
    Shield Power-up:
        Added shield power-up type (separate from weapon power-up)
        Draws a thick cyan circle around player when active
        Makes player invulnerable to enemy collisions
        Lasts 60 seconds with countdown timer in HUD
    Game Mechanics:
        Player starts with 3 lives and 100 health
        Enemy collision reduces health by 25
        When health reaches 0, lose a life and reset health
        Level increases every 100 points
        Hi-Score tracks highest achieved score

Additional notes:

/    You'll need three different enemy textures (enemy_basic.png, enemy_fast.png, enemy_zigzag.png)
    Added a shield.png for the shield power-up
    The circle drawing function is basic; you might want to optimize it or use a sprite for better performance
    Power-ups now have a 20% drop chance (up from 10%) with 50/50 chance of being shield or weapon type
