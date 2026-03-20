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

typedef enum {
    WAV_LASERZAP = 0,
    WAV_EXPLOSION = 1,
    WAV_PERCUSSION = 2,
    WAV_COIN = 3,
    WAV_SHOTBURST = 4,
    NUM_WAVS = 5
} WavType;

typedef struct {
    float* data;
    Uint32 len;
} WavBuffer;

typedef struct {
    int wavIndex;
    Uint32 pos;
    bool active;
    bool loop;
} ActiveSound;

#define MAX_ACTIVE_SOUNDS 16
#define MAX_HIGHSCORES 5

typedef struct {
    char name[20];
    int score;
} HighScoreEntry;

typedef struct {
    SDL_AudioStream* stream;
    SoundEffect sounds[NUM_SOUND_SLOTS];
    WavBuffer wavs[NUM_WAVS];
    ActiveSound activeSounds[MAX_ACTIVE_SOUNDS];
} AudioContext;

#define NUM_SIDES 16
#define NUM_RINGS 8
#define RING_DISTANCE 1.5f
#define TUNNEL_RADIUS 2.0f
#define FOV 300.0f
#define MAX_SHOTS 10
#define MAX_ENEMIES 5
#define MAX_BURST_SHOTS 6

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
    float z;
    int segment;
    bool active;
} BurstShot;

typedef enum {
    TUNNEL_IRREGULAR = 0,
    TUNNEL_CIRCLE = 1,
    TUNNEL_SQUARE = 2,
    TUNNEL_FLAT = 3
} TunnelShape;

typedef enum {
    STATE_LANDING,
    STATE_PLAYING,
    STATE_GAMEOVER,
    STATE_HIGHSCORE_DISPLAY
} GameState;

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    AudioContext audio;
    bool running;
    GameState state;
    int playerSegment; 
    float tunnelOffset; 
    Shot shots[MAX_SHOTS];
    Enemy enemies[MAX_ENEMIES];
    BurstShot burstShots[MAX_BURST_SHOTS];
    Uint64 nextBurstTime;
    int remainingInBurst;
    Uint64 nextBurstShotTime;
    Uint64 lastSpawnTime;
    int lives;
    bool gameOver; // We can eventually remove this in favor of state, but keeping for now to minimize diff
    TunnelShape gameOverShape;
    bool superzapperUsed;
    int flashTimer;
    int score;
    TunnelShape tunnelShape;
    TunnelShape selectedTunnelShape;
    int enemiesDestroyedCount;
    float gameSpeedMultiplier;
    HighScoreEntry highScores[MAX_HIGHSCORES];
    bool showHighScores;
    char newHighScoreName[20];
    int newHighScorePosition;
    int nameEntryCursorPos;
    // Touch controls (web only)
    bool touchLeftActive;
    bool touchRightActive;
    bool touchFireActive;
    bool touchSuperzapperActive;
    bool showTouchControls;
    bool highscoreEntryPending; // Track if we have a pending highscore entry
    bool touchOnlyMode; // True if running on a touch-only device
    
    // Circular swipe gesture tracking
    float lastTouchX, lastTouchY; // For swipe detection
    bool isSwiping; // Whether we're in a swipe gesture
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

static const GlyphLine glyphG[] = {
    {1.0f, 1.0f, 0.0f, 1.0f},
    {0.0f, 1.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 1.0f, 0.0f},
    {1.0f, 0.0f, 1.0f, 0.5f},
    {1.0f, 0.5f, 0.5f, 0.5f},
};

static const GlyphLine glyphM[] = {
    {0.0f, 0.0f, 0.0f, 1.0f},
    {0.0f, 1.0f, 0.5f, 0.5f},
    {0.5f, 0.5f, 1.0f, 1.0f},
    {1.0f, 1.0f, 1.0f, 0.0f},
};

static const GlyphLine glyphO[] = {
    {0.0f, 0.0f, 0.0f, 1.0f},
    {0.0f, 1.0f, 1.0f, 1.0f},
    {1.0f, 1.0f, 1.0f, 0.0f},
    {1.0f, 0.0f, 0.0f, 0.0f},
};

static const GlyphLine glyphV[] = {
    {0.0f, 1.0f, 0.5f, 0.0f},
    {0.5f, 0.0f, 1.0f, 1.0f},
};

static const GlyphLine glyphU[] = {
    {0.0f, 1.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 1.0f, 0.0f},
    {1.0f, 0.0f, 1.0f, 1.0f},
};

static const GlyphLine glyphC[] = {
    {1.0f, 1.0f, 0.0f, 1.0f},
    {0.0f, 1.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 1.0f, 0.0f},
};

static const GlyphLine glyphT[] = {
    {0.0f, 1.0f, 1.0f, 1.0f},
    {0.5f, 1.0f, 0.5f, 0.0f},
};

static const GlyphLine glyphI[] = {
    {0.0f, 1.0f, 1.0f, 1.0f},
    {0.5f, 1.0f, 0.5f, 0.0f},
    {0.0f, 0.0f, 1.0f, 0.0f},
};

static const GlyphLine glyphColon[] = {
    {0.45f, 0.7f, 0.55f, 0.7f},
    {0.45f, 0.3f, 0.55f, 0.3f},
};

static const GlyphLine glyphD0[] = { {0,0,0,1}, {0,1,1,1}, {1,1,1,0}, {1,0,0,0} };
static const GlyphLine glyphD1[] = { {0.5,0,0.5,1}, {0.2,0.8,0.5,1} };
static const GlyphLine glyphD2[] = { {0,1,1,1}, {1,1,1,0.5}, {1,0.5,0,0.5}, {0,0.5,0,0}, {0,0,1,0} };
static const GlyphLine glyphD3[] = { {0,1,1,1}, {1,1,1,0}, {1,0,0,0}, {0,0.5,1,0.5} };
static const GlyphLine glyphD4[] = { {0,1,0,0.5}, {0,0.5,1,0.5}, {1,1,1,0} };
static const GlyphLine glyphD5[] = { {1,1,0,1}, {0,1,0,0.5}, {0,0.5,1,0.5}, {1,0.5,1,0}, {1,0,0,0} };
static const GlyphLine glyphD6[] = { {1,1,0,1}, {0,1,0,0}, {0,0,1,0}, {1,0,1,0.5}, {1,0.5,0,0.5} };
static const GlyphLine glyphD7[] = { {0,1,1,1}, {1,1,0.4,0} };
static const GlyphLine glyphD8[] = { {0,0,0,1}, {0,1,1,1}, {1,1,1,0}, {1,0,0,0}, {0,0.5,1,0.5} };
static const GlyphLine glyphD9[] = { {1,0,1,1}, {1,1,0,1}, {0,1,0,0.5}, {0,0.5,1,0.5} };

static const Glyph glyphs[] = {
    {'P', glyphP, (int)(sizeof(glyphP) / sizeof(GlyphLine))},
    {'R', glyphR, (int)(sizeof(glyphR) / sizeof(GlyphLine))},
    {'E', glyphE, (int)(sizeof(glyphE) / sizeof(GlyphLine))},
    {'S', glyphS, (int)(sizeof(glyphS) / sizeof(GlyphLine))},
    {'A', glyphA, (int)(sizeof(glyphA) / sizeof(GlyphLine))},
    {'N', glyphN, (int)(sizeof(glyphN) / sizeof(GlyphLine))},
    {'Y', glyphY, (int)(sizeof(glyphY) / sizeof(GlyphLine))},
    {'K', glyphK, (int)(sizeof(glyphK) / sizeof(GlyphLine))},
    {'G', glyphG, (int)(sizeof(glyphG) / sizeof(GlyphLine))},
    {'M', glyphM, (int)(sizeof(glyphM) / sizeof(GlyphLine))},
    {'O', glyphO, (int)(sizeof(glyphO) / sizeof(GlyphLine))},
    {'V', glyphV, (int)(sizeof(glyphV) / sizeof(GlyphLine))},
    {'U', glyphU, (int)(sizeof(glyphU) / sizeof(GlyphLine))},
    {'C', glyphC, (int)(sizeof(glyphC) / sizeof(GlyphLine))},
    {'T', glyphT, (int)(sizeof(glyphT) / sizeof(GlyphLine))},
    {'I', glyphI, (int)(sizeof(glyphI) / sizeof(GlyphLine))},
    {':', glyphColon, (int)(sizeof(glyphColon) / sizeof(GlyphLine))},
    {'0', glyphD0, (int)(sizeof(glyphD0) / sizeof(GlyphLine))},
    {'1', glyphD1, (int)(sizeof(glyphD1) / sizeof(GlyphLine))},
    {'2', glyphD2, (int)(sizeof(glyphD2) / sizeof(GlyphLine))},
    {'3', glyphD3, (int)(sizeof(glyphD3) / sizeof(GlyphLine))},
    {'4', glyphD4, (int)(sizeof(glyphD4) / sizeof(GlyphLine))},
    {'5', glyphD5, (int)(sizeof(glyphD5) / sizeof(GlyphLine))},
    {'6', glyphD6, (int)(sizeof(glyphD6) / sizeof(GlyphLine))},
    {'7', glyphD7, (int)(sizeof(glyphD7) / sizeof(GlyphLine))},
    {'8', glyphD8, (int)(sizeof(glyphD8) / sizeof(GlyphLine))},
    {'9', glyphD9, (int)(sizeof(glyphD9) / sizeof(GlyphLine))},
};

void ResetGame(AppContext* ctx);
static void ContinueGameWithSelectedGeometry(AppContext* ctx);
static void AddHighScore(AppContext* ctx, int score);
static void FinalizeHighScoreEntry(AppContext* ctx);

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
    // Top: GAME OVER
    const char* title = "GAME OVER";
    float titleSize = 64.0f;
    float titleSpacing = titleSize * 0.2f;
    float titleWidth = MeasureGlyphStringWidth(title, titleSize, titleSpacing);
    SDL_SetRenderDrawColor(ctx->renderer, 255, 50, 50, 255);
    DrawGlyphString(ctx->renderer, title, ((float)w - titleWidth) * 0.5f, h * 0.25f, titleSize, titleSpacing);

    // Middle: YOUR SCORE: 12345
    char scoreStr[32];
    SDL_snprintf(scoreStr, sizeof(scoreStr), "YOUR SCORE: %d", ctx->score);
    float scoreSize = 32.0f;
    float scoreSpacing = scoreSize * 0.2f;
    float scoreWidth = MeasureGlyphStringWidth(scoreStr, scoreSize, scoreSpacing);
    SDL_SetRenderDrawColor(ctx->renderer, 57, 255, 20, 255); // Intense Neon Green
    DrawGlyphString(ctx->renderer, scoreStr, ((float)w - scoreWidth) * 0.5f, h * 0.45f, scoreSize, scoreSpacing);

    // Bottom: PRESS UP TO RESTART
    const char* prompt = "PRESS UP TO RESTART";
    float promptSize = 24.0f;
    float promptSpacing = promptSize * 0.25f;
    float promptWidth = MeasureGlyphStringWidth(prompt, promptSize, promptSpacing);
    SDL_SetRenderDrawColor(ctx->renderer, 255, 255, 255, 200);
    DrawGlyphString(ctx->renderer, prompt, ((float)w - promptWidth) * 0.5f, h * 0.65f, promptSize, promptSpacing);

    // Touch controls message (web only) - using printed text like landing page
#ifdef __EMSCRIPTEN__
    SDL_SetRenderScale(ctx->renderer, 1.5f, 1.5f);
    SDL_SetRenderDrawColor(ctx->renderer, 200, 200, 50, 200);
    const char* touchPrompt = "OR TAP TO RESTART WITH TOUCH CONTROLS";
    float tx = ((float)w / 1.5f - (float)strlen(touchPrompt) * 8.0f) * 0.5f;
    float ty = (float)h * 0.72f / 1.5f;
    SDL_RenderDebugText(ctx->renderer, tx, ty, touchPrompt);
    SDL_SetRenderScale(ctx->renderer, 1.0f, 1.0f);
#endif
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

static void DrawLandingPage(AppContext* ctx, int w, int h) {
    // Top: TEMPEST
    const char* title = "TEMPEST";
    float titleSize = 96.0f;
    float titleSpacing = titleSize * 0.2f;
    float titleWidth = MeasureGlyphStringWidth(title, titleSize, titleSpacing);
    SDL_SetRenderDrawColor(ctx->renderer, 255, 50, 50, 255);
    DrawGlyphString(ctx->renderer, title, ((float)w - titleWidth) * 0.5f, h * 0.15f, titleSize, titleSpacing);

    // Controls - Using SDL_RenderDebugText (built-in font)
    const char* controls[] = {
        "ARROWS TO MOVE LEFT/RIGHT",
        "SPACE TO FIRE",
        "Z FOR SUPERZAPPER",
    };
    
    // Touch controls activation message (web only)
    const char* touchMessage = NULL;
#ifdef __EMSCRIPTEN__
    if (ctx->state == STATE_LANDING) {
        touchMessage = "TAP TO START WITH TOUCH CONTROLS";
    } else if (ctx->showTouchControls) {
        touchMessage = "TOUCH CONTROLS ACTIVE";
    }
#endif
    
    // Set a scale to make debug text larger
    float oldScaleX, oldScaleY;
    SDL_GetRenderScale(ctx->renderer, &oldScaleX, &oldScaleY);
    SDL_SetRenderScale(ctx->renderer, 2.5f, 2.5f); // Scale up for "large ascii letters"
    
    SDL_SetRenderDrawColor(ctx->renderer, 57, 255, 20, 255); // Neon Green
    for (int i = 0; i < 3; i++) {  // Changed from 4 to 3 since we removed one control
        // Since we are scaled, we need to divide the coordinates by the scale
        float tx = ((float)w / 2.5f - (float)strlen(controls[i]) * 8.0f) * 0.5f;
        float ty = (float)h * 0.4f / 2.5f + i * 20.0f;
        SDL_RenderDebugText(ctx->renderer, tx, ty, controls[i]);
    }
    
    // Restore original scale
    SDL_SetRenderScale(ctx->renderer, oldScaleX, oldScaleY);

    // Bottom: PRESS UP TO START
    const char* prompt = "PRESS UP TO START";
    float promptSize = 32.0f;
    float promptSpacing = promptSize * 0.25f;
    float promptWidth = MeasureGlyphStringWidth(prompt, promptSize, promptSpacing);
    SDL_SetRenderDrawColor(ctx->renderer, 255, 255, 255, 220);
    DrawGlyphString(ctx->renderer, prompt, ((float)w - promptWidth) * 0.5f, h * 0.82f, promptSize, promptSpacing);

    // Sound is always enabled - no toggle UI needed
    
    // Touch controls activation message (web only)
#ifdef __EMSCRIPTEN__
    if (touchMessage) {
        // Use same scale as other controls for consistent font size
        SDL_SetRenderScale(ctx->renderer, 2.5f, 2.5f);
        SDL_SetRenderDrawColor(ctx->renderer, 255, 50, 50, 220); // Red color
        float tx = ((float)w / 2.5f - (float)strlen(touchMessage) * 8.0f) * 0.5f;
        float ty = (float)h * 0.90f / 2.5f;
        SDL_RenderDebugText(ctx->renderer, tx, ty, touchMessage);
        SDL_SetRenderScale(ctx->renderer, oldScaleX, oldScaleY); // Restore scale
    }
#endif
    
    SDL_SetRenderScale(ctx->renderer, 1.0f, 1.0f);
}

void PlayWav(AudioContext* ctx, WavType type, bool loop) {
    if (type >= NUM_WAVS) return;
    if (ctx->wavs[type].data == NULL) return;

    // Prevent duplicate playback for any sound type (looping or not)
    for (int i = 0; i < MAX_ACTIVE_SOUNDS; i++) {
        if (ctx->activeSounds[i].active && ctx->activeSounds[i].wavIndex == (int)type) {
            return;
        }
    }

    for (int i = 0; i < MAX_ACTIVE_SOUNDS; i++) {
        if (!ctx->activeSounds[i].active) {
            ctx->activeSounds[i].wavIndex = (int)type;
            ctx->activeSounds[i].pos = 0;
            ctx->activeSounds[i].loop = loop;
            ctx->activeSounds[i].active = true;
            return;
        }
    }
}

static void TriggerGameOverShape(AppContext* ctx) {
    if (ctx->state == STATE_GAMEOVER) return;
    ctx->state = STATE_GAMEOVER;
    ctx->gameOver = true;
    TunnelShape randomShape = (TunnelShape)(rand() % 4);
    ctx->gameOverShape = randomShape;
    ctx->selectedTunnelShape = randomShape;
    ApplyTunnelShape(ctx, randomShape);
    fprintf(stderr, "DEBUG: GameOver triggered, gameOverShape=%d, selectedTunnelShape=%d, tunnelShape=%d\n", ctx->gameOverShape, ctx->selectedTunnelShape, ctx->tunnelShape);
    PlayWav(&ctx->audio, WAV_EXPLOSION, false);
    
    // Check if this score qualifies for high scores
    printf("DEBUG: About to check highscore for score: %d\n", ctx->score);
    AddHighScore(ctx, ctx->score);
    printf("DEBUG: After AddHighScore, state = %d (LANDING=%d, PLAYING=%d, GAMEOVER=%d, HIGHSCORE=%d)\n", 
           ctx->state, STATE_LANDING, STATE_PLAYING, STATE_GAMEOVER, STATE_HIGHSCORE_DISPLAY);
}

void AudioCallback(void* userdata, SDL_AudioStream* stream, int len, int total_amount) {
    AudioContext* ctx = (AudioContext*)userdata;
    float buf[2048]; // Enough for 1024 stereo samples
    int remainingBytes = len;

    while (remainingBytes > 0) {
        int bytesToProcess = remainingBytes;
        if (bytesToProcess > (int)sizeof(buf)) bytesToProcess = (int)sizeof(buf);
        int numSamples = bytesToProcess / (int)sizeof(float);
        SDL_memset(buf, 0, bytesToProcess);

        for (int i = 0; i < numSamples; i += 2) {
            float mixedL = 0.0f;
            float mixedR = 0.0f;

            // Mix WAVs
            for (int s = 0; s < MAX_ACTIVE_SOUNDS; s++) {
                ActiveSound* as = &ctx->activeSounds[s];
                if (as->active) {
                    WavBuffer* wb = &ctx->wavs[as->wavIndex];
                    if (wb->data && wb->len > 0) {
                        float vol = 1.0f;
                        // Reduce volume for everything except the background music (WAV_PERCUSSION)
                        if (as->wavIndex != WAV_PERCUSSION) {
                            vol = 0.125f;
                        }
                        mixedL += wb->data[as->pos] * vol;
                        mixedR += wb->data[as->pos + 1] * vol;
                        as->pos += 2;
                        if (as->pos >= wb->len) {
                            if (as->loop) {
                                as->pos = 0;
                            } else {
                                as->active = false;
                            }
                        }
                    }
                }
            }

            // Procedural sounds disabled to avoid "residual sounds"

            if (mixedL > 1.0f) mixedL = 1.0f;
            if (mixedL < -1.0f) mixedL = -1.0f;
            if (mixedR > 1.0f) mixedR = 1.0f;
            if (mixedR < -1.0f) mixedR = -1.0f;
            buf[i] = mixedL;
            buf[i+1] = mixedR;
        }

        SDL_PutAudioStreamData(stream, buf, bytesToProcess);
        remainingBytes -= bytesToProcess;
    }
}

void PlaySound(AudioContext* ctx, int type, float freqStart, float freqEnd, float duration, float volume) {
    (void)ctx; (void)type; (void)freqStart; (void)freqEnd; (void)duration; (void)volume;
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
    // Only reset score if we don't have a pending highscore entry
    if (!ctx->highscoreEntryPending) {
        ctx->score = 0;
    }
    ctx->lives = 3;
    ctx->gameOver = false;
    ctx->superzapperUsed = false;
    ctx->flashTimer = 0;
    ctx->playerSegment = 0;
    ctx->remainingInBurst = 0;
    ctx->nextBurstTime = SDL_GetTicks() + 10000 + (rand() % 20000); // 20s +/- 10s
    ctx->enemiesDestroyedCount = 0;
    ctx->gameSpeedMultiplier = 1.0f;
    ctx->showHighScores = false;
    ctx->nameEntryCursorPos = 0;
    ctx->newHighScorePosition = -1; // Not in name entry mode
    ApplyTunnelShape(ctx, ctx->selectedTunnelShape);
    for(int i=0; i<MAX_SHOTS; i++) ctx->shots[i].active = false;
    for(int i=0; i<MAX_ENEMIES; i++) ctx->enemies[i].active = false;
    for(int i=0; i<MAX_BURST_SHOTS; i++) ctx->burstShots[i].active = false;
    
    // Clear the pending flag when starting a new game (not continuing with highscore)
    ctx->highscoreEntryPending = false;
}

static void ContinueGameWithSelectedGeometry(AppContext* ctx) {
    ctx->lives = 3;
    // Only reset score if we don't have a pending highscore entry
    if (!ctx->highscoreEntryPending) {
        ctx->score = 0;
    }
    ctx->gameOver = false;
    ctx->superzapperUsed = false;
    ctx->flashTimer = 0;
    ctx->playerSegment = 0;
    ctx->enemiesDestroyedCount = 0;
    ctx->gameSpeedMultiplier = 1.0f;
    fprintf(stderr, "DEBUG: ContinueGameWithSelectedGeometry using gameOverShape=%d\n", ctx->gameOverShape);
    ctx->selectedTunnelShape = ctx->gameOverShape;
    ApplyTunnelShape(ctx, ctx->gameOverShape);
    for(int i=0; i<MAX_SHOTS; i++) ctx->shots[i].active = false;
    for(int i=0; i<MAX_ENEMIES; i++) ctx->enemies[i].active = false;
}

static void LoadHighScores(AppContext* ctx) {
#ifdef __EMSCRIPTEN__
    // Web version - use localStorage
    char* json = (char*)EM_ASM_INT({
        return localStorage.getItem('tempestHighScores') || '';
    });
    
    if (json && strlen(json) > 0) {
        // Simple JSON parser for our specific format
        char* ptr = json;
        int scoreIndex = 0;
        
        // Skip opening bracket
        if (*ptr == '[') ptr++;
        
        while (*ptr && scoreIndex < MAX_HIGHSCORES) {
            // Skip whitespace and commas
            while (*ptr && (*ptr == ' ' || *ptr == ',' || *ptr == '\n' || *ptr == '\r')) ptr++;
            
            if (*ptr == '{') {
                ptr++; // Skip opening brace
                
                // Extract name
                char* nameStart = strstr(ptr, "\"name\":");
                if (nameStart) {
                    nameStart += 8; // Skip "name":"
                    char* nameEnd = strchr(nameStart, '"');
                    if (nameEnd) {
                        strncpy(ctx->highScores[scoreIndex].name, nameStart, nameEnd - nameStart);
                        ctx->highScores[scoreIndex].name[nameEnd - nameStart] = '\0';
                        ptr = nameEnd + 1;
                    }
                }
                
                // Extract score
                char* scoreStart = strstr(ptr, "\"score\":");
                if (scoreStart) {
                    scoreStart += 8; // Skip "score":
                    ctx->highScores[scoreIndex].score = atoi(scoreStart);
                    ptr = scoreStart;
                    while (*ptr && *ptr != '}' && *ptr != ',') ptr++;
                }
                
                scoreIndex++;
            } else {
                break;
            }
        }
        free(json);
    } else {
        // Fallback to default scores
        for (int i = 0; i < MAX_HIGHSCORES; i++) {
            sprintf(ctx->highScores[i].name, "PROSPERO");
            ctx->highScores[i].score = 500 - (i * 100);
        }
    }
#else
    // Native version - use file I/O
    FILE* f = fopen("highscores.txt", "rb");
    if (f) {
        fread(ctx->highScores, sizeof(HighScoreEntry), MAX_HIGHSCORES, f);
        fclose(f);
    } else {
        // Initialize with default high scores (top 5)
        for (int i = 0; i < MAX_HIGHSCORES; i++) {
            sprintf(ctx->highScores[i].name, "PROSPERO");
            ctx->highScores[i].score = 500 - (i * 100);
        }
    }
#endif
}

static void SaveHighScores(AppContext* ctx) {
#ifdef __EMSCRIPTEN__
    // Web version - use localStorage with JSON serialization
    char json[512];
    sprintf(json, "[");
    for (int i = 0; i < MAX_HIGHSCORES; i++) {
        if (i > 0) strcat(json, ",");
        char entry[128];
        sprintf(entry, "{\"name\":\"%s\",\"score\":%d}", ctx->highScores[i].name, ctx->highScores[i].score);
        strcat(json, entry);
    }
    strcat(json, "]");
    
    EM_ASM_({
        localStorage.setItem('tempestHighScores', UTF8ToString($0));
    }, json);
#else
    // Native version - use file I/O
    FILE* f = fopen("highscores.txt", "wb");
    if (f) {
        fwrite(ctx->highScores, sizeof(HighScoreEntry), MAX_HIGHSCORES, f);
        fclose(f);
    }
#endif
}

static void AddHighScore(AppContext* ctx, int score) {
    printf("DEBUG: AddHighScore called with score: %d\n", score);
    
    // Check if we have a pending highscore entry from before
    if (ctx->highscoreEntryPending) {
        printf("DEBUG: Resuming pending highscore entry\n");
        ctx->state = STATE_HIGHSCORE_DISPLAY;
        return;
    }
    
    // Check if score qualifies for high score table
    for (int i = 0; i < MAX_HIGHSCORES; i++) {
        printf("DEBUG: Comparing with highscore %d: %d\n", i, ctx->highScores[i].score);
        if (score > ctx->highScores[i].score) {
            printf("DEBUG: New highscore! Position %d\n", i);
            // Shift lower scores down
            for (int j = MAX_HIGHSCORES - 1; j > i; j--) {
                ctx->highScores[j] = ctx->highScores[j - 1];
            }
            // Set up for name entry (now integrated into highscore display)
            ctx->newHighScorePosition = i;
            ctx->nameEntryCursorPos = 0;
            memset(ctx->newHighScoreName, 0, sizeof(ctx->newHighScoreName));
#ifdef __EMSCRIPTEN__
            // Auto-set to "PLAYER" for touch devices
            sprintf(ctx->newHighScoreName, "PLAYER");
            ctx->nameEntryCursorPos = 6;
#else
            // Start with "AAA" for keyboard editing
            sprintf(ctx->newHighScoreName, "AAA");
#endif
            ctx->state = STATE_HIGHSCORE_DISPLAY;
            return;
        }
    }
    printf("DEBUG: No highscore achieved\n");
    // If no high score, go directly to game over
    ctx->highscoreEntryPending = false;
    ctx->state = STATE_GAMEOVER;
}

static void FinalizeHighScoreEntry(AppContext* ctx) {
    // Add the finalized score to the high score table
    strcpy(ctx->highScores[ctx->newHighScorePosition].name, ctx->newHighScoreName);
    ctx->highScores[ctx->newHighScorePosition].score = ctx->score;
    
    // Mark that we have a pending highscore entry (for if player gets another highscore)
    ctx->highscoreEntryPending = true;
    
    // Save high scores to file
    SaveHighScores(ctx);
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

    // Sound is always enabled - no toggle UI needed

    // Draw touch controls for web version
#ifdef __EMSCRIPTEN__
    if (ctx->showTouchControls) {
        // Draw circular swipe area indicator
        SDL_SetRenderDrawColor(ctx->renderer, 0, 100, 200, 80);
        float centerX = w * 0.5f;
        float centerY = h * 0.5f;
        float radius = fmin(w, h) * 0.4f;
        
        // Draw outer circle
        for (float angle = 0; angle < 2 * M_PI; angle += 0.1f) {
            float x1 = centerX + radius * cosf(angle);
            float y1 = centerY + radius * sinf(angle);
            float x2 = centerX + radius * cosf(angle + 0.1f);
            float y2 = centerY + radius * sinf(angle + 0.1f);
            SDL_RenderLine(ctx->renderer, x1, y1, x2, y2);
        }
        
        // Draw inner circle (fire zone)
        SDL_SetRenderDrawColor(ctx->renderer, 200, 0, 0, 120);
        float innerRadius = radius * 0.3f;
        for (float angle = 0; angle < 2 * M_PI; angle += 0.1f) {
            float x1 = centerX + innerRadius * cosf(angle);
            float y1 = centerY + innerRadius * sinf(angle);
            float x2 = centerX + innerRadius * cosf(angle + 0.1f);
            float y2 = centerY + innerRadius * sinf(angle + 0.1f);
            SDL_RenderLine(ctx->renderer, x1, y1, x2, y2);
        }
        
        // Superzapper button (keep in corner for easy access)
        SDL_SetRenderDrawColor(ctx->renderer, 0, 200, 0, 150);
        SDL_FRect superZone = {w * 0.7f, h * 0.8f, w * 0.3f, h * 0.2f};
        SDL_RenderFillRect(ctx->renderer, &superZone);
        
        // Draw labels
        SDL_SetRenderDrawColor(ctx->renderer, 255, 255, 255, 255);
        SDL_RenderDebugText(ctx->renderer, centerX - 50.0f, centerY - 10.0f, "SWIPE TO ROTATE");
        SDL_RenderDebugText(ctx->renderer, centerX - 30.0f, centerY + 10.0f, "TAP TO FIRE");
        SDL_RenderDebugText(ctx->renderer, w * 0.85f - 20.0f, h * 0.85f, "ZAP");
        
        // Draw swipe direction arrows
        SDL_RenderDebugText(ctx->renderer, centerX - radius - 40.0f, centerY - 20.0f, "CCW");
        SDL_RenderDebugText(ctx->renderer, centerX + radius + 10.0f, centerY - 20.0f, "CW");
    }
#endif
}



static void DrawHighScoreDisplayScreen(AppContext* ctx, int w, int h) {
    // Clear screen
    SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, 255);
    SDL_RenderClear(ctx->renderer);
    
    // Use same style as landing page: neon green, scaled up text
    SDL_SetRenderDrawColor(ctx->renderer, 57, 255, 20, 255); // Neon Green
    
    // Set scale for larger text (same as landing page)
    float oldScaleX, oldScaleY;
    SDL_GetRenderScale(ctx->renderer, &oldScaleX, &oldScaleY);
    SDL_SetRenderScale(ctx->renderer, 2.5f, 2.5f);
    
    // Center high score table on screen
    float tableX = ((float)w / 2.5f - 200.0f) / 2.0f; // Center horizontally
    float tableY = 10.0f; // Small top margin
    
    // Border removed to allow full window usage for high scores
    
    // Draw title in glowing red (same as TEMPEST title)
    SDL_SetRenderDrawColor(ctx->renderer, 255, 50, 50, 255); // Glowing red
    float titleX = tableX + (200.0f - (float)strlen("HIGH SCORE TABLE") * 8.0f) / 2.0f;
    SDL_RenderDebugText(ctx->renderer, titleX, tableY + 12.0f, "HIGH SCORE TABLE");
    
    // Restore neon green for scores
    SDL_SetRenderDrawColor(ctx->renderer, 57, 255, 20, 255);
    
    // Draw high scores (score before name, no numbering)
    // Add two line breaks after title (increase initial Y position)
    bool isEditing = false;
    for (int i = 0; i < MAX_HIGHSCORES; i++) {
        float yPos = tableY + 48.0f + i * 10.0f; // Increased from 28.0f to 48.0f for spacing
        char scoreText[50];
        
        // Format score with fixed width (6 digits) and name
        if (i == ctx->newHighScorePosition) {
            isEditing = true;
            printf("Displaying new highscore name: '%s'\n", ctx->newHighScoreName);
            sprintf(scoreText, "%6d %s", ctx->score, ctx->newHighScoreName);
        } else {
            sprintf(scoreText, "%6d %s", ctx->highScores[i].score, ctx->highScores[i].name);
        }
        // Draw at fixed position (scores right-aligned within 6-digit field, names left-aligned)
        SDL_RenderDebugText(ctx->renderer, tableX + 20.0f, yPos, scoreText); // Moved 10 pixels right
    }
    
    // Draw name entry cursor if editing
    if (isEditing) {
        float yPos = tableY + 48.0f + ctx->newHighScorePosition * 10.0f; // Match score Y position
        // Cursor position: fixed X + 6-digit score width + cursor position in name
        // 6 digits * 8 pixels = 48 pixels for score, plus space = 56 pixels total
        float cursorX = tableX + 20.0f + 48.0f + 8.0f + (ctx->nameEntryCursorPos * 8.0f); // Moved 10 pixels right
        
        // Draw blinking cursor (white for visibility)
        SDL_SetRenderDrawColor(ctx->renderer, 255, 255, 255, 255);
        if ((SDL_GetTicks() / 500) % 2 == 0) {
            SDL_RenderLine(ctx->renderer, cursorX, yPos, cursorX, yPos + 4.0f); // Scaled cursor height
        }
        SDL_SetRenderDrawColor(ctx->renderer, 57, 255, 20, 255); // Restore neon green
    }
    
    // Draw instructions (simplified - only show when not editing)
    if (!isEditing) {
        float instrX = tableX + (200.0f - (float)strlen("PRESS R TO CONTINUE") * 8.0f) / 2.0f;
        SDL_RenderDebugText(ctx->renderer, instrX, tableY + 150.0f, "PRESS R TO CONTINUE");
        
        // Add tap instruction for web version
#ifdef __EMSCRIPTEN__
        float tapInstrX = tableX + (200.0f - (float)strlen("OR TAP TO CONTINUE") * 8.0f) / 2.0f;
        SDL_RenderDebugText(ctx->renderer, tapInstrX, tableY + 160.0f, "OR TAP TO CONTINUE");
#endif
    } else {
        // Draw simple name selection for touch devices (web only)
#ifdef __EMSCRIPTEN__
        SDL_SetRenderScale(ctx->renderer, 1.0f, 1.0f);
        SDL_SetRenderDrawColor(ctx->renderer, 50, 50, 100, 180);
        
        // Draw selection background
        SDL_FRect selectBg = {w * 0.3f, h * 0.6f, w * 0.4f, h * 0.2f};
        SDL_RenderFillRect(ctx->renderer, &selectBg);
        
        // Different UI based on input method
#ifndef __EMSCRIPTEN__
        // Keyboard mode: Show editing instructions
        SDL_SetRenderDrawColor(ctx->renderer, 255, 255, 255, 220);
        const char* instr = "TYPE NAME, PRESS ENTER TO FINISH";
        SDL_RenderDebugText(ctx->renderer, (w - strlen(instr) * 8) * 0.5f, h * 0.55f, instr);
        
        // Show confirmation for keyboard users
        SDL_SetRenderDrawColor(ctx->renderer, 150, 200, 150, 220);
        SDL_FRect confirmBtn = {w * 0.4f, h * 0.65f, w * 0.2f, h * 0.1f};
        SDL_RenderFillRect(ctx->renderer, &confirmBtn);
        SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, 255);
        const char* confirmText = "PRESS R TO CONTINUE";
        float confirmTextX = confirmBtn.x + (confirmBtn.w - strlen(confirmText) * 8) * 0.5f;
        float confirmTextY = confirmBtn.y + (confirmBtn.h - 8) * 0.5f;
        SDL_RenderDebugText(ctx->renderer, confirmTextX, confirmTextY, confirmText);
#else
        // Touch mode: Automatically use "PLAYER" with touch confirmation
        // Set name based on input mode (only if not already set)
        static bool nameInitialized = false;
        if (!nameInitialized && (ctx->newHighScoreName[0] == '\0' || ctx->newHighScoreName[0] == ' ')) {
            nameInitialized = true; // Prevent re-initialization
            if (ctx->touchOnlyMode) {
                // Touch-only devices: auto-set and lock to "PLAYER"
                strcpy(ctx->newHighScoreName, "PLAYER");
                ctx->nameEntryCursorPos = 6;
            } else {
                // Browser/keyboard mode: start with "PLAYER" but allow editing
                strcpy(ctx->newHighScoreName, "PLAYER");
                ctx->nameEntryCursorPos = 6;
            }
        }
        
        // Draw single confirmation button for touch
        SDL_SetRenderDrawColor(ctx->renderer, 150, 200, 150, 220);
        SDL_FRect confirmBtn = {w * 0.4f, h * 0.65f, w * 0.2f, h * 0.1f};
        SDL_RenderFillRect(ctx->renderer, &confirmBtn);
        SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, 255);
        const char* confirmText = "TAP TO CONTINUE";
        float confirmTextX = confirmBtn.x + (confirmBtn.w - strlen(confirmText) * 8) * 0.5f;
        float confirmTextY = confirmBtn.y + (confirmBtn.h - 8) * 0.5f;
        SDL_RenderDebugText(ctx->renderer, confirmTextX, confirmTextY, confirmText);
#endif
        
        SDL_SetRenderScale(ctx->renderer, 1.5f, 1.5f);
#endif
    }
    
    // Restore original scale
    SDL_SetRenderScale(ctx->renderer, oldScaleX, oldScaleY);
}

void MainLoop(void* arg) {
    AppContext* ctx = (AppContext*)arg;

    // Fixed 60 FPS logic
    static Uint64 lastFrameTime = 0;
    Uint64 now = SDL_GetTicks();
    if (lastFrameTime == 0) lastFrameTime = now;
    
    Uint64 elapsed = now - lastFrameTime;
    if (elapsed < 16) return; // Skip if less than ~16.6ms passed
    lastFrameTime = now;

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
        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            // On landing page, tap activates touch controls AND starts game
            if (ctx->state == STATE_LANDING) {
#ifdef __EMSCRIPTEN__
                ctx->showTouchControls = true;
#endif
                ctx->state = STATE_PLAYING;
                ResetGame(ctx);
            }
            // On game over, tap restarts with touch controls
            else if (ctx->state == STATE_GAMEOVER) {
#ifdef __EMSCRIPTEN__
                ctx->showTouchControls = true;
#endif
                ContinueGameWithSelectedGeometry(ctx);
                ctx->state = STATE_PLAYING;
            }
            // On highscore screen, tap returns to game over (then player can restart)
            else if (ctx->state == STATE_HIGHSCORE_DISPLAY) {
                bool isEditing = ctx->newHighScorePosition >= 0 && ctx->newHighScorePosition < MAX_HIGHSCORES;
                if (isEditing) {
                    // Auto-finalize the highscore entry for touch devices
                    FinalizeHighScoreEntry(ctx);
                    ctx->newHighScorePosition = -1; // Clear editing mode
                }
                // Don't clear highscoreEntryPending here - it should persist until actual game restart
                ctx->state = STATE_GAMEOVER;
            }
            else {
                // During gameplay, toggle touch controls (web only)
#ifdef __EMSCRIPTEN__
                ctx->showTouchControls = !ctx->showTouchControls;
#endif
            }
        }

        // Touch controls for web version
#ifdef __EMSCRIPTEN__
        if (event.type == SDL_EVENT_FINGER_DOWN || event.type == SDL_EVENT_FINGER_MOTION) {
            // Get touch position normalized to [0,1]
            float touchX = event.tfinger.x;
            float touchY = event.tfinger.y;
            
            // Convert to screen coordinates
            int w, h;
            SDL_GetWindowSize(ctx->window, &w, &h);
            int screenX = (int)(touchX * w);
            int screenY = (int)(touchY * h);
            
            // Handle simple name selection in highscore name entry mode
            if (ctx->state == STATE_HIGHSCORE_DISPLAY) {
                bool isEditing = ctx->newHighScorePosition >= 0 && ctx->newHighScorePosition < MAX_HIGHSCORES;
                if (isEditing) {
                    // Automatically set name to "PLAYER" for mobile
#ifdef __EMSCRIPTEN__
                    if (ctx->newHighScoreName[0] == '\0' || ctx->newHighScoreName[0] == ' ') {
                        strcpy(ctx->newHighScoreName, "PLAYER");
                        ctx->nameEntryCursorPos = 6;
                    }
#endif
                    
                    // Check if touch is on confirmation button (touch mode only)
#ifdef __EMSCRIPTEN__
                    if (screenY > h * 0.65 && screenY < h * 0.75 && screenX > w * 0.4 && screenX < w * 0.6) {
                        // Continue to game over screen
                        ctx->state = STATE_GAMEOVER;
                    }
#endif
                }
            }
            
            // Circular swipe gesture detection for gameplay
            if (ctx->state == STATE_PLAYING) {
                // Convert touch position to center-relative coordinates
                float centerX = w * 0.5f;
                float centerY = h * 0.5f;
                float relX = screenX - centerX;
                float relY = screenY - centerY;
                
                if (event.type == SDL_EVENT_FINGER_DOWN) {
                    // Start of potential swipe gesture
                    ctx->lastTouchX = relX;
                    ctx->lastTouchY = relY;
                    ctx->isSwiping = false;
                    
                    // Reset rotation controls
                    ctx->touchLeftActive = false;
                    ctx->touchRightActive = false;
                } 
                else if (event.type == SDL_EVENT_FINGER_MOTION && !ctx->isSwiping) {
                    // Simplified swipe detection: left/right only
                    float currentX = relX;
                    float minSwipeDistance = 30.0f; // Minimum horizontal distance for swipe
                    
                    // Calculate horizontal movement
                    float deltaX = currentX - ctx->lastTouchX;
                    
                    if (fabs(deltaX) > minSwipeDistance) {
                        // Left swipe (negative X direction) = clockwise
                        if (deltaX < -minSwipeDistance) {
                            ctx->touchLeftActive = false;
                            ctx->touchRightActive = true;  // Right active = clockwise
                        }
                        // Right swipe (positive X direction) = counter-clockwise
                        else if (deltaX > minSwipeDistance) {
                            ctx->touchLeftActive = true;   // Left active = counter-clockwise
                            ctx->touchRightActive = false;
                        }
                        ctx->isSwiping = true;
                    }
                    
                    // Update last position
                    ctx->lastTouchX = currentX;
                    ctx->lastTouchY = relY;
                }
            }
        }
        if (event.type == SDL_EVENT_FINGER_DOWN) {
            // Start tracking touch position
            int w, h;
            SDL_GetWindowSize(ctx->window, &w, &h);
            ctx->lastTouchX = (int)(event.tfinger.x * w);
            ctx->lastTouchY = (int)(event.tfinger.y * h);
            ctx->isSwiping = false;
        }
        if (event.type == SDL_EVENT_FINGER_UP) {
            ctx->touchLeftActive = false;
            ctx->touchRightActive = false;
            ctx->touchFireActive = false;
            ctx->touchSuperzapperActive = false;
            ctx->isSwiping = false;
        }
#endif

        if (event.type == SDL_EVENT_KEY_DOWN) {
            
            if (ctx->state == STATE_LANDING) {
                // Only Arrow Up starts the game with keyboard controls
                if (event.key.scancode == SDL_SCANCODE_UP) {
#ifdef __EMSCRIPTEN__
                    ctx->showTouchControls = false;
#endif
                    ctx->state = STATE_PLAYING;
                    ResetGame(ctx);
                    continue;
                }
                // All other keys ignored on landing page
                continue;
            }
            if (ctx->state == STATE_GAMEOVER) {
                // Try to restart with a specific shape (0-3 keys)
                if (RestartWithShape(ctx, event.key.scancode, event.key.key, event.key.repeat)) {
                    ctx->state = STATE_PLAYING;
                    continue;
                }
                
                // Only Arrow Up restarts with current random geometry
                if (event.key.scancode == SDL_SCANCODE_UP) {
                    ContinueGameWithSelectedGeometry(ctx);
                    ctx->state = STATE_PLAYING;
                    continue;
                }
                
                // R key restarts the game
                if (event.key.scancode == SDL_SCANCODE_R) {
                    // Check if we have a pending highscore entry that needs to be handled
                    if (ctx->highscoreEntryPending) {
                        printf("DEBUG: R pressed but have pending highscore, showing highscore screen\n");
                        ctx->state = STATE_HIGHSCORE_DISPLAY;
                    } else {
                        ctx->state = STATE_PLAYING;
                    }
                    continue;
                }
                continue; // Ignore other keys
            }
            if (event.key.scancode == SDL_SCANCODE_LEFT) ctx->playerSegment = (ctx->playerSegment + 1) % NUM_SIDES;
            if (event.key.scancode == SDL_SCANCODE_RIGHT) ctx->playerSegment = (ctx->playerSegment - 1 + NUM_SIDES) % NUM_SIDES;
            if (ctx->state == STATE_PLAYING) {
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
                        PlayWav(&ctx->audio, WAV_LASERZAP, false);
                        break;
                    }
                }
            }
            if (event.key.scancode == SDL_SCANCODE_Z && !ctx->superzapperUsed) {
                ctx->superzapperUsed = true;
                ctx->flashTimer = 10;
                for(int i=0; i<MAX_ENEMIES; i++) ctx->enemies[i].active = false;
                PlayWav(&ctx->audio, WAV_EXPLOSION, false);
            }
            
            // Handle high score display with integrated name entry
            if (ctx->state == STATE_HIGHSCORE_DISPLAY) {
                // Check if we're in name entry mode
                bool isEditing = ctx->newHighScorePosition >= 0 && ctx->newHighScorePosition < MAX_HIGHSCORES;
                
                if (isEditing) {
                    if (event.key.scancode == SDL_SCANCODE_RETURN) {
                        // Finalize name entry
                        FinalizeHighScoreEntry(ctx);
                        ctx->newHighScorePosition = -1; // Clear editing mode
                    } else if (event.key.scancode == SDL_SCANCODE_BACKSPACE) {
                        // Backspace - remove last character
                        if (ctx->nameEntryCursorPos > 0) {
                            ctx->nameEntryCursorPos--;
                            ctx->newHighScoreName[ctx->nameEntryCursorPos] = ' ';
                        }
                    } else if (event.key.key >= 'a' && event.key.key <= 'z' && !event.key.repeat) {
                        // Letter keys (ignore repeats to prevent multiple registrations)
                        if (ctx->nameEntryCursorPos < 19) {
                            ctx->newHighScoreName[ctx->nameEntryCursorPos] = (char)toupper(event.key.key);
                            ctx->nameEntryCursorPos++;
                            // Ensure null termination
                            ctx->newHighScoreName[ctx->nameEntryCursorPos] = '\0';
                        }
                    }
                } else {
                    // Not editing, handle normal dismissal
                    if (event.key.scancode == SDL_SCANCODE_R) {
                        ctx->state = STATE_GAMEOVER;
                    }
                }
            }
            
            // Handle high score screen dismissal
            if (ctx->showHighScores && event.key.scancode == SDL_SCANCODE_R) {
                ctx->showHighScores = false;
                ResetGame(ctx);
                ctx->state = STATE_PLAYING;
            }
        }
    }

    // Touch controls logic (web only)
#ifdef __EMSCRIPTEN__
    // Frame counter for very slow rotation (20% speed = every 5th frame)
    static int rotationFrameCounter = 0;
    rotationFrameCounter++;
    
    // Much slower rotation - 20% of original speed
    if (rotationFrameCounter % 5 == 0) { // Only rotate every 5th frame
        if (ctx->touchLeftActive) {
            ctx->playerSegment = (ctx->playerSegment + 1) % NUM_SIDES;
        }
        if (ctx->touchRightActive) {
            ctx->playerSegment = (ctx->playerSegment - 1 + NUM_SIDES) % NUM_SIDES;
        }
    }
    
    // Tap-to-fire anywhere: detect tap releases (fixed to fire only once)
    static bool wasTouching = false;
    static bool fireTriggered = false; // Prevent multiple fires from same tap
    
    if (!ctx->touchLeftActive && !ctx->touchRightActive && !ctx->isSwiping) {
        // Not swiping, so this could be a tap
        if (!wasTouching) {
            // Touch just started - could be a tap
            wasTouching = true;
            fireTriggered = false; // Reset for new tap
        } else {
            // Touch ended - this was a tap, fire! (but only if not already triggered)
            wasTouching = false;
            
            if (!fireTriggered) {
                fireTriggered = true; // Mark as triggered to prevent multiple fires
                
                // Fire when tap is released
                bool shouldFire = false;
                if (ctx->state == STATE_PLAYING) {
                    static Uint64 lastFireTime = 0;
                    Uint64 currentTime = SDL_GetTicks();
                    if (currentTime - lastFireTime > 200) { // 200ms cooldown
                        shouldFire = true;
                        lastFireTime = currentTime;
                    }
                }
                
                if (shouldFire) {
                    for(int i=0; i<MAX_SHOTS; i++) {
                        if(!ctx->shots[i].active) {
                            ctx->shots[i].active = true;
                            ctx->shots[i].z = 2.0f;
                            ctx->shots[i].segment = ctx->playerSegment;
                            PlayWav(&ctx->audio, WAV_LASERZAP, false);
                            break;
                        }
                    }
                }
            }
        }
    } else {
        wasTouching = false; // Reset if swiping
        fireTriggered = false;
    }
    
    // Fire control is now handled in the touch controls section above
    // (tap anywhere to fire)
    
    // Superzapper control
    static bool wasTouchSuperzapperActive = false;
    if (ctx->touchSuperzapperActive && !wasTouchSuperzapperActive && !ctx->superzapperUsed && ctx->state == STATE_PLAYING) {
        ctx->superzapperUsed = true;
        ctx->flashTimer = 10;
        for(int i=0; i<MAX_ENEMIES; i++) ctx->enemies[i].active = false;
        PlayWav(&ctx->audio, WAV_EXPLOSION, false);
    }
    wasTouchSuperzapperActive = ctx->touchSuperzapperActive;
#endif

    if (ctx->state == STATE_PLAYING) {
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
                ctx->enemies[i].z -= 0.05f * ctx->gameSpeedMultiplier;
                if (ctx->enemies[i].z < 2.0f) {
                    ctx->enemies[i].active = false;
                    ctx->lives--;
                    PlayWav(&ctx->audio, WAV_EXPLOSION, false);
                    if (ctx->lives <= 0) {
                        TriggerGameOverShape(ctx);
                    }
                }

                for (int j = 0; j < MAX_SHOTS; j++) {
                    if (ctx->shots[j].active && ShotEnemyHit(ctx, &ctx->shots[j], &ctx->enemies[i])) {
                        ctx->shots[j].active = false;
                        ctx->enemies[i].active = false;
                        ctx->score += 100;
                        ctx->enemiesDestroyedCount++;
                        
                        // Increase game speed slightly with each kill
                        ctx->gameSpeedMultiplier += 0.02f;
                        if (ctx->gameSpeedMultiplier > 2.5f) {
                            ctx->gameSpeedMultiplier = 2.5f; // Cap maximum speed
                        }
                        
                        PlayWav(&ctx->audio, WAV_COIN, false);
                        
                        // Check if we should change geometry
                        if (ctx->enemiesDestroyedCount >= 8) {
                            TunnelShape newShape;
                            do {
                                newShape = (TunnelShape)(rand() % 4);
                            } while (newShape == ctx->tunnelShape); // Ensure different geometry
                            
                            ctx->selectedTunnelShape = newShape;
                            ApplyTunnelShape(ctx, newShape);
                            ctx->enemiesDestroyedCount = 0; // Reset counter
                            
                            // Visual feedback - flash
                            ctx->flashTimer = 5;
                            PlayWav(&ctx->audio, WAV_PERCUSSION, false);
                        }
                        break;
                    }
                }
            }
        }

        // --- NEW: Burst fire logic ---
        if (ctx->remainingInBurst == 0) {
            if (now >= ctx->nextBurstTime) {
                ctx->remainingInBurst = MAX_BURST_SHOTS;
                // Play sound immediately as a warning
                PlayWav(&ctx->audio, WAV_SHOTBURST, false);
                // Delay the first shot spawning by 500ms
                ctx->nextBurstShotTime = now + 500;
                // Schedule next burst (20 +/- 10s)
                ctx->nextBurstTime = now + 2500 + (rand() % 2000);
            }
        } else {
            if (now >= ctx->nextBurstShotTime) {
                // Spawn one shot in the burst
                for (int i = 0; i < MAX_BURST_SHOTS; i++) {
                    if (!ctx->burstShots[i].active) {
                        ctx->burstShots[i].active = true;
                        ctx->burstShots[i].z = NUM_RINGS * RING_DISTANCE;
                        ctx->burstShots[i].segment = ctx->playerSegment; // Aimed at blaster
                        break;
                    }
                }
                ctx->remainingInBurst--;
                ctx->nextBurstShotTime = now + 40; // almost instant spacing
            }
        }

        // Update burst shots
        for (int i = 0; i < MAX_BURST_SHOTS; i++) {
            if (ctx->burstShots[i].active) {
                ctx->burstShots[i].z -= 0.15f * ctx->gameSpeedMultiplier; // Faster than enemies
                if (ctx->burstShots[i].z < 2.0f) {
                    ctx->burstShots[i].active = false;
                    // Collision with player
                    if (ctx->burstShots[i].segment == ctx->playerSegment) {
                        ctx->lives--;
                        PlayWav(&ctx->audio, WAV_EXPLOSION, false);
                        if (ctx->lives <= 0) TriggerGameOverShape(ctx);
                    }
                }
            }
        }
        }

    if (ctx->state == STATE_GAMEOVER) {
        SDL_SetRenderDrawColor(ctx->renderer, 50, 0, 0, 255); // Dark red background
    } else if (ctx->flashTimer > 0) {
        SDL_SetRenderDrawColor(ctx->renderer, 255, 255, 255, 255); // White flash
    } else {
        SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 10, 255);
    }
    SDL_RenderClear(ctx->renderer);

    if (ctx->state == STATE_LANDING) {
        DrawLandingPage(ctx, w, h);
    } else {
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

                // Draw Burst Shots
                for (int i = 0; i < MAX_BURST_SHOTS; i++) {
                if (ctx->burstShots[i].active) {
                float param1 = (float)ctx->burstShots[i].segment / (float)NUM_SIDES;
                float param2 = (float)(ctx->burstShots[i].segment + 1) / (float)NUM_SIDES;
                float z = ctx->burstShots[i].z;
                float tx1, ty1, tx2, ty2;
                TunnelXY(param1, ctx->tunnelShape, &tx1, &ty1);
                TunnelXY(param2, ctx->tunnelShape, &tx2, &ty2);

                // Center of the segment
                float cx = (tx1 + tx2) * 0.5f;
                float cy = (ty1 + ty2) * 0.5f;
                float size = 0.1f;

                Point3D p[4] = {
                    {cx - size, cy - size, z},
                    {cx + size, cy - size, z},
                    {cx + size, cy + size, z},
                    {cx - size, cy + size, z}
                };
                float sx[4], sy[4];
                for(int k=0; k<4; k++) Project(p[k], w, h, &sx[k], &sy[k]);

                SDL_SetRenderDrawColor(ctx->renderer, 255, 255, 255, 255);
                for(int k=0; k<4; k++) {
                    SDL_RenderLine(ctx->renderer, sx[k], sy[k], sx[(k+1)%4], sy[(k+1)%4]);
                }
                }
                }
        // Draw Shots
        SDL_SetRenderDrawBlendMode(ctx->renderer, SDL_BLENDMODE_ADD);
        for (int i = 0; i < MAX_SHOTS; i++) {
            if (ctx->shots[i].active) {
                float param1 = (float)ctx->shots[i].segment / (float)NUM_SIDES;
                float param2 = (float)(ctx->shots[i].segment + 1) / (float)NUM_SIDES;
                float z_start = ctx->shots[i].z;
                float z_end = z_start + 0.9f;

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

                SDL_SetRenderDrawColor(ctx->renderer, 200, 0, 0, 180);
                SDL_RenderLine(ctx->renderer, x1 - 1.0f, y1, x3 - 1.0f, y3);
                SDL_RenderLine(ctx->renderer, x1 + 1.0f, y1, x3 + 1.0f, y3);
                SDL_RenderLine(ctx->renderer, x1, y1 - 1.0f, x3, y3 - 1.0f);
                SDL_RenderLine(ctx->renderer, x1, y1 + 1.0f, x3, y3 + 1.0f);
                SDL_RenderLine(ctx->renderer, x2 - 1.0f, y2, x4 - 1.0f, y4);
                SDL_RenderLine(ctx->renderer, x2 + 1.0f, y2, x4 + 1.0f, y4);
                SDL_RenderLine(ctx->renderer, x2, y2 - 1.0f, x4, y4 - 1.0f);
                SDL_RenderLine(ctx->renderer, x2, y2 + 1.0f, x4, y4 + 1.0f);

                SDL_SetRenderDrawColor(ctx->renderer, 255, 0, 0, 255);
                SDL_RenderLine(ctx->renderer, x1, y1, x3, y3);
                SDL_RenderLine(ctx->renderer, x2, y2, x4, y4);
            }
        }
        SDL_SetRenderDrawBlendMode(ctx->renderer, SDL_BLENDMODE_BLEND);

        if (ctx->state == STATE_PLAYING) {
            DrawBlaster(ctx, w, h);
            DrawHUD(ctx, w, h);
        }
        if (ctx->state == STATE_GAMEOVER) {
            DrawGameOverPrompt(ctx, w, h);
        }
        if (ctx->state == STATE_HIGHSCORE_DISPLAY) {
            DrawHighScoreDisplayScreen(ctx, w, h);
        }
    }

    SDL_RenderPresent(ctx->renderer);
}

static bool LoadAndConvertWAV(const char* filename, const SDL_AudioSpec* targetSpec, WavBuffer* out) {
    SDL_AudioSpec srcSpec;
    Uint8* srcData;
    Uint32 srcLen;
    if (!SDL_LoadWAV(filename, &srcSpec, &srcData, &srcLen)) {
        fprintf(stderr, "Failed to load %s: %s\n", filename, SDL_GetError());
        return false;
    }

    // Use SDL_AudioStream for conversion as it's more reliable in some SDL3 versions
    SDL_AudioStream* stream = SDL_CreateAudioStream(&srcSpec, targetSpec);
    if (!stream) {
        fprintf(stderr, "Failed to create conversion stream for %s: %s\n", filename, SDL_GetError());
        SDL_free(srcData);
        return false;
    }

    if (!SDL_PutAudioStreamData(stream, srcData, srcLen)) {
        fprintf(stderr, "Failed to put data into conversion stream for %s: %s\n", filename, SDL_GetError());
        SDL_DestroyAudioStream(stream);
        SDL_free(srcData);
        return false;
    }

    if (!SDL_FlushAudioStream(stream)) {
        fprintf(stderr, "Failed to flush conversion stream for %s: %s\n", filename, SDL_GetError());
        SDL_DestroyAudioStream(stream);
        SDL_free(srcData);
        return false;
    }

    int available = SDL_GetAudioStreamAvailable(stream);
    if (available > 0) {
        out->data = (float*)SDL_malloc(available);
        if (out->data) {
            int got = SDL_GetAudioStreamData(stream, out->data, available);
            out->len = (Uint32)(got / sizeof(float));
            fprintf(stderr, "Loaded and converted %s: %u samples (%d bytes)\n", filename, out->len, got);
            SDL_DestroyAudioStream(stream);
            SDL_free(srcData);
            return true;
        }
    }

    fprintf(stderr, "Conversion of %s yielded no data: %s\n", filename, SDL_GetError());
    SDL_DestroyAudioStream(stream);
    SDL_free(srcData);
    return false;
}

int main(int argc, char* argv[]) {
    srand((unsigned int)time(NULL));
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) return 1;
    AppContext ctx = {0};
    ctx.selectedTunnelShape = TUNNEL_CIRCLE;
    ctx.window = SDL_CreateWindow("Tempest SDL3", 800, 800, 0);
    ctx.renderer = SDL_CreateRenderer(ctx.window, NULL);
    
    // Initialize touch controls for web version
#ifdef __EMSCRIPTEN__
    ctx.showTouchControls = true;
    // For now, assume web version might have keyboard, so don't force touch-only mode
    // In future, could detect actual touch capability
    ctx.touchOnlyMode = false;
#endif
    
    // Load high scores
    LoadHighScores(&ctx);
    
    SDL_AudioSpec targetSpec = {SDL_AUDIO_F32, 2, 44100};
    ctx.audio.stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &targetSpec, &AudioCallback, &ctx.audio);
    
    if (ctx.audio.stream) {
        if (!LoadAndConvertWAV("laserzap.wav", &targetSpec, &ctx.audio.wavs[WAV_LASERZAP])) fprintf(stderr, "Failed laserzap\n");
        if (!LoadAndConvertWAV("explosion.wav", &targetSpec, &ctx.audio.wavs[WAV_EXPLOSION])) fprintf(stderr, "Failed explosion\n");
        if (!LoadAndConvertWAV("percussion.wav", &targetSpec, &ctx.audio.wavs[WAV_PERCUSSION])) fprintf(stderr, "Failed percussion\n");
        if (!LoadAndConvertWAV("coin.wav", &targetSpec, &ctx.audio.wavs[WAV_COIN])) fprintf(stderr, "Failed coin\n");
        if (!LoadAndConvertWAV("shotburst.wav", &targetSpec, &ctx.audio.wavs[WAV_SHOTBURST])) fprintf(stderr, "Failed shotburst\n");
        
        PlayWav(&ctx.audio, WAV_PERCUSSION, true);
        SDL_ResumeAudioStreamDevice(ctx.audio.stream);
    }
    
    ctx.running = true;
    ctx.state = STATE_LANDING;
    ResetGame(&ctx);

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(MainLoop, &ctx, 0, 1);
#else
    while (ctx.running) {
        MainLoop(&ctx);
        SDL_Delay(1); // Minimal delay to prevent 100% CPU usage
    }
#endif
    if (ctx.audio.stream) SDL_DestroyAudioStream(ctx.audio.stream);
    for (int i = 0; i < NUM_WAVS; i++) {
        if (ctx.audio.wavs[i].data) SDL_free(ctx.audio.wavs[i].data);
    }
    SDL_Quit();
    return 0;
}
