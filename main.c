#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#define NUM_SIDES 16
#define NUM_RINGS 8
#define RING_DISTANCE 1.5f
#define TUNNEL_RADIUS 2.0f
#define FOV 300.0f
#define MAX_SHOTS 10
#define MAX_ENEMIES 5

typedef struct {
    float x, y, z;
} Point3D;

typedef struct {
    float z;
    int segment;
    bool active;
} Shot;

typedef struct {
    float z;
    int segment;
    bool active;
} Enemy;

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    bool running;
    int playerSegment; 
    float tunnelOffset; 
    Shot shots[MAX_SHOTS];
    Enemy enemies[MAX_ENEMIES];
    Uint64 lastSpawnTime;
    int lives;
    bool gameOver;
    bool superzapperUsed;
    int flashTimer;
} AppContext;

void Project(Point3D p, int width, int height, float* sx, float* sy) {
    float z_val = p.z;
    if (z_val < 0.1f) z_val = 0.1f;
    float scale = FOV / z_val;
    *sx = (width / 2.0f) + (p.x * scale);
    *sy = (height / 2.0f) + (p.y * scale);
}

void DrawBlaster(AppContext* ctx, int w, int h) {
    float a1 = (float)ctx->playerSegment * (2.0f * M_PI / NUM_SIDES);
    float a2 = (float)(ctx->playerSegment + 1) * (2.0f * M_PI / NUM_SIDES);
    float depth = 0.2f;  // How far the "claws" go into the tunnel
    
    // The blaster is a U-shape defined by 4 points
    Point3D p[4];
    
    // Point 1: Left claw tip (deeper in)
    p[0] = (Point3D){cosf(a1) * (TUNNEL_RADIUS - depth), sinf(a1) * (TUNNEL_RADIUS - depth), 2.2f};
    // Point 2: Left base (on rim)
    p[1] = (Point3D){cosf(a1) * TUNNEL_RADIUS, sinf(a1) * TUNNEL_RADIUS, 2.0f};
    // Point 3: Right base (on rim)
    p[2] = (Point3D){cosf(a2) * TUNNEL_RADIUS, sinf(a2) * TUNNEL_RADIUS, 2.0f};
    // Point 4: Right claw tip (deeper in)
    p[3] = (Point3D){cosf(a2) * (TUNNEL_RADIUS - depth), sinf(a2) * (TUNNEL_RADIUS - depth), 2.2f};

    float sx[4], sy[4];
    for(int i=0; i<4; i++) Project(p[i], w, h, &sx[i], &sy[i]);

    // Draw with additive blending for "glow"
    SDL_SetRenderDrawBlendMode(ctx->renderer, SDL_BLENDMODE_ADD);
    
    // Outer "faint" glow
    SDL_SetRenderDrawColor(ctx->renderer, 150, 150, 0, 100);
    for(int i=0; i<3; i++) {
        SDL_RenderLine(ctx->renderer, sx[i]-1, sy[i], sx[i+1]-1, sy[i+1]);
        SDL_RenderLine(ctx->renderer, sx[i]+1, sy[i], sx[i+1]+1, sy[i+1]);
    }

    // Inner bright yellow core
    SDL_SetRenderDrawColor(ctx->renderer, 255, 255, 0, 255);
    for(int i=0; i<3; i++) {
        SDL_RenderLine(ctx->renderer, sx[i], sy[i], sx[i+1], sy[i+1]);
    }
    
    SDL_SetRenderDrawBlendMode(ctx->renderer, SDL_BLENDMODE_BLEND);
}

void ResetGame(AppContext* ctx) {
    ctx->lives = 3;
    ctx->gameOver = false;
    ctx->superzapperUsed = false;
    ctx->flashTimer = 0;
    ctx->playerSegment = 0;
    for(int i=0; i<MAX_SHOTS; i++) ctx->shots[i].active = false;
    for(int i=0; i<MAX_ENEMIES; i++) ctx->enemies[i].active = false;
}

void DrawHUD(AppContext* ctx, int w, int h) {
    SDL_SetRenderDrawColor(ctx->renderer, 255, 255, 0, 255);
    for (int i = 0; i < ctx->lives; i++) {
        float x = 20.0f + i * 30.0f;
        float y = 20.0f;
        float size = 10.0f;
        // Draw a small "U" shape
        SDL_RenderLine(ctx->renderer, x, y, x, y + size);
        SDL_RenderLine(ctx->renderer, x, y + size, x + size, y + size);
        SDL_RenderLine(ctx->renderer, x + size, y + size, x + size, y);
    }
    
    // If superzapper is available, draw a small indicator
    if (!ctx->superzapperUsed) {
        SDL_SetRenderDrawColor(ctx->renderer, 255, 255, 255, 255);
        SDL_RenderLine(ctx->renderer, 20, 45, 30, 45);
        SDL_RenderLine(ctx->renderer, 25, 40, 25, 50); // Small "+"
    }
}

void MainLoop(void* arg) {
    AppContext* ctx = (AppContext*)arg;
    SDL_Event event;
    int w, h;
    SDL_GetWindowSize(ctx->window, &w, &h);

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            ctx->running = false;
#ifdef __EMSCRIPTEN__
            emscripten_cancel_main_loop();
#endif
        }
        if (event.type == SDL_EVENT_KEY_DOWN) {
            if (ctx->gameOver) {
                if (event.key.scancode == SDL_SCANCODE_R) ResetGame(ctx);
                continue;
            }
            if (event.key.scancode == SDL_SCANCODE_LEFT) ctx->playerSegment = (ctx->playerSegment - 1 + NUM_SIDES) % NUM_SIDES;
            if (event.key.scancode == SDL_SCANCODE_RIGHT) ctx->playerSegment = (ctx->playerSegment + 1) % NUM_SIDES;
            if (event.key.scancode == SDL_SCANCODE_SPACE) {
                for(int i=0; i<MAX_SHOTS; i++) {
                    if(!ctx->shots[i].active) {
                        ctx->shots[i].active = true;
                        ctx->shots[i].z = 2.0f;
                        ctx->shots[i].segment = ctx->playerSegment;
                        break;
                    }
                }
            }
            if (event.key.scancode == SDL_SCANCODE_Z && !ctx->superzapperUsed) {
                ctx->superzapperUsed = true;
                ctx->flashTimer = 10; // 10 frames of flash
                for(int i=0; i<MAX_ENEMIES; i++) ctx->enemies[i].active = false;
            }
        }
    }

    if (!ctx->gameOver) {
        if (ctx->flashTimer > 0) ctx->flashTimer--;
        // Move forward
        ctx->tunnelOffset += 0.02f;
        if (ctx->tunnelOffset >= RING_DISTANCE) ctx->tunnelOffset -= RING_DISTANCE;

        // Update shots
        for(int i=0; i<MAX_SHOTS; i++) {
            if(ctx->shots[i].active) {
                ctx->shots[i].z += 0.5f;
                if(ctx->shots[i].z > NUM_RINGS * RING_DISTANCE) ctx->shots[i].active = false;
            }
        }

        // Spawn enemies
        Uint64 now = SDL_GetTicks();
        if (now - ctx->lastSpawnTime > 2000) { // Every 2 seconds
            for (int i = 0; i < MAX_ENEMIES; i++) {
                if (!ctx->enemies[i].active) {
                    ctx->enemies[i].active = true;
                    ctx->enemies[i].z = NUM_RINGS * RING_DISTANCE;
                    ctx->enemies[i].segment = rand() % NUM_SIDES;
                    ctx->lastSpawnTime = now;
                    break;
                }
            }
        }

        // Update enemies and collision
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (ctx->enemies[i].active) {
                ctx->enemies[i].z -= 0.05f;
                if (ctx->enemies[i].z < 2.0f) {
                    ctx->enemies[i].active = false; // Reached the rim
                    ctx->lives--;
                    if (ctx->lives <= 0) ctx->gameOver = true;
                }

                // Check collision with shots
                for (int j = 0; j < MAX_SHOTS; j++) {
                    if (ctx->shots[j].active && ctx->shots[j].segment == ctx->enemies[i].segment) {
                        if (fabsf(ctx->shots[j].z - ctx->enemies[i].z) < 1.0f) {
                            ctx->shots[j].active = false;
                            ctx->enemies[i].active = false;
                            break;
                        }
                    }
                }
            }
        }
    }

    if (ctx->gameOver) {
        SDL_SetRenderDrawColor(ctx->renderer, 50, 0, 0, 255); // Dark red background
    } else if (ctx->flashTimer > 0) {
        SDL_SetRenderDrawColor(ctx->renderer, 255, 255, 255, 255); // White flash
    } else {
        SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 10, 255);
    }
    SDL_RenderClear(ctx->renderer);

    // Draw Tunnel
    for (int r = 0; r < NUM_RINGS; r++) {
        float z = (float)(NUM_RINGS - r) * RING_DISTANCE - ctx->tunnelOffset;
        Uint8 color_val = (Uint8)(255 - (r * (255 / NUM_RINGS)));
        
        for (int s = 0; s < NUM_SIDES; s++) {
            float a1 = (float)s * (2.0f * M_PI / NUM_SIDES);
            float a2 = (float)(s + 1) * (2.0f * M_PI / NUM_SIDES);
            
            Point3D p1 = {cosf(a1) * TUNNEL_RADIUS, sinf(a1) * TUNNEL_RADIUS, z};
            Point3D p2 = {cosf(a2) * TUNNEL_RADIUS, sinf(a2) * TUNNEL_RADIUS, z};
            Point3D p3 = {cosf(a1) * TUNNEL_RADIUS, sinf(a1) * TUNNEL_RADIUS, z + RING_DISTANCE};

            float x1, y1, x2, y2, x3, y3;
            Project(p1, w, h, &x1, &y1);
            Project(p2, w, h, &x2, &y2);
            Project(p3, w, h, &x3, &y3);

            SDL_SetRenderDrawColor(ctx->renderer, 0, color_val / 2, color_val, 255);
            SDL_RenderLine(ctx->renderer, x1, y1, x3, y3);

            SDL_SetRenderDrawColor(ctx->renderer, 0, color_val, color_val, 255);
            SDL_RenderLine(ctx->renderer, x1, y1, x2, y2);
        }
    }

    // Draw Enemies
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (ctx->enemies[i].active) {
            float a1 = (float)ctx->enemies[i].segment * (2.0f * M_PI / NUM_SIDES);
            float a2 = (float)(ctx->enemies[i].segment + 1) * (2.0f * M_PI / NUM_SIDES);
            float z = ctx->enemies[i].z;
            
            // Draw an "X" in the segment
            Point3D p[4];
            p[0] = (Point3D){cosf(a1) * TUNNEL_RADIUS, sinf(a1) * TUNNEL_RADIUS, z};
            p[1] = (Point3D){cosf(a2) * TUNNEL_RADIUS, sinf(a2) * TUNNEL_RADIUS, z + 0.5f};
            p[2] = (Point3D){cosf(a2) * TUNNEL_RADIUS, sinf(a2) * TUNNEL_RADIUS, z};
            p[3] = (Point3D){cosf(a1) * TUNNEL_RADIUS, sinf(a1) * TUNNEL_RADIUS, z + 0.5f};

            float sx[4], sy[4];
            for(int k=0; k<4; k++) Project(p[k], w, h, &sx[k], &sy[k]);

            SDL_SetRenderDrawColor(ctx->renderer, 0, 255, 0, 255);
            SDL_RenderLine(ctx->renderer, sx[0], sy[0], sx[1], sy[1]);
            SDL_RenderLine(ctx->renderer, sx[2], sy[2], sx[3], sy[3]);
        }
    }

    // Draw Shots
    SDL_SetRenderDrawBlendMode(ctx->renderer, SDL_BLENDMODE_ADD);
    for (int i = 0; i < MAX_SHOTS; i++) {
        if (ctx->shots[i].active) {
            float a1 = (float)ctx->shots[i].segment * (2.0f * M_PI / NUM_SIDES);
            float a2 = (float)(ctx->shots[i].segment + 1) * (2.0f * M_PI / NUM_SIDES);
            float z_start = ctx->shots[i].z;
            float z_end = z_start + 0.5f;

            Point3D p1 = {cosf(a1) * TUNNEL_RADIUS, sinf(a1) * TUNNEL_RADIUS, z_start};
            Point3D p2 = {cosf(a2) * TUNNEL_RADIUS, sinf(a2) * TUNNEL_RADIUS, z_start};
            Point3D p3 = {cosf(a1) * TUNNEL_RADIUS, sinf(a1) * TUNNEL_RADIUS, z_end};
            Point3D p4 = {cosf(a2) * TUNNEL_RADIUS, sinf(a2) * TUNNEL_RADIUS, z_end};

            float x1, y1, x2, y2, x3, y3, x4, y4;
            Project(p1, w, h, &x1, &y1);
            Project(p2, w, h, &x2, &y2);
            Project(p3, w, h, &x3, &y3);
            Project(p4, w, h, &x4, &y4);

            SDL_SetRenderDrawColor(ctx->renderer, 255, 0, 0, 255);
            SDL_RenderLine(ctx->renderer, x1, y1, x3, y3);
            SDL_RenderLine(ctx->renderer, x2, y2, x4, y4);
        }
    }
    SDL_SetRenderDrawBlendMode(ctx->renderer, SDL_BLENDMODE_BLEND);

    DrawBlaster(ctx, w, h);
    DrawHUD(ctx, w, h);

    SDL_RenderPresent(ctx->renderer);
}

int main(int argc, char* argv[]) {
    srand((unsigned int)time(NULL));
    if (!SDL_Init(SDL_INIT_VIDEO)) return 1;
    AppContext ctx = {0};
    ctx.window = SDL_CreateWindow("Tempest SDL3", 800, 800, 0);
    ctx.renderer = SDL_CreateRenderer(ctx.window, NULL);
    ctx.running = true;
    ResetGame(&ctx);

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(MainLoop, &ctx, 0, 1);
#else
    while (ctx.running) {
        MainLoop(&ctx);
        SDL_Delay(16);
    }
#endif
    SDL_Quit();
    return 0;
}
