#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define MAX_BULLETS 100
#define NUM_CARS 4
#define MAX_LAPS 3
#define TRACK_WIDTH 50
#define CAR_SIZE 20
#define BULLET_SPEED 10.0f
#define BULLET_SIZE 4
#define FONT_SIZE 24

// Car structure
typedef struct {
    float x, y;        // Position
    float vx, vy;      // Velocity
    float angle;       // Facing angle (degrees)
    int lap;           // Lap count
    int alive;         // 1 if active, 0 if destroyed
} Car;

// Bullet structure
typedef struct {
    float x, y;        // Position
    float vx, vy;      // Velocity
    int active;
} Bullet;

// Track point structure
typedef struct {
    int x, y;
} TrackPoint;

// Game state structure
typedef struct {
    SDL_Renderer* renderer;
    SDL_Texture* track_texture;
    TTF_Font* font;
    Car cars[NUM_CARS];
    Bullet bullets[MAX_BULLETS];
    int bullet_count;
    TrackPoint track_points[5];
    int num_track_points;
    int ai_targets[NUM_CARS - 1];
    int shoot_cooldown;
} Game;

// Custom thick line drawing (simplified)
void draw_thick_line(SDL_Renderer* renderer, int x1, int y1, int x2, int y2, int thickness) {
    for (int i = -thickness / 2; i <= thickness / 2; i++) {
        SDL_RenderDrawLine(renderer, x1 + i, y1, x2 + i, y2);
        SDL_RenderDrawLine(renderer, x1, y1 + i, x2, y2 + i);
    }
}

// Custom filled circle (approximation)
void draw_filled_circle(SDL_Renderer* renderer, int xc, int yc, int r) {
    for (int y = -r; y <= r; y++) {
        for (int x = -r; x <= r; x++) {
            if (x * x + y * y <= r * r) {
                SDL_RenderDrawPoint(renderer, xc + x, yc + y);
            }
        }
    }
}

// Generate a simple dynamic track
void generate_track(TrackPoint* points, int* num_points) {
    *num_points = 5;
    points[0] = (TrackPoint){SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2};      // Center
    points[1] = (TrackPoint){SCREEN_WIDTH - 300, SCREEN_HEIGHT / 2};    // Right
    points[2] = (TrackPoint){SCREEN_WIDTH - 300, SCREEN_HEIGHT - 300};  // Bottom-right
    points[3] = (TrackPoint){300, SCREEN_HEIGHT - 300};                // Bottom-left
    points[4] = (TrackPoint){300, SCREEN_HEIGHT / 2};                  // Left, closes loop
}

// Initialize game state
int init_game(Game* game, SDL_Window** window) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0 || TTF_Init() < 0) {
        fprintf(stderr, "Initialization Error: %s\n", SDL_GetError());
        return 0;
    }

    *window = SDL_CreateWindow("Top-Down Car Racing Game", SDL_WINDOWPOS_CENTERED,
                               SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!*window) {
        fprintf(stderr, "Window Creation Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 0;
    }

    game->renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);
    if (!game->renderer) {
        fprintf(stderr, "Renderer Creation Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(*window);
        SDL_Quit();
        return 0;
    }

    game->font = TTF_OpenFont("arial.ttf", FONT_SIZE);
    if (!game->font) {
        fprintf(stderr, "Font Loading Error: %s\n", TTF_GetError());
        SDL_DestroyRenderer(game->renderer);
        SDL_DestroyWindow(*window);
        SDL_Quit();
        return 0;
    }

    // Create track texture
    game->track_texture = SDL_CreateTexture(game->renderer, SDL_PIXELFORMAT_RGBA8888,
                                            SDL_TEXTUREACCESS_TARGET, SCREEN_WIDTH, SCREEN_HEIGHT);
    SDL_SetRenderTarget(game->renderer, game->track_texture);
    SDL_SetRenderDrawColor(game->renderer, 0, 128, 0, 255); // Grass
    SDL_RenderClear(game->renderer);
    SDL_SetRenderDrawColor(game->renderer, 128, 128, 128, 255); // Road
    generate_track(game->track_points, &game->num_track_points);
    for (int i = 0; i < game->num_track_points - 1; i++) {
        draw_thick_line(game->renderer, game->track_points[i].x, game->track_points[i].y,
                        game->track_points[i + 1].x, game->track_points[i + 1].y, TRACK_WIDTH);
        draw_filled_circle(game->renderer, game->track_points[i].x, game->track_points[i].y, TRACK_WIDTH / 2);
    }
    draw_thick_line(game->renderer, game->track_points[game->num_track_points - 1].x,
                    game->track_points[game->num_track_points - 1].y,
                    game->track_points[0].x, game->track_points[0].y, TRACK_WIDTH);
    draw_filled_circle(game->renderer, game->track_points[game->num_track_points - 1].x,
                       game->track_points[game->num_track_points - 1].y, TRACK_WIDTH / 2);
    SDL_SetRenderTarget(game->renderer, NULL);

    // Initialize cars
    for (int i = 0; i < NUM_CARS; i++) {
        game->cars[i] = (Car){game->track_points[0].x + i * 50, game->track_points[0].y, 0, 0, 0, 0, 1};
    }
    for (int i = 0; i < NUM_CARS - 1; i++) game->ai_targets[i] = 1;
    game->bullet_count = 0;
    game->shoot_cooldown = 0;

    return 1;
}

// Update player car
void update_player(Car* player, const Uint8* keys, int shoot_cooldown) {
    const float max_speed = 5.0f, accel = 0.2f, turn_rate = 3.0f, drag = 0.05f;
    if (player->alive) {
        if (keys[SDL_SCANCODE_W]) {
            player->vx += cos(player->angle * M_PI / 180) * accel;
            player->vy += sin(player->angle * M_PI / 180) * accel;
        }
        if (keys[SDL_SCANCODE_S]) {
            player->vx -= cos(player->angle * M_PI / 180) * accel;
            player->vy -= sin(player->angle * M_PI / 180) * accel;
        }
        if (keys[SDL_SCANCODE_A]) player->angle += turn_rate;
        if (keys[SDL_SCANCODE_D]) player->angle -= turn_rate;

        player->vx *= (1 - drag);
        player->vy *= (1 - drag);
        float speed = sqrt(player->vx * player->vx + player->vy * player->vy);
        if (speed > max_speed) {
            player->vx = (player->vx / speed) * max_speed;
            player->vy = (player->vy / speed) * max_speed;
        }
        player->x += player->vx;
        player->y += player->vy;

        // Boundary checks
        if (player->x < 0) player->x = 0;
        if (player->x > SCREEN_WIDTH) player->x = SCREEN_WIDTH;
        if (player->y < 0) player->y = 0;
        if (player->y > SCREEN_HEIGHT) player->y = SCREEN_HEIGHT;

        // Normalize angle
        if (player->angle >= 360) player->angle -= 360;
        if (player->angle < 0) player->angle += 360;
    }
}

// Update AI car
void update_ai(Car* car, TrackPoint* track_points, int* target_idx, int num_points) {
    const float max_speed = 5.0f;
    if (car->alive) {
        TrackPoint target = track_points[*target_idx];
        float dx = target.x - car->x;
        float dy = target.y - car->y;
        float desired_angle = atan2(dy, dx) * 180 / M_PI;
        float angle_diff = desired_angle - car->angle;
        if (angle_diff > 180) angle_diff -= 360;
        if (angle_diff < -180) angle_diff += 360;
        car->angle += angle_diff * 0.05f; // Smoother turning
        car->vx = cos(car->angle * M_PI / 180) * max_speed * 0.8f;
        car->vy = sin(car->angle * M_PI / 180) * max_speed * 0.8f;
        car->x += car->vx;
        car->y += car->vy;

        if (sqrt(dx * dx + dy * dy) < 20) {
            *target_idx = (*target_idx + 1) % num_points;
        }

        // Boundary checks
        if (car->x < 0) car->x = 0;
        if (car->x > SCREEN_WIDTH) car->x = SCREEN_WIDTH;
        if (car->y < 0) car->y = 0;
        if (car->y > SCREEN_HEIGHT) car->y = SCREEN_HEIGHT;

        // Normalize angle
        if (car->angle >= 360) car->angle -= 360;
        if (car->angle < 0) car->angle += 360;
    }
}

// Update bullets
void update_bullets(Game* game) {
    for (int i = 0; i < game->bullet_count; i++) {
        if (game->bullets[i].active) {
            game->bullets[i].x += game->bullets[i].vx;
            game->bullets[i].y += game->bullets[i].vy;

            // Remove if off-screen
            if (game->bullets[i].x < 0 || game->bullets[i].x > SCREEN_WIDTH ||
                game->bullets[i].y < 0 || game->bullets[i].y > SCREEN_HEIGHT) {
                game->bullets[i] = game->bullets[game->bullet_count - 1];
                game->bullet_count--;
                i--;
                continue;
            }

            // Check collisions with AI cars
            for (int j = 1; j < NUM_CARS; j++) {
                if (game->cars[j].alive &&
                    sqrt(pow(game->bullets[i].x - game->cars[j].x, 2) +
                         pow(game->bullets[i].y - game->cars[j].y, 2)) < CAR_SIZE / 2 + BULLET_SIZE / 2) {
                    game->cars[j].alive = 0;
                    game->bullets[i] = game->bullets[game->bullet_count - 1];
                    game->bullet_count--;
                    i--;
                    break;
                }
            }
        }
    }
}

// Update laps
void update_laps(Car* car, float prev_x) {
    if (car->alive && prev_x < SCREEN_WIDTH / 2 && car->x >= SCREEN_WIDTH / 2 && car->vx > 0) {
        car->lap++;
    }
}

// Calculate player place
int calculate_place(Car* cars) {
    int place = 1;
    for (int i = 1; i < NUM_CARS; i++) {
        if (cars[i].alive && cars[i].lap > cars[0].lap) place++;
    }
    return place;
}

// Render the scene
void render_scene(Game* game) {
    SDL_SetRenderDrawColor(game->renderer, 0, 0, 0, 255);
    SDL_RenderClear(game->renderer);
    SDL_RenderCopy(game->renderer, game->track_texture, NULL, NULL);

    // Draw cars
    for (int i = 0; i < NUM_CARS; i++) {
        if (game->cars[i].alive) {
            SDL_SetRenderDrawColor(game->renderer, i == 0 ? 255 : 0, 0, i == 0 ? 0 : 255, 255);
            SDL_RenderDrawLine(game->renderer, game->cars[i].x, game->cars[i].y,
                               game->cars[i].x + cos(game->cars[i].angle * M_PI / 180) * CAR_SIZE,
                               game->cars[i].y + sin(game->cars[i].angle * M_PI / 180) * CAR_SIZE);
            SDL_Rect car_rect = {(int)game->cars[i].x - CAR_SIZE / 2, (int)game->cars[i].y - CAR_SIZE / 2, CAR_SIZE, CAR_SIZE};
            SDL_RenderDrawRect(game->renderer, &car_rect);
        }
    }

    // Draw bullets
    SDL_SetRenderDrawColor(game->renderer, 255, 255, 0, 255);
    for (int i = 0; i < game->bullet_count; i++) {
        if (game->bullets[i].active) {
            SDL_Rect bullet_rect = {(int)game->bullets[i].x - BULLET_SIZE / 2, (int)game->bullets[i].y - BULLET_SIZE / 2,
                                    BULLET_SIZE, BULLET_SIZE};
            SDL_RenderFillRect(game->renderer, &bullet_rect);
        }
    }

    // Render UI
    char text[50];
    SDL_Color white = {255, 255, 255, 255};
    sprintf(text, "Lap: %d / %d", game->cars[0].lap, MAX_LAPS);
    SDL_Surface* lap_surf = TTF_RenderText_Solid(game->font, text, white);
    SDL_Texture* lap_tex = SDL_CreateTextureFromSurface(game->renderer, lap_surf);
    SDL_Rect lap_rect = {10, 10, lap_surf->w, lap_surf->h};
    SDL_RenderCopy(game->renderer, lap_tex, NULL, &lap_rect);

    sprintf(text, "Place: %d / %d", calculate_place(game->cars), NUM_CARS);
    SDL_Surface* place_surf = TTF_RenderText_Solid(game->font, text, white);
    SDL_Texture* place_tex = SDL_CreateTextureFromSurface(game->renderer, place_surf);
    SDL_Rect place_rect = {10, 40, place_surf->w, place_surf->h};
    SDL_RenderCopy(game->renderer, place_tex, NULL, &place_rect);

    SDL_RenderPresent(game->renderer);

    SDL_FreeSurface(lap_surf);
    SDL_DestroyTexture(lap_tex);
    SDL_FreeSurface(place_surf);
    SDL_DestroyTexture(place_tex);
}

int main(int argc, char* argv[]) {
    Game game = {0};
    SDL_Window* window = NULL;
    if (!init_game(&game, &window)) return 1;

    int running = 1;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = 0;
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE && game.shoot_cooldown <= 0) {
                if (game.bullet_count < MAX_BULLETS) {
                    game.bullets[game.bullet_count] = (Bullet){
                        game.cars[0].x, game.cars[0].y,
                        cos(game.cars[0].angle * M_PI / 180) * BULLET_SPEED,
                        sin(game.cars[0].angle * M_PI / 180) * BULLET_SPEED,
                        1
                    };
                    game.bullet_count++;
                    game.shoot_cooldown = 10;
                }
            }
        }

        const Uint8* keys = SDL_GetKeyboardState(NULL);
        float prev_x[NUM_CARS];
        for (int i = 0; i < NUM_CARS; i++) prev_x[i] = game.cars[i].x;

        update_player(&game.cars[0], keys, game.shoot_cooldown);
        for (int i = 1; i < NUM_CARS; i++) {
            update_ai(&game.cars[i], game.track_points, &game.ai_targets[i - 1], game.num_track_points);
        }
        update_bullets(&game);
        for (int i = 0; i < NUM_CARS; i++) update_laps(&game.cars[i], prev_x[i]);
        if (game.cars[0].lap > MAX_LAPS) running = 0; // End game after player finishes
        if (game.shoot_cooldown > 0) game.shoot_cooldown--;

        render_scene(&game);
        SDL_Delay(16); // ~60 FPS
    }

    SDL_DestroyTexture(game.track_texture);
    TTF_CloseFont(game.font);
    SDL_DestroyRenderer(game.renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
