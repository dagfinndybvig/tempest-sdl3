#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define NUM_SOUND_SLOTS 4

typedef struct {
    float phase;
    float freqStart;
    float freqEnd;
    float duration;
    float volume;
    int type;
    bool active;
} SoundEffect;

typedef struct {
    SDL_AudioStream* stream;
    SoundEffect sounds[NUM_SOUND_SLOTS];
    Uint8* wavData;
    Uint32 wavLen;
    Uint32 wavPos;
    bool wavActive;
} AudioContext;

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

typedef enum {
    TUNNEL_IRREGULAR = 0,
    TUNNEL_CIRCLE = 1,
    TUNNEL_SQUARE = 2,
    TUNNEL_FLAT = 3
} TunnelShape;

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    AudioContext audio;
    bool running;
    int playerSegment; 
    float tunnelOffset; 
    Shot shots[MAX_SHOTS];
    Enemy enemies[MAX_ENEMIES];
    Uint64 lastSpawnTime;
    int lives;
    bool gameOver;
    TunnelShape gameOverShape;
    bool superzapperUsed;
    int flashTimer;
    int score;
    TunnelShape tunnelShape;
    TunnelShape selectedTunnelShape;
} AppContext;

typedef struct {
    float x0, y0, x1, y1;
} GlyphLine;

typedef struct {
    char ch;
    const GlyphLine* lines;
    int count;
} Glyph;

static const GlyphLine glyphP[] = {
    {0.0f, 0.0f, 0.0f, 1.0f},
    {0.0f, 1.0f, 0.65f, 1.0f},
    {0.65f, 1.0f, 0.65f, 0.55f},
    {0.65f, 0.55f, 0.0f, 0.55f},
};

static const GlyphLine glyphR[] = {
    {0.0f, 0.0f, 0.0f, 1.0f},
    {0.0f, 1.0f, 0.65f, 1.0f},
    {0.65f, 1.0f, 0.65f, 0.55f},
    {0.65f, 0.55f, 0.0f, 0.55f},
    {0.0f, 0.55f, 1.0f, 0.0f},
};

static const GlyphLine glyphE[] = {
    {0.0f, 0.0f, 0.0f, 1.0f},
    {0.0f, 1.0f, 1.0f, 1.0f},
    {0.0f, 0.5f, 0.75f, 0.5f},
    {0.0f, 0.0f, 1.0f, 0.0f},
};

static const GlyphLine glyphS[] = {
    {0.9f, 1.0f, 0.2f, 1.0f},
    {0.2f, 1.0f, 0.0f, 0.5f},
    {0.0f, 0.5f, 0.65f, 0.5f},
    {0.65f, 0.5f, 1.0f, 0.35f},
    {1.0f, 0.35f, 0.1f, 0.0f},
};

static const GlyphLine glyphA[] = {
    {0.0f, 0.0f, 0.4f, 1.0f},
    {0.4f, 1.0f, 0.8f, 0.0f},
    {0.15f, 0.5f, 0.65f, 0.5f},
};

static const GlyphLine glyphN[] = {
    {0.0f, 0.0f, 0.0f, 1.0f},
    {0.0f, 1.0f, 1.0f, 0.0f},
    {1.0f, 0.0f, 1.0f, 1.0f},
};

static const GlyphLine glyphY[] = {
    {0.0f, 1.0f, 0.5f, 0.5f},
    {1.0f, 1.0f, 0.5f, 0.5f},
    {0.5f, 0.5f, 0.5f, 0.0f},
};

static const GlyphLine glyphK[] = {
    {0.0f, 0.0f, 0.0f, 1.0f},
    {0.0f, 0.5f, 1.0f, 1.0f},
    {0.0f, 0.5f, 1.0f, 0.0f},
};

static const Glyph glyphs[] = {
    {'P', glyphP, (int)(sizeof(glyphP) / sizeof(GlyphLine))},
    {'R', glyphR, (int)(sizeof(glyphR) / sizeof(GlyphLine))},
    {'E', glyphE, (int)(sizeof(glyphE) / sizeof(GlyphLine))},
    {'S', glyphS, (int)(sizeof(glyphS) / sizeof(GlyphLine))},
    {'A', glyphA, (int)(sizeof(glyphA) / sizeof(GlyphLine))},
    {'N', glyphN, (int)(sizeof(glyphN) / sizeof(GlyphLine))},
    {'Y', glyphY, (int)(sizeof(glyphY) / sizeof(GlyphLine))},
    {'K', glyphK, (int)(sizeof(glyphK) / sizeof(GlyphLine))},
};

void ResetGame(AppContext* ctx);
static void ContinueGameWithSelectedGeometry(AppContext* ctx);

static const Glyph* GetGlyphByChar(char ch) {
    ch = (char)toupper((unsigned char)ch);
    for (size_t i = 0; i < sizeof(glyphs) / sizeof(glyphs[0]); i++) {
        if (glyphs[i].ch == ch) return &glyphs[i];
    }
    return NULL;
}

static float MeasureGlyphStringWidth(const char* text, float size, float spacing) {
    float width = 0.0f;
    for (const char* ptr = text; *ptr; ptr++) {
        float charWidth = (*ptr == ' ') ? size * 0.6f : size;
        width += charWidth + spacing;
    }
    if (width > 0.0f) width -= spacing;
    return width;
}

static void DrawGlyphString(SDL_Renderer* renderer, const char* text, float x, float y, float size, float spacing) {
    float cursor = x;
    for (const char* ptr = text; *ptr; ptr++) {
        char ch = *ptr;
        float charWidth = (ch == ' ') ? size * 0.6f : size;
        if (ch != ' ') {
            const Glyph* glyph = GetGlyphByChar(ch);
            if (glyph) {
                for (int i = 0; i < glyph->count; i++) {
                    const GlyphLine* line = &glyph->lines[i];
                    float glyphHeight = size;
                    float x0 = cursor + line->x0 * size;
                    float y0 = y + (1.0f - line->y0) * glyphHeight;
                    float x1 = cursor + line->x1 * size;
                    float y1 = y + (1.0f - line->y1) * glyphHeight;
                    SDL_RenderLine(renderer, x0, y0, x1, y1);
                }
            }
        }
        cursor += charWidth + spacing;
    }
}

static void DrawGameOverPrompt(AppContext* ctx, int w, int h) {
    const char* message = "PRESS ANY KEY";
    float size = 32.0f;
    float spacing = size * 0.25f;
    float totalWidth = MeasureGlyphStringWidth(message, size, spacing);
    float startX = ((float)w - totalWidth) * 0.5f;
    float startY = h * 0.55f;
    SDL_SetRenderDrawColor(ctx->renderer, 255, 255, 255, 220);
    DrawGlyphString(ctx->renderer, message, startX, startY, size, spacing);
}

void PlaySound(AudioContext* ctx, int type, float freqStart, float freqEnd, float duration, float volume);


static float s_noisePhase = 0.0f;

static float NoiseValue(float param, float multiplier, float phaseMul) {
    return sinf(param * multiplier + s_noisePhase * phaseMul);
}

static float StaticJagged(float param) {
    float jitter = sinf(param * 13.17f) * 0.12f + sinf(param * 29.41f) * 0.08f;
    if (jitter > 0.2f) jitter = 0.2f;
    if (jitter < -0.2f) jitter = -0.2f;
    return jitter;
}

static void ApplyTunnelShape(AppContext* ctx, TunnelShape shape) {
    ctx->tunnelShape = shape;
    if (shape == TUNNEL_FLAT) {
        s_noisePhase = ((float)rand() / (float)RAND_MAX) * 2.0f * M_PI;
    }
}

static void SelectTunnelShape(AppContext* ctx, TunnelShape shape) {
    if (shape < TUNNEL_IRREGULAR || shape > TUNNEL_FLAT) {
        shape = TUNNEL_CIRCLE;
    }
    ctx->selectedTunnelShape = shape;
    ApplyTunnelShape(ctx, shape);
}

static void TriggerGameOverShape(AppContext* ctx) {
    if (ctx->gameOver) return;
    ctx->gameOver = true;
    TunnelShape randomShape = (TunnelShape)(rand() % 4);
    ctx->gameOverShape = randomShape;
    ctx->selectedTunnelShape = randomShape;
    ApplyTunnelShape(ctx, randomShape);
    fprintf(stderr, "DEBUG: GameOver triggered, gameOverShape=%d, selectedTunnelShape=%d, tunnelShape=%d\n", ctx->gameOverShape, ctx->selectedTunnelShape, ctx->tunnelShape);
    PlaySound(&ctx->audio, 2, 100.0f, 50.0f, 0.5f, 0.7f);
}

void AudioCallback(void* userdata, SDL_AudioStream* stream, int len, int freq) {
    AudioContext* ctx = (AudioContext*)userdata;
    float* buf = SDL_malloc(len);
    int samples = len / (int)sizeof(float);

    for (int i = 0; i < samples; i++) {
        float sample = 0.0f;

        /* Play WAV only when explicitly activated */
        if (ctx->wavData && ctx->wavLen > 0 && ctx->wavActive) {
            if (ctx->wavPos + 1 < ctx->wavLen) {
                int16_t s16 = (int16_t)((ctx->wavData[ctx->wavPos + 1] << 8) | ctx->wavData[ctx->wavPos]);
                sample += s16 / 32768.0f;
                ctx->wavPos += 2;
            } else {
                /* Reached end of the WAV buffer: stop playback */
                ctx->wavActive = false;
                ctx->wavPos = 0;
            }
        }

        /* Mix in procedural sounds */
        for (int s = 0; s < NUM_SOUND_SLOTS; s++) {
            if (ctx->sounds[s].active) {
                float t = ctx->sounds[s].phase;
                float progress = t / ctx->sounds[s].duration;
                float currentFreq = ctx->sounds[s].freqStart + (ctx->sounds[s].freqEnd - ctx->sounds[s].freqStart) * progress;
                float v = ctx->sounds[s].volume;

                if (ctx->sounds[s].type == 0) {
                    float env = expf(-t * 15.0f);
                    sample += sinf(t * currentFreq * 2.0f * M_PI) * v * env;
                } else if (ctx->sounds[s].type == 1) {
                    float env = expf(-t * 5.0f);
                    float mod = sinf(t * 15.0f * 2.0f * M_PI);
                    sample += (sinf(t * currentFreq * 2.0f * M_PI) + mod * 0.3f) * v * env;
                } else if (ctx->sounds[s].type == 2) {
                    float env = expf(-t * 8.0f);
                    float noise = ((float)(rand() % 100) / 100.0f - 0.5f) * 2.0f;
                    sample += noise * v * env;
                } else if (ctx->sounds[s].type == 3) {
                    float env = 1.0f - progress;
                    float mod = sinf(t * 8.0f * 2.0f * M_PI);
                    sample += (sinf(t * currentFreq * 2.0f * M_PI) + mod * 0.5f) * v * env;
                } else {
                    sample += sinf(t * currentFreq * 2.0f * M_PI) * v;
                }

                ctx->sounds[s].phase += 1.0f / freq;
                if (ctx->sounds[s].phase >= ctx->sounds[s].duration) {
                    ctx->sounds[s].active = false;
                }
            }
        }

        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
        buf[i] = sample;
    }

    SDL_PutAudioStreamData(stream, buf, len);
    SDL_free(buf);
}

void PlaySound(AudioContext* ctx, int type, float freqStart, float freqEnd, float duration, float volume) {
    (void)ctx; (void)type; (void)freqStart; (void)freqEnd; (void)duration; (void)volume;
    /* Procedural sounds disabled: only laserzap.wav (PlayWav) will produce audio. */
    return;
}

void PlayWav(AudioContext* ctx) {
    ctx->wavPos = 0;
    ctx->wavActive = true;
}

void Project(Point3D p, int width, int height, float* sx, float* sy) {
    float z_val = p.z;
    if (z_val < 0.1f) z_val = 0.1f;
    float scale = FOV / z_val;
    *sx = (width / 2.0f) + (p.x * scale);
    *sy = (height / 2.0f) + (p.y * scale);
}

/* Compute XY coordinates on the tunnel rim given a param in [0,1).
   Supports circular, square, flat, and irregular shapes. */
static void TunnelXY(float param, TunnelShape shape, float* outx, float* outy) {
    if (shape == TUNNEL_IRREGULAR) {
        float jitter = StaticJagged(param);
        float irregularParam = param + jitter;
        if (irregularParam < 0.0f) irregularParam += 1.0f;
        if (irregularParam >= 1.0f) irregularParam -= 1.0f;

        float angle = irregularParam * 2.0f * M_PI;
        *outx = cosf(angle) * TUNNEL_RADIUS;
        *outy = sinf(angle) * TUNNEL_RADIUS;
    } else if (shape == TUNNEL_CIRCLE) {
        float angle = param * 2.0f * M_PI;
        *outx = cosf(angle) * TUNNEL_RADIUS;
        *outy = sinf(angle) * TUNNEL_RADIUS;
    } else if (shape == TUNNEL_SQUARE) {
        float scaled = param * 4.0f;
        float base = floorf(scaled);
        int side = ((int)base) % 4;
        float frac = scaled - base;
        float r = TUNNEL_RADIUS;
        switch (side) {
            case 0: *outx = r; *outy = -r + 2.0f * r * frac; break;
            case 1: *outx = r - 2.0f * r * frac; *outy = r; break;
            case 2: *outx = -r; *outy = r - 2.0f * r * frac; break;
            default: *outx = -r + 2.0f * r * frac; *outy = -r; break;
        }
    } else { /* TUNNEL_FLAT */
        float angle = param * 2.0f * M_PI;
        float radiusNoise = NoiseValue(param, 6.5f, 1.1f) * 0.12f;
        float verticalNoise = NoiseValue(param + 0.5f, 5.0f, 1.5f) * 0.35f;
        float asymmetry = NoiseValue(param + 1.3f, 9.0f, 0.7f) * 0.07f;

        float radius = TUNNEL_RADIUS * (1.0f + radiusNoise);
        float xBias = 1.0f + asymmetry;
        float yBias = 0.15f + verticalNoise * 0.4f + asymmetry * 0.2f;
        if (yBias < 0.02f) yBias = 0.02f;

        float tiltTowardsViewer = TUNNEL_RADIUS * 0.3f;
        float yValue = yBias * TUNNEL_RADIUS + tiltTowardsViewer;
        if (yValue < 0.02f * TUNNEL_RADIUS) yValue = 0.02f * TUNNEL_RADIUS;

        *outx = cosf(angle) * radius * xBias;
        *outy = yValue;
    }
}

static bool ShotEnemyHit(AppContext* ctx, const Shot* shot, const Enemy* enemy) {
    if (fabsf(shot->z - enemy->z) >= 1.0f) return false;
    float shotX, shotY, enemyX, enemyY;
    TunnelXY((float)shot->segment / NUM_SIDES, ctx->tunnelShape, &shotX, &shotY);
    TunnelXY((float)enemy->segment / NUM_SIDES, ctx->tunnelShape, &enemyX, &enemyY);
    float dx = shotX - enemyX;
    float dy = shotY - enemyY;
    float threshold = TUNNEL_RADIUS * 0.45f;
    return (dx * dx + dy * dy) <= (threshold * threshold);
}

static bool RestartWithShape(AppContext* ctx, SDL_Scancode scancode, SDL_Keycode key, bool repeat) {
    if (repeat) return false;
    int shape = -1;
    if (scancode == SDL_SCANCODE_0 || key == SDLK_0) shape = TUNNEL_IRREGULAR;
    else if (scancode == SDL_SCANCODE_1 || key == SDLK_1) shape = TUNNEL_CIRCLE;
    else if (scancode == SDL_SCANCODE_2 || key == SDLK_2) shape = TUNNEL_SQUARE;
    else if (scancode == SDL_SCANCODE_3 || key == SDLK_3) shape = TUNNEL_FLAT;

    if (shape < 0) return false;
    SelectTunnelShape(ctx, (TunnelShape)shape);
    ResetGame(ctx);
    return true;
}

void DrawBlaster(AppContext* ctx, int w, int h) {
    float depth = 0.2f;  // How far the "claws" go into the tunnel

    // The blaster is a U-shape defined by 4 points
    Point3D p[4];

    float param1 = (float)ctx->playerSegment / (float)NUM_SIDES;
    float param2 = (float)(ctx->playerSegment + 1) / (float)NUM_SIDES;
    float tx1, ty1, tx2, ty2;
    TunnelXY(param1, ctx->tunnelShape, &tx1, &ty1);
    TunnelXY(param2, ctx->tunnelShape, &tx2, &ty2);

    float scaleInner = (TUNNEL_RADIUS - depth) / TUNNEL_RADIUS;
    // Point 1: Left claw tip (deeper in)
    p[0] = (Point3D){tx1 * scaleInner, ty1 * scaleInner, 2.2f};
    // Point 2: Left base (on rim)
    p[1] = (Point3D){tx1, ty1, 2.0f};
    // Point 3: Right base (on rim)
    p[2] = (Point3D){tx2, ty2, 2.0f};
    // Point 4: Right claw tip (deeper in)
    p[3] = (Point3D){tx2 * scaleInner, ty2 * scaleInner, 2.2f};

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
    ctx->score = 0;
    ctx->gameOver = false;
    ctx->superzapperUsed = false;
    ctx->flashTimer = 0;
    ctx->playerSegment = 0;
    ApplyTunnelShape(ctx, ctx->selectedTunnelShape);
    for(int i=0; i<MAX_SHOTS; i++) ctx->shots[i].active = false;
    for(int i=0; i<MAX_ENEMIES; i++) ctx->enemies[i].active = false;
}

static void ContinueGameWithSelectedGeometry(AppContext* ctx) {
    ctx->lives = 3;
    ctx->score = 0;
    ctx->gameOver = false;
    ctx->superzapperUsed = false;
    ctx->flashTimer = 0;
    ctx->playerSegment = 0;
    fprintf(stderr, "DEBUG: ContinueGameWithSelectedGeometry using gameOverShape=%d\n", ctx->gameOverShape);
    ctx->selectedTunnelShape = ctx->gameOverShape;
    ApplyTunnelShape(ctx, ctx->gameOverShape);
    for(int i=0; i<MAX_SHOTS; i++) ctx->shots[i].active = false;
    for(int i=0; i<MAX_ENEMIES; i++) ctx->enemies[i].active = false;
}

void DrawDigit(SDL_Renderer* renderer, int digit, float x, float y, float size) {
    // Simple segment-based digits
    bool segments[10][7] = {
        {1,1,1,0,1,1,1}, // 0
        {0,0,1,0,0,1,0}, // 1
        {1,0,1,1,1,0,1}, // 2
        {1,0,1,1,0,1,1}, // 3
        {0,1,1,1,0,1,0}, // 4
        {1,1,0,1,0,1,1}, // 5
        {1,1,0,1,1,1,1}, // 6
        {1,0,1,0,0,1,0}, // 7
        {1,1,1,1,1,1,1}, // 8
        {1,1,1,1,0,1,1}  // 9
    };
    // 0: top, 1: top-left, 2: top-right, 3: middle, 4: bottom-left, 5: bottom-right, 6: bottom
    if (segments[digit][0]) SDL_RenderLine(renderer, x, y, x + size, y);
    if (segments[digit][1]) SDL_RenderLine(renderer, x, y, x, y + size);
    if (segments[digit][2]) SDL_RenderLine(renderer, x + size, y, x + size, y + size);
    if (segments[digit][3]) SDL_RenderLine(renderer, x, y + size, x + size, y + size);
    if (segments[digit][4]) SDL_RenderLine(renderer, x, y + size, x, y + 2 * size);
    if (segments[digit][5]) SDL_RenderLine(renderer, x + size, y + size, x + size, y + 2 * size);
    if (segments[digit][6]) SDL_RenderLine(renderer, x, y + 2 * size, x + size, y + 2 * size);
}

void DrawScore(SDL_Renderer* renderer, int score, float x, float y, float size) {
    int temp = score;
    for (int i = 0; i < 6; i++) {
        DrawDigit(renderer, temp % 10, x - i * (size * 1.5f), y, size);
        temp /= 10;
        if (temp == 0 && i > 0) break;
    }
}

void DrawHUD(AppContext* ctx, int w, int h) {
    SDL_SetRenderDrawColor(ctx->renderer, 255, 255, 0, 255);
    for (int i = 0; i < ctx->lives; i++) {
        float x = 20.0f + i * 30.0f;
        float y = 20.0f;
        float size = 10.0f;
        SDL_RenderLine(ctx->renderer, x, y, x, y + size);
        SDL_RenderLine(ctx->renderer, x, y + size, x + size, y + size);
        SDL_RenderLine(ctx->renderer, x + size, y + size, x + size, y);
    }
    
    // Draw Score
    SDL_SetRenderDrawColor(ctx->renderer, 0, 255, 255, 255); // Cyan score
    DrawScore(ctx->renderer, ctx->score, (float)w - 40.0f, 20.0f, 10.0f);
    
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
                fprintf(stderr, "DEBUG: Key pressed during gameOver. scancode=%d, key=%d, repeat=%d\n", event.key.scancode, event.key.key, event.key.repeat);
                // Try to restart with a specific shape (0-3 keys)
                if (RestartWithShape(ctx, event.key.scancode, event.key.key, event.key.repeat)) {
                    fprintf(stderr, "DEBUG: RestartWithShape returned true\n");
                    continue;
                }
                // Any other key: restart with the already-selected random geometry
                fprintf(stderr, "DEBUG: Calling ContinueGameWithSelectedGeometry, selectedTunnelShape=%d\n", ctx->selectedTunnelShape);
                ContinueGameWithSelectedGeometry(ctx);
                fprintf(stderr, "DEBUG: After continue, gameOver=%d, selectedTunnelShape=%d, tunnelShape=%d\n", ctx->gameOver, ctx->selectedTunnelShape, ctx->tunnelShape);
                continue;
            }
            if (event.key.scancode == SDL_SCANCODE_LEFT) ctx->playerSegment = (ctx->playerSegment - 1 + NUM_SIDES) % NUM_SIDES;
            if (event.key.scancode == SDL_SCANCODE_RIGHT) ctx->playerSegment = (ctx->playerSegment + 1) % NUM_SIDES;
            if (!ctx->gameOver) {
                if (event.key.scancode == SDL_SCANCODE_0 || event.key.key == SDLK_0) {
                    SelectTunnelShape(ctx, TUNNEL_IRREGULAR);
                }
                if (event.key.scancode == SDL_SCANCODE_1 || event.key.key == SDLK_1) {
                    SelectTunnelShape(ctx, TUNNEL_CIRCLE);
                }
                if (event.key.scancode == SDL_SCANCODE_2 || event.key.key == SDLK_2) {
                    SelectTunnelShape(ctx, TUNNEL_SQUARE);
                }
                if (event.key.scancode == SDL_SCANCODE_3 || event.key.key == SDLK_3) {
                    SelectTunnelShape(ctx, TUNNEL_FLAT);
                }
            }
            if (event.key.scancode == SDL_SCANCODE_SPACE) {
                for(int i=0; i<MAX_SHOTS; i++) {
                    if(!ctx->shots[i].active) {
                        ctx->shots[i].active = true;
                        ctx->shots[i].z = 2.0f;
                        ctx->shots[i].segment = ctx->playerSegment;
                        PlayWav(&ctx->audio);
                        break;
                    }
                }
            }
            if (event.key.scancode == SDL_SCANCODE_Z && !ctx->superzapperUsed) {
                ctx->superzapperUsed = true;
                ctx->flashTimer = 10;
                for(int i=0; i<MAX_ENEMIES; i++) ctx->enemies[i].active = false;
                PlaySound(&ctx->audio, 1, 100.0f, 800.0f, 0.4f, 0.5f);
            }
        }
    }

    if (!ctx->gameOver) {
        if (ctx->flashTimer > 0) ctx->flashTimer--;
        // Move forward
        ctx->tunnelOffset += 0.02f;
        if (ctx->tunnelOffset >= RING_DISTANCE) ctx->tunnelOffset -= RING_DISTANCE;
        s_noisePhase += 0.04f;
        if (s_noisePhase > 2.0f * M_PI) s_noisePhase -= 2.0f * M_PI;

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
                    ctx->enemies[i].active = false;
                    ctx->lives--;
                    PlaySound(&ctx->audio, 2, 100.0f, 100.0f, 0.15f, 0.5f);
                    if (ctx->lives <= 0) {
                        TriggerGameOverShape(ctx);
                    }
                }

                for (int j = 0; j < MAX_SHOTS; j++) {
                    if (ctx->shots[j].active && ShotEnemyHit(ctx, &ctx->shots[j], &ctx->enemies[i])) {
                        ctx->shots[j].active = false;
                        ctx->enemies[i].active = false;
                        ctx->score += 100;
                        PlaySound(&ctx->audio, 0, 800.0f, 150.0f, 0.12f, 0.35f);
                        break;
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
            float param1 = (float)s / (float)NUM_SIDES;
            float param2 = (float)(s + 1) / (float)NUM_SIDES;

            float tx1, ty1, tx2, ty2;
            TunnelXY(param1, ctx->tunnelShape, &tx1, &ty1);
            TunnelXY(param2, ctx->tunnelShape, &tx2, &ty2);

            Point3D p1 = {tx1, ty1, z};
            Point3D p2 = {tx2, ty2, z};
            Point3D p3 = {tx1, ty1, z + RING_DISTANCE};

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
            float param1 = (float)ctx->enemies[i].segment / (float)NUM_SIDES;
            float param2 = (float)(ctx->enemies[i].segment + 1) / (float)NUM_SIDES;
            float z = ctx->enemies[i].z;
            
            // Draw an "X" in the segment
            Point3D p[4];
            float tx1, ty1, tx2, ty2;
            TunnelXY(param1, ctx->tunnelShape, &tx1, &ty1);
            TunnelXY(param2, ctx->tunnelShape, &tx2, &ty2);
            p[0] = (Point3D){tx1, ty1, z};
            p[1] = (Point3D){tx2, ty2, z + 0.5f};
            p[2] = (Point3D){tx2, ty2, z};
            p[3] = (Point3D){tx1, ty1, z + 0.5f};

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
            float param1 = (float)ctx->shots[i].segment / (float)NUM_SIDES;
            float param2 = (float)(ctx->shots[i].segment + 1) / (float)NUM_SIDES;
            float z_start = ctx->shots[i].z;
            float z_end = z_start + 0.5f;

            float tx1, ty1, tx2, ty2;
            TunnelXY(param1, ctx->tunnelShape, &tx1, &ty1);
            TunnelXY(param2, ctx->tunnelShape, &tx2, &ty2);

            Point3D p1 = {tx1, ty1, z_start};
            Point3D p2 = {tx2, ty2, z_start};
            Point3D p3 = {tx1, ty1, z_end};
            Point3D p4 = {tx2, ty2, z_end};

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
    if (ctx->gameOver) {
        DrawGameOverPrompt(ctx, w, h);
    }

    SDL_RenderPresent(ctx->renderer);
}

int main(int argc, char* argv[]) {
    srand((unsigned int)time(NULL));
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) return 1;
    AppContext ctx = {0};
    ctx.selectedTunnelShape = TUNNEL_CIRCLE;
    ctx.window = SDL_CreateWindow("Tempest SDL3", 800, 800, 0);
    ctx.renderer = SDL_CreateRenderer(ctx.window, NULL);
    
    SDL_AudioSpec wavSpec;
    if (SDL_LoadWAV("laserzap.wav", &wavSpec, &ctx.audio.wavData, &ctx.audio.wavLen)) {
        fprintf(stderr, "Loaded laserzap.wav: %u bytes\n", ctx.audio.wavLen);
    }
    
    SDL_AudioSpec spec = {SDL_AUDIO_F32, 1, 48000};
    ctx.audio.stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, &AudioCallback, &ctx.audio);
    if (ctx.audio.stream) {
        SDL_ResumeAudioStreamDevice(ctx.audio.stream);
    }
    
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
    if (ctx.audio.stream) SDL_DestroyAudioStream(ctx.audio.stream);
    if (ctx.audio.wavData) SDL_free(ctx.audio.wavData);
    SDL_Quit();
    return 0;
}
