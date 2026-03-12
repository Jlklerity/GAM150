#include "Particles.h"
#include "AEEngine.h"
#include <cmath>
#include <cstdlib>

// ============================================================================
//  Internal constants
// ============================================================================
namespace
{
    // --- Pool sizes ---
    constexpr int MAX_PARTICLES = 256;
    constexpr int MAX_IMPACT_LINES = 32;

    // --- Damage impact tuning ---
    constexpr int   DAMAGE_DUST_COUNT = 22;      // number of dust particles
    constexpr float DAMAGE_DUST_SPEED = 320.0f;  // max horizontal speed (px/s)
    constexpr float DAMAGE_DUST_UPSPEED = 200.0f;  // max initial upward speed
    constexpr float DAMAGE_PARTICLE_LIFE = 0.7f;    // seconds before fully faded
    constexpr float DAMAGE_PARTICLE_SIZE_MIN = 4.0f;
    constexpr float DAMAGE_PARTICLE_SIZE_MAX = 10.0f;

    constexpr int   DAMAGE_LINE_COUNT = 8;       // radiating impact lines
    constexpr float DAMAGE_LINE_LENGTH_MIN = 20.0f;
    constexpr float DAMAGE_LINE_LENGTH_MAX = 55.0f;
    constexpr float DAMAGE_LINE_LIFE = 0.18f;   // seconds

    constexpr float DAMAGE_FLASH_DURATION = 0.12f;   // seconds for flash to fade
    constexpr float DAMAGE_FLASH_PEAK_ALPHA = 0.55f;   // max overlay opacity

    constexpr float DAMAGE_SHAKE_DURATION = 0.28f;   // seconds of shake
    constexpr float DAMAGE_SHAKE_INTENSITY = 7.0f;    // max offset in pixels

    // --- Kill impact tuning ---
    constexpr int   KILL_BURST_COUNT = 18;      // particles in burst
    constexpr float KILL_BURST_SPEED = 380.0f;  // max speed (px/s)
    constexpr float KILL_BURST_UPSPEED = 260.0f;  // max initial upward speed
    constexpr float KILL_PARTICLE_LIFE = 0.55f;
    constexpr float KILL_PARTICLE_SIZE_MIN = 4.0f;
    constexpr float KILL_PARTICLE_SIZE_MAX = 9.0f;

    // --- Shared physics ---
    constexpr float PARTICLE_GRAVITY = 600.0f;  // px/s^2 downward pull
    constexpr float PARTICLE_DRAG = 0.88f;   // horizontal damping per frame

    // ============================================================================
    //  Data structures
    // ============================================================================
    struct Particle
    {
        float x, y;             // world position
        float vx, vy;           // velocity (px/s)
        float size;             // width and height in pixels
        float r, g, b;          // color
        float life;             // 1 = full, 0 = dead
        float decay;            // life lost per second
        bool  active = false;
    };

    struct ImpactLine
    {
        float x1, y1;           // start point (near impact)
        float x2, y2;           // end point (away from impact)
        float r, g, b;
        float life;             // 1 = full, 0 = dead
        float decay;
        bool  active = false;
    };

    struct ScreenShake
    {
        float timer = 0.0f;
        float duration = 0.0f;
        float intensity = 0.0f;
        float offset_x = 0.0f;
        float offset_y = 0.0f;
    };

    struct FlashState
    {
        float alpha = 0.0f;
        float timer = 0.0f;
        float duration = 0.0f;
    };

    // ============================================================================
    //  State
    // ============================================================================
    Particle     g_particles[MAX_PARTICLES]{};
    ImpactLine   g_lines[MAX_IMPACT_LINES]{};
    ScreenShake  g_shake{};
    FlashState   g_flash{};

    AEGfxVertexList* g_quad_mesh = nullptr;     // 1x1 unit rect, reused for all particles

    // ============================================================================
    //  Utility helpers
    // ============================================================================
    float Rand01()
    {
        return (float)std::rand() / (float)RAND_MAX;
    }

    float RandRange(float a, float b)
    {
        return a + (b - a) * Rand01();
    }

    // Returns a free particle slot, or -1 if pool is full
    int AllocParticle()
    {
        for (int i = 0; i < MAX_PARTICLES; ++i)
            if (!g_particles[i].active) return i;
        return -1;
    }

    // Returns a free impact line slot, or -1 if pool is full
    int AllocLine()
    {
        for (int i = 0; i < MAX_IMPACT_LINES; ++i)
            if (!g_lines[i].active) return i;
        return -1;
    }

    // Maps enemy word length (3-8) to an RGB burst color
    void WordLengthToColor(int word_length, float& r, float& g, float& b)
    {
        switch (word_length)
        {
        case 3:  r = 0.4f; g = 0.9f; b = 1.0f; break;   // light blue
        case 4:  r = 0.4f; g = 1.0f; b = 0.5f; break;   // green
        case 5:  r = 1.0f; g = 0.9f; b = 0.2f; break;   // yellow
        case 6:  r = 1.0f; g = 0.5f; b = 0.1f; break;   // orange
        case 7:  r = 1.0f; g = 0.2f; b = 0.3f; break;   // red
        case 8:  r = 0.8f; g = 0.2f; b = 1.0f; break;   // purple
        default: r = 1.0f; g = 1.0f; b = 1.0f; break;   // white fallback
        }
    }

    // Draws a colored quad at world position using the shared unit mesh
    void DrawQuad(float x, float y, float size, float r, float g, float b, float alpha)
    {
        if (!g_quad_mesh) return;

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(r, g, b, alpha);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

        AEMtx33 scale_m, trans_m, transform_m;
        AEMtx33Scale(&scale_m, size, size);
        AEMtx33Trans(&trans_m, x, y);
        AEMtx33Concat(&transform_m, &trans_m, &scale_m);

        AEGfxSetTransform(transform_m.m);
        AEGfxMeshDraw(g_quad_mesh, AE_GFX_MDM_TRIANGLES);
    }

    // Draws a line between two world points (as a thin elongated quad)
    void DrawLine(float x1, float y1, float x2, float y2,
        float r, float g, float b, float alpha)
    {
        if (!g_quad_mesh) return;

        float dx = x2 - x1;
        float dy = y2 - y1;
        float len = sqrtf(dx * dx + dy * dy);
        if (len < 0.1f) return;

        float cx = (x1 + x2) * 0.5f;
        float cy = (y1 + y2) * 0.5f;
        float angle = atan2f(dy, dx);

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(r, g, b, alpha);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

        AEMtx33 scale_m, rot_m, trans_m, temp_m, transform_m;
        AEMtx33Scale(&scale_m, len, 3.0f);          // 3px thick line
        AEMtx33Rot(&rot_m, angle);
        AEMtx33Trans(&trans_m, cx, cy);
        AEMtx33Concat(&temp_m, &rot_m, &scale_m);
        AEMtx33Concat(&transform_m, &trans_m, &temp_m);

        AEGfxSetTransform(transform_m.m);
        AEGfxMeshDraw(g_quad_mesh, AE_GFX_MDM_TRIANGLES);
    }

} // anonymous namespace

// ============================================================================
//  Public 
// ============================================================================

void Particles_Init()
{
    // Build a simple 1x1 unit rectangle mesh (centered at origin)
    AEGfxMeshStart();

    AEGfxTriAdd(
        -0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 0.0f,
        0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 0.0f,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 1.0f);

    AEGfxTriAdd(
        0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 0.0f,
        0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 1.0f);

    g_quad_mesh = AEGfxMeshEnd();

    // Clear all state
    for (int i = 0; i < MAX_PARTICLES; ++i) g_particles[i].active = false;
    for (int i = 0; i < MAX_IMPACT_LINES; ++i) g_lines[i].active = false;
    g_shake = {};
    g_flash = {};
}

void Particles_Free()
{
    if (g_quad_mesh)
    {
        AEGfxMeshFree(g_quad_mesh);
        g_quad_mesh = nullptr;
    }
}

void Particles_Update(float dt)
{
    // --- Particles ---
    for (int i = 0; i < MAX_PARTICLES; ++i)
    {
        Particle& p = g_particles[i];
        if (!p.active) continue;

        p.life -= p.decay * dt;
        if (p.life <= 0.0f) { p.active = false; continue; }

        p.vy -= PARTICLE_GRAVITY * dt;     // gravity pulls down
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.vx *= (1.0f - (1.0f - PARTICLE_DRAG) * dt * 60.0f); // frame-rate independent drag
    }

    // --- Impact lines ---
    for (int i = 0; i < MAX_IMPACT_LINES; ++i)
    {
        ImpactLine& l = g_lines[i];
        if (!l.active) continue;

        l.life -= l.decay * dt;
        if (l.life <= 0.0f) l.active = false;
    }

    // --- Screen shake ---
    if (g_shake.timer > 0.0f)
    {
        g_shake.timer -= dt;
        float t = g_shake.timer / g_shake.duration;    // 1..0 as shake fades
        float intensity = g_shake.intensity * t;
        g_shake.offset_x = RandRange(-intensity, intensity);
        g_shake.offset_y = RandRange(-intensity * 0.5f, intensity * 0.5f);

        if (g_shake.timer <= 0.0f)
        {
            g_shake.timer = 0.0f;
            g_shake.offset_x = 0.0f;
            g_shake.offset_y = 0.0f;
        }
        AEGfxSetCamPosition(g_shake.offset_x, g_shake.offset_y);
    }
    else
    {
        // Ensure camera is reset when shake is done
        AEGfxSetCamPosition(0.0f, 0.0f);
    }

    // --- Flash ---
    if (g_flash.timer > 0.0f)
    {
        g_flash.timer -= dt;
        if (g_flash.timer < 0.0f) g_flash.timer = 0.0f;

        float t = g_flash.timer / g_flash.duration;    // 1..0
        g_flash.alpha = DAMAGE_FLASH_PEAK_ALPHA * t;
    }
}

void Particles_Draw()
{
    if (!g_quad_mesh) return;

    // --- Impact lines (drawn first so particles appear on top) ---
    for (int i = 0; i < MAX_IMPACT_LINES; ++i)
    {
        ImpactLine& l = g_lines[i];
        if (!l.active) continue;
        DrawLine(l.x1, l.y1, l.x2, l.y2, l.r, l.g, l.b, l.life);
    }

    // --- Particles ---
    for (int i = 0; i < MAX_PARTICLES; ++i)
    {
        Particle& p = g_particles[i];
        if (!p.active) continue;
        DrawQuad(p.x, p.y, p.size, p.r, p.g, p.b, p.life);
    }

    //  Screen flash 
    if (g_flash.alpha > 0.0f && g_quad_mesh)
    {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, g_flash.alpha);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

        AEMtx33 transform_m;
        AEMtx33Scale(&transform_m, 1600.0f, 900.0f);

        AEGfxSetTransform(transform_m.m);
        AEGfxMeshDraw(g_quad_mesh, AE_GFX_MDM_TRIANGLES);
    }

    // Reset render state cleanly for subsequent draw calls
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxSetTransparency(1.0f);
}

void Particles_SpawnDamageImpact(float x, float y)
{
    // Dust particles 
    const float dust_colors[3][3] = {
        { 0.90f, 0.90f, 0.90f },   // light grey
        { 1.00f, 1.00f, 1.00f },   // white
        { 0.70f, 0.70f, 0.70f },   // mid grey
    };

    for (int i = 0; i < DAMAGE_DUST_COUNT; ++i)
    {
        int slot = AllocParticle();
        if (slot < 0) break;

        Particle& p = g_particles[slot];
        p.active = true;
        p.x = x + RandRange(-12.0f, 12.0f);
        p.y = y + RandRange(-4.0f, 4.0f);
        p.vx = RandRange(-DAMAGE_DUST_SPEED, DAMAGE_DUST_SPEED);
        p.vy = RandRange(0.0f, DAMAGE_DUST_UPSPEED);   // mostly upward fan
        p.size = RandRange(DAMAGE_PARTICLE_SIZE_MIN, DAMAGE_PARTICLE_SIZE_MAX);
        p.life = 1.0f;
        p.decay = 1.0f / (DAMAGE_PARTICLE_LIFE * RandRange(0.7f, 1.3f));

        int ci = i % 3;
        p.r = dust_colors[ci][0];
        p.g = dust_colors[ci][1];
        p.b = dust_colors[ci][2];
    }

    // --- Impact lines (yellow, radiate outward horizontally) ---
    for (int i = 0; i < DAMAGE_LINE_COUNT; ++i)
    {
        int slot = AllocLine();
        if (slot < 0) break;

        ImpactLine& l = g_lines[slot];
        l.active = true;

        // Fan lines left and right along the floor, slight upward angle
        float angle = RandRange(-0.4f, 0.4f);                   // near-horizontal
        if (i % 2 == 0) angle += 3.14159f;                        // alternate sides

        float length = RandRange(DAMAGE_LINE_LENGTH_MIN, DAMAGE_LINE_LENGTH_MAX);
        float near_dist = 10.0f;                                   // gap from center

        l.x1 = x + cosf(angle) * near_dist;
        l.y1 = y + sinf(angle) * near_dist;
        l.x2 = x + cosf(angle) * (near_dist + length);
        l.y2 = y + sinf(angle) * (near_dist + length);

        l.r = 1.0f; l.g = 0.85f; l.b = 0.1f;                    // yellow
        l.life = 1.0f;
        l.decay = 1.0f / DAMAGE_LINE_LIFE;
        l.active = true;
    }

    // --- Screen flash ---
    g_flash.timer = DAMAGE_FLASH_DURATION;
    g_flash.duration = DAMAGE_FLASH_DURATION;
    g_flash.alpha = DAMAGE_FLASH_PEAK_ALPHA;

    // --- Screen shake ---
    g_shake.timer = DAMAGE_SHAKE_DURATION;
    g_shake.duration = DAMAGE_SHAKE_DURATION;
    g_shake.intensity = DAMAGE_SHAKE_INTENSITY;
}

void Particles_SpawnKillImpact(float x, float y, int word_length)
{
    float cr, cg, cb;
    WordLengthToColor(word_length, cr, cg, cb);

    // Slightly lighter secondary color for variety
    float cr2 = cr * 0.6f + 0.4f;
    float cg2 = cg * 0.6f + 0.4f;
    float cb2 = cb * 0.6f + 0.4f;

    for (int i = 0; i < KILL_BURST_COUNT; ++i)
    {
        int slot = AllocParticle();
        if (slot < 0) break;

        Particle& p = g_particles[slot];
        p.active = true;
        p.x = x + RandRange(-10.0f, 10.0f);
        p.y = y + RandRange(-4.0f, 4.0f);

        // Full 360 burst (wide horizontal, some vertical)
        float angle = RandRange(-3.14159f, 3.14159f);
        float speed = RandRange(KILL_BURST_SPEED * 0.4f, KILL_BURST_SPEED);
        p.vx = cosf(angle) * speed;
        p.vy = fabsf(sinf(angle)) * KILL_BURST_UPSPEED + RandRange(30.0f, 80.0f); // bias upward

        p.size = RandRange(KILL_PARTICLE_SIZE_MIN, KILL_PARTICLE_SIZE_MAX);
        p.life = 1.0f;
        p.decay = 1.0f / (KILL_PARTICLE_LIFE * RandRange(0.7f, 1.3f));

        // Alternate between primary and lighter secondary color
        if (i % 2 == 0) { p.r = cr;  p.g = cg;  p.b = cb; }
        else { p.r = cr2; p.g = cg2; p.b = cb2; }
    }

}

#pragma warning(push)
#pragma warning(disable: 26440)  // VCR003: suppress 'function can be made static' - intentionally non-static, declared in Particles.h
void Particles_TriggerShake(float duration, float intensity)
{
    g_shake.timer = duration;
    g_shake.duration = duration;
    g_shake.intensity = intensity;
}
#pragma warning(pop)