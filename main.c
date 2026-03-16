#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include <math.h>
#include <stdbool.h>

#define NUM_SIDES 16
#define NUM_RINGS 12
#define RING_DISTANCE 1.5f
#define TUNNEL_RADIUS 2.0f
#define FOV 300.0f

typedef struct {
    float x, y, z;
} Point3D;

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    bool running;
    float playerAngle; 
    float tunnelOffset; 
} AppContext;

void Project(Point3D p, int width, int height, float* sx, float* sy) {
    float z_val = p.z;
    if (z_val < 0.1f) z_val = 0.1f;
    float scale = FOV / z_val;
    *sx = (width / 2.0f) + (p.x * scale);
    *sy = (height / 2.0f) + (p.y * scale);
}

void MainLoop(void* arg) {
    AppContext* ctx = (AppContext*)arg;
    SDL_Event event;
    int w, h;
    SDL_GetWindowSize(ctx->window, &w, &h);

    const bool* state = SDL_GetKeyboardState(NULL);
    if (state[SDL_SCANCODE_LEFT]) ctx->playerAngle -= 0.1f;
    if (state[SDL_SCANCODE_RIGHT]) ctx->playerAngle += 0.1f;

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            ctx->running = false;
#ifdef __EMSCRIPTEN__
            emscripten_cancel_main_loop();
#endif
        }
    }

    // Move "forward" through the tunnel
    ctx->tunnelOffset += 0.02f;
    if (ctx->tunnelOffset >= RING_DISTANCE) ctx->tunnelOffset -= RING_DISTANCE;

    SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 10, 255); // Deep space blue
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

            // Draw connecting lines (depth)
            SDL_SetRenderDrawColor(ctx->renderer, 0, color_val / 2, color_val, 255);
            SDL_RenderLine(ctx->renderer, x1, y1, x3, y3);

            // Draw ring lines
            SDL_SetRenderDrawColor(ctx->renderer, 0, color_val, color_val, 255);
            SDL_RenderLine(ctx->renderer, x1, y1, x2, y2);
        }
    }

    // Draw Blaster (Player)
    float px = cosf(ctx->playerAngle) * TUNNEL_RADIUS;
    float py = sinf(ctx->playerAngle) * TUNNEL_RADIUS;
    float sx, sy;
    Project((Point3D){px, py, 1.0f}, w, h, &sx, &sy);
    
    // Simple yellow blaster
    SDL_SetRenderDrawColor(ctx->renderer, 255, 255, 0, 255);
    SDL_FRect player = {sx - 15, sy - 15, 30, 30};
    SDL_RenderFillRect(ctx->renderer, &player);

    SDL_RenderPresent(ctx->renderer);
}

int main(int argc, char* argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) return 1;
    AppContext ctx = {0};
    ctx.window = SDL_CreateWindow("Tempest SDL3", 800, 600, 0);
    ctx.renderer = SDL_CreateRenderer(ctx.window, NULL);
    ctx.running = true;

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
