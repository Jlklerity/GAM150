#include "pch.h"
#include "Particles.h"

//  Internal constants
namespace
{
    //  Pool sizes 
    constexpr int MAX_PARTICLES = 256;       // max simultaneous particles
    constexpr int MAX_IMPACT_LINES = 32;     // max simultaneous impact lines

    //  Damage impact tuning 
    constexpr int   DAMAGE_DUST_COUNT = 22;          // number of dust particles per impact
    constexpr float DAMAGE_DUST_SPEED = 320.0f;      // max horizontal speed (px/s)
    constexpr float DAMAGE_DUST_UPSPEED = 200.0f;    // max initial upward speed (px/s)
    constexpr float DAMAGE_PARTICLE_LIFE = 0.7f;     // seconds before fully faded
    constexpr float DAMAGE_PARTICLE_SIZE_MIN = 4.0f; // min particle size (px)
    constexpr float DAMAGE_PARTICLE_SIZE_MAX = 10.0f;// max particle size (px)

    constexpr int   DAMAGE_LINE_COUNT = 8;           // radiating impact lines per impact
    constexpr float DAMAGE_LINE_LENGTH_MIN = 20.0f;  // min line length (px)
    constexpr float DAMAGE_LINE_LENGTH_MAX = 55.0f;  // max line length (px)
    constexpr float DAMAGE_LINE_LIFE = 0.18f;        // seconds before line fades

    constexpr float DAMAGE_FLASH_DURATION = 0.12f;   // seconds for flash to fade
    constexpr float DAMAGE_FLASH_PEAK_ALPHA = 0.55f; // max overlay opacity

    constexpr float DAMAGE_SHAKE_DURATION = 0.28f;   // seconds of shake
    constexpr float DAMAGE_SHAKE_INTENSITY = 7.0f;   // max camera offset (px)

    //  Kill impact tuning 
    constexpr int   KILL_BURST_COUNT = 18;           // particles in burst
    constexpr float KILL_BURST_SPEED = 380.0f;       // max speed (px/s)
    constexpr float KILL_BURST_UPSPEED = 260.0f;     // max initial upward speed (px/s)
    constexpr float KILL_PARTICLE_LIFE = 0.55f;      // seconds before fully faded
    constexpr float KILL_PARTICLE_SIZE_MIN = 4.0f;   // min particle size (px)
    constexpr float KILL_PARTICLE_SIZE_MAX = 9.0f;   // max particle size (px)

    //  Shared physics 
    constexpr float PARTICLE_GRAVITY = 600.0f;  // px/s^2 downward pull applied each frame
    constexpr float PARTICLE_DRAG = 0.88f;      // horizontal damping per frame (0=no movement, 1=no drag)

    //  Data structures

    // single particle, lives in g_particles pool
    struct Particle
    {
        float x, y;          // world position
        float vx, vy;        // velocity (px/s)
        float size;          // width and height in pixels
        float r, g, b;       // color
        float life;          // 1 = full, 0 = dead
        float decay;         // life lost per second
        bool  active = false;// false = slot is free for reuse
    };

    // single impact line, lives in g_lines pool
    struct ImpactLine
    {
        float x1, y1;        // start point (near impact)
        float x2, y2;        // end point (away from impact)
        float r, g, b;       // color
        float life;          // 1 = full, 0 = dead (used as alpha)
        float decay;         // life lost per second
        bool  active = false;// false = slot is free for reuse
    };

    // screen shake state, only one active at a time
    struct ScreenShake
    {
        float timer = 0.0f;     // time remaining on shake
        float duration = 0.0f; // total shake duration (used to calculate fade)
        float intensity = 0.0f;// max camera offset in pixels
        float offset_x = 0.0f; // current frame camera x offset
        float offset_y = 0.0f; // current frame camera y offset
    };

    // screen flash state, only one active at a time
    struct FlashState
    {
        float alpha = 0.0f;    // current overlay opacity
        float timer = 0.0f;    // time remaining on flash
        float duration = 0.0f; // total flash duration (used to calculate fade)
    };

    //  Sprite effect tuning
    constexpr int   MAX_SPRITE_EFFECTS = 16;          // max simultaneous sprite-sheet effects
    constexpr float SHIELD_BREAK_FRAME_DUR = 0.07f;   // seconds per frame for shield break
    constexpr float FRIENDLY_HEAL_FRAME_DUR = 0.08f;  // seconds per frame for friendly heal
    constexpr float SPRITE_EFFECT_SIZE = 150.0f;    // drawn size in pixels (matches enemy size)

    // which sprite sheet to use
    enum class SpriteEffectType { SHIELD_BREAK, FRIENDLY_HEAL };

    // single sprite-sheet effect instance
    struct SpriteEffect
    {
        float x, y;                  // world position (centered)
        int   frame = 0;             // current frame index
        int   frame_count = 4;       // total frames on the sheet
        float timer = 0.0f;          // time accumulated on current frame
        float frame_dur = 0.25f;     // seconds per frame
        float alpha = 1.0f;          // overall transparency
        SpriteEffectType type = SpriteEffectType::SHIELD_BREAK;
        bool  active = false;        // false = free slot
    };

    SpriteEffect g_sprite_effects[MAX_SPRITE_EFFECTS]{};

    AEGfxTexture* g_tex_shield_break = nullptr;   // shieldbreaksheet.png
    AEGfxTexture* g_tex_friendly_heal = nullptr;  // friendlyeffect.png

    // builds a quad mesh whose UVs cover exactly one frame of a horizontal sprite sheet,
    // params u0 and u1 are the left and right UV edges of the frame (e.g. frame 2 of 4 = u0=0.5, u1=0.75),
    // caller is responsible for freeing the returned mesh after drawing - don't hold onto it
    AEGfxVertexList* BuildFrameQuad(float u0, float u1)
    {
        AEGfxMeshStart();
        AEGfxTriAdd(
            -0.5f, -0.5f, 0xFFFFFFFF, u0, 1.0f,
            0.5f, -0.5f, 0xFFFFFFFF, u1, 1.0f,
            -0.5f, 0.5f, 0xFFFFFFFF, u0, 0.0f);
        AEGfxTriAdd(
            0.5f, -0.5f, 0xFFFFFFFF, u1, 1.0f,
            0.5f, 0.5f, 0xFFFFFFFF, u1, 0.0f,
            -0.5f, 0.5f, 0xFFFFFFFF, u0, 0.0f);
        return AEGfxMeshEnd();
    }

    AEGfxVertexList* g_uv_quad_mesh = nullptr;   // kept for safe Free(), not used for drawing

    // ==============================================================
    //  State
    // ==============================================================
    Particle     g_particles[MAX_PARTICLES]{};   // fixed pool, no dynamic allocation
    ImpactLine   g_lines[MAX_IMPACT_LINES]{};    // fixed pool, no dynamic allocation
    ScreenShake  g_shake{};                      // single shared shake state
    FlashState   g_flash{};                      // single shared flash state

    AEGfxVertexList* g_quad_mesh = nullptr;      // 1x1 unit rect, reused for all particles and lines

    // scans the sprite effect pool for the first free slot, returns its index or -1 if the pool is full,
    // caller should always check for -1 before using the returned index
    int AllocSpriteEffect()
    {
        for (int i = 0; i < MAX_SPRITE_EFFECTS; ++i)
            if (!g_sprite_effects[i].active) return i;
        return -1;
    }



    // ===========================================================
    //  Utility helpers
    // ===========================================================

    // returns a random float between 0.0 and 1.0, used as a base for all randomised particle values
    float Rand01()
    {
        return (float)std::rand() / (float)RAND_MAX;
    }

    // returns a random float between a and b, used to scatter particle position, speed, size, and lifespan
    float RandRange(float a, float b)
    {
        return a + (b - a) * Rand01();
    }

    // scans the particle pool for the first free slot, returns its index or -1 if the pool is full,
    // using a pool avoids dynamic allocation - particles are reused in-place when they die
    int AllocParticle()
    {
        for (int i = 0; i < MAX_PARTICLES; ++i)
            if (!g_particles[i].active) return i; // return first free slot
        return -1; // pool full, caller should check for -1 before using
    }

    // scans the impact line pool for the first free slot, returns its index or -1 if the pool is full
    int AllocLine()
    {
        for (int i = 0; i < MAX_IMPACT_LINES; ++i)
            if (!g_lines[i].active) return i; // return first free slot
        return -1; // pool full, caller should check for -1 before using
    }

    // maps an enemy word length (3-8) to an RGB burst colour for the kill impact effect,
    // colours match the enemy sprite colour coding so the burst feels tied to the enemy type,
    // outputs via the r, g, b reference params - defaults to white for any out-of-range value
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

    // draws a solid colour square at the given world position using the shared unit mesh,
    // params are world x/y centre, size in pixels, rgb colour, and alpha for fade-out
    void DrawQuad(float x, float y, float size, float r, float g, float b, float alpha)
    {
        if (!g_quad_mesh) return; // safety check

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);        // enable alpha blending
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(r, g, b, alpha);    // apply color and alpha
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

        // build transform: scale to size, then translate to world pos
        AEMtx33 scale_m, trans_m, transform_m;
        AEMtx33Scale(&scale_m, size, size);         // scale unit quad to particle size
        AEMtx33Trans(&trans_m, x, y);               // move to world position
        AEMtx33Concat(&transform_m, &trans_m, &scale_m); // combine: scale first, then translate

        AEGfxSetTransform(transform_m.m);
        AEGfxMeshDraw(g_quad_mesh, AE_GFX_MDM_TRIANGLES);
    }

    // draws a 3px thick line between two world positions by scaling and rotating the shared unit mesh,
    // params are the start and end world positions, rgb colour, and alpha for fade-out
    void DrawLine(float x1, float y1, float x2, float y2,
        float r, float g, float b, float alpha)
    {
        if (!g_quad_mesh) return; // safety check

        float dx = x2 - x1;
        float dy = y2 - y1;
        float len = sqrtf(dx * dx + dy * dy); // length of the line
        if (len < 0.1f) return;               // skip degenerate lines

        float cx = (x1 + x2) * 0.5f;  // center x of line (for translation)
        float cy = (y1 + y2) * 0.5f;  // center y of line (for translation)
        float angle = atan2f(dy, dx);  // angle of line in radians (for rotation)

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);        // enable alpha blending
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(r, g, b, alpha);    // apply color and alpha
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

        // build transform: scale to line length and 3px thick, rotate to angle, translate to center
        AEMtx33 scale_m, rot_m, trans_m, temp_m, transform_m;
        AEMtx33Scale(&scale_m, len, 3.0f);               // stretch quad to line length, 3px thick
        AEMtx33Rot(&rot_m, angle);                        // rotate to match line direction
        AEMtx33Trans(&trans_m, cx, cy);                   // move to line center
        AEMtx33Concat(&temp_m, &rot_m, &scale_m);        // combine: scale then rotate
        AEMtx33Concat(&transform_m, &trans_m, &temp_m);  // combine: then translate

        AEGfxSetTransform(transform_m.m);
        AEGfxMeshDraw(g_quad_mesh, AE_GFX_MDM_TRIANGLES);
    }

} // anonymous namespace

// ============================================================================
//  Public
// ============================================================================

// builds the shared unit quad mesh used by all particle and line draws, clears all pool slots,
// loads the shield break and friendly heal sprite sheet textures, safe to call on restart
void Particles_Init()
{
    if (g_quad_mesh)
    {
        AEGfxMeshFree(g_quad_mesh); // release old mesh from GPU memory
        g_quad_mesh = nullptr;   // clear pointer before rebuilding
    }

    // build a simple 1x1 unit rectangle mesh (centered at origin), reused for all draws
    AEGfxMeshStart();

    // two triangles forming a quad
    AEGfxTriAdd(
        -0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 0.0f,
        0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 0.0f,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 1.0f);

    AEGfxTriAdd(
        0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 0.0f,
        0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 1.0f);

    g_quad_mesh = AEGfxMeshEnd();

    // clear all pool slots and reset shared state
    for (int i = 0; i < MAX_PARTICLES; ++i) g_particles[i].active = false;
    for (int i = 0; i < MAX_IMPACT_LINES; ++i) g_lines[i].active = false;
    for (int i = 0; i < MAX_SPRITE_EFFECTS; ++i) g_sprite_effects[i].active = false;
    g_shake = {};
    g_flash = {};

    // load on-kill sprite sheet textures
    g_tex_shield_break = AEGfxTextureLoad("Assets/shieldbreaksheet.png");
    g_tex_friendly_heal = AEGfxTextureLoad("Assets/friendlyeffect.png");
}

// frees the shared quad mesh and both sprite sheet textures, call this in Game_Exit() to avoid memory leaks
void Particles_Free()
{
    // free shared quad mesh if it exists
    if (g_quad_mesh)
    {
        AEGfxMeshFree(g_quad_mesh);
        g_quad_mesh = nullptr;
    }

    // free UV quad mesh used by sprite effects
    if (g_uv_quad_mesh)
    {
        AEGfxMeshFree(g_uv_quad_mesh);
        g_uv_quad_mesh = nullptr;
    }

    // free on-kill sprite sheet textures
    if (g_tex_shield_break) { AEGfxTextureUnload(g_tex_shield_break);  g_tex_shield_break = nullptr; }
    if (g_tex_friendly_heal) { AEGfxTextureUnload(g_tex_friendly_heal); g_tex_friendly_heal = nullptr; }
}

// advances all active particles, impact lines, sprite effects, screen shake and flash each frame,
// param is dt in seconds (should already be time-scaled from the caller if TimeSlow is active),
// also applies the camera shake offset via AEGfxSetCamPosition and resets it when shake ends
void Particles_Update(float dt)
{
    // --- Particles ---
    for (int i = 0; i < MAX_PARTICLES; ++i)
    {
        Particle& p = g_particles[i];
        if (!p.active) continue; // skip free slots

        p.life -= p.decay * dt;                  // fade out over time
        if (p.life <= 0.0f) {
            p.active = false;
            continue;
        } // dead, free slot for reuse

        p.vy -= PARTICLE_GRAVITY * dt;           // gravity pulls down each frame
        p.x += p.vx * dt;                        // move horizontally
        p.y += p.vy * dt;                        // move vertically
        p.vx *= (1.0f - (1.0f - PARTICLE_DRAG) * dt * 60.0f); //  horizontal drag
    }

    // --- Sprite effects (on-kill sheet animations) ---
    for (int i = 0; i < MAX_SPRITE_EFFECTS; ++i)
    {
        SpriteEffect& se = g_sprite_effects[i];
        if (!se.active) continue;                        // skip free slots

        se.timer += dt;
        if (se.timer >= se.frame_dur)                    // time to advance to next frame
        {
            se.timer -= se.frame_dur;                    // carry over excess time
            se.frame++;
            if (se.frame >= se.frame_count)              // all frames shown, effect is done
            {
                se.active = false;
                continue;
            }
        }

        // fade out across the last two frames for a smooth finish
        float frames_remaining = (float)(se.frame_count - se.frame);
        se.alpha = (frames_remaining <= 2.0f) ? (frames_remaining * 0.5f) : 1.0f;
    }

    // --- Impact lines ---
    for (int i = 0; i < MAX_IMPACT_LINES; ++i)
    {
        ImpactLine& l = g_lines[i];
        if (!l.active) continue; // skip free slots

        l.life -= l.decay * dt;              // fade out over time
        if (l.life <= 0.0f) l.active = false;// dead, free slot for reuse
    }

    // --- Screen shake ---
    if (g_shake.timer > 0.0f)
    {
        g_shake.timer -= dt;
        float t = g_shake.timer / g_shake.duration;          // 1(js start) to 0(end) as shake fades
        float intensity = g_shake.intensity * t;             // shake weakens over time
        g_shake.offset_x = RandRange(-intensity, intensity); // random x offset this frame
        g_shake.offset_y = RandRange(-intensity * 0.5f, intensity * 0.5f); // smaller y offset

        if (g_shake.timer <= 0.0f)
        {
            // shake finished, reset offsets
            g_shake.timer = 0.0f;
            g_shake.offset_x = 0.0f;
            g_shake.offset_y = 0.0f;
        }
        AEGfxSetCamPosition(g_shake.offset_x, g_shake.offset_y); // move camera to create shake
    }
    else
    {
        AEGfxSetCamPosition(0.0f, 0.0f); // ensure camera is reset when shake is done
    }

    // --- Flash ---
    if (g_flash.timer > 0.0f)
    {
        g_flash.timer -= dt;
        if (g_flash.timer < 0.0f) g_flash.timer = 0.0f;   // clamp so it doesn't go negative

        float t = g_flash.timer / g_flash.duration;        // 1..0 as flash fades
        g_flash.alpha = DAMAGE_FLASH_PEAK_ALPHA * t;       // fade alpha toward 0 (flash in draw)
    }
}

// renders all active impact lines, particles, sprite sheet effects, and the screen flash overlay,
// drawn in that order so lines are behind particles, particles are behind sprites, flash is on top,
// call this after drawing enemies and before drawing UI so effects sit between them
void Particles_Draw()
{
    if (!g_quad_mesh) return; // safety check, mesh must be initialised

    // --- Impact lines (drawn first so particles on top) ---
    for (int i = 0; i < MAX_IMPACT_LINES; ++i)
    {
        ImpactLine& l = g_lines[i];
        if (!l.active) continue;
        DrawLine(l.x1, l.y1, l.x2, l.y2, l.r, l.g, l.b, l.life); // life used as alpha
    }

    // --- Particles ---
    for (int i = 0; i < MAX_PARTICLES; ++i)
    {
        Particle& p = g_particles[i];
        if (!p.active) continue;
        DrawQuad(p.x, p.y, p.size, p.r, p.g, p.b, p.life); // life used as alpha
    }

    // --- Sprite effects (on-kill sheet animations) ---
    for (int i = 0; i < MAX_SPRITE_EFFECTS; ++i)
    {
        SpriteEffect& se = g_sprite_effects[i];
        if (!se.active) continue;

        AEGfxTexture* tex = (se.type == SpriteEffectType::SHIELD_BREAK)
            ? g_tex_shield_break
            : g_tex_friendly_heal;
        if (!tex) continue;                              // texture failed to load, skip silently

        // calculate UV range for the current frame only
        float frame_w = 1.0f / (float)se.frame_count;
        float u0 = (float)se.frame * frame_w;            // left UV edge of this frame
        float u1 = u0 + frame_w;                         // right UV edge of this frame

        // build a fresh mesh whose UVs cover exactly this frame's slice of the sheet
        AEGfxVertexList* frame_mesh = BuildFrameQuad(u0, u1);
        if (!frame_mesh) continue;

        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(se.alpha);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        AEGfxTextureSet(tex, 0.0f, 0.0f);               // no offset needed; UVs are baked into the mesh

        AEMtx33 scale_m, trans_m, transform_m;
        AEMtx33Scale(&scale_m, SPRITE_EFFECT_SIZE, SPRITE_EFFECT_SIZE);
        AEMtx33Trans(&trans_m, se.x, se.y);
        AEMtx33Concat(&transform_m, &trans_m, &scale_m);

        AEGfxSetTransform(transform_m.m);
        AEGfxMeshDraw(frame_mesh, AE_GFX_MDM_TRIANGLES);
        AEGfxMeshFree(frame_mesh);                       // free immediately after draw
    }

    // --- Screen flash (drawn last so it overlays everything) ---
    if (g_flash.alpha > 0.0f && g_quad_mesh)
    {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, g_flash.alpha); // white overlay with fading alpha
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

        AEMtx33 transform_m;
        AEMtx33Scale(&transform_m, 1600.0f, 900.0f); // scale quad to full screen size

        AEGfxSetTransform(transform_m.m);
        AEGfxMeshDraw(g_quad_mesh, AE_GFX_MDM_TRIANGLES);
    }

    // reset render state cleanly so subsequent draw calls aren't affected
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxSetTransparency(1.0f);
}

// spawns the full damage combo effect at the given world position - grey/white dust particles,
// radiating yellow impact lines, a brief white screen flash, and a camera shake,
// call this when an enemy lands and deals damage to the player
void Particles_SpawnDamageImpact(float x, float y)
{
    // --- Dust particles ---
    // three shades of grey/white, cycled across all dust particles
    const float dust_colors[3][3] = {
        { 0.90f, 0.90f, 0.90f },   // light grey
        { 1.00f, 1.00f, 1.00f },   // white
        { 0.70f, 0.70f, 0.70f },   // mid grey
    };

    for (int i = 0; i < DAMAGE_DUST_COUNT; ++i)
    {
        int slot = AllocParticle();  // find a free slot in the pool
        if (slot < 0) break;         // pool full, stop spawning

        Particle& p = g_particles[slot];
        p.active = true;
        p.x = x + RandRange(-12.0f, 12.0f);            // scatter spawn pos around impact point
        p.y = y + RandRange(-4.0f, 4.0f);
        p.vx = RandRange(-DAMAGE_DUST_SPEED, DAMAGE_DUST_SPEED); // random horizontal direction
        p.vy = RandRange(0.0f, DAMAGE_DUST_UPSPEED);             // always upward, random strength
        p.size = RandRange(DAMAGE_PARTICLE_SIZE_MIN, DAMAGE_PARTICLE_SIZE_MAX); // random size
        p.life = 1.0f;                                           // start fully alive
        p.decay = 1.0f / (DAMAGE_PARTICLE_LIFE * RandRange(0.7f, 1.3f)); // randomise lifespan slightly

        int ci = i % 3;              // cycle through the 3 dust colors
        p.r = dust_colors[ci][0];
        p.g = dust_colors[ci][1];
        p.b = dust_colors[ci][2];
    }

    // --- Impact lines (yellow, radiate outward horizontally) ---
    for (int i = 0; i < DAMAGE_LINE_COUNT; ++i)
    {
        int slot = AllocLine(); // find a free slot in the pool
        if (slot < 0) break;    // pool full, stop spawning

        ImpactLine& l = g_lines[slot];
        l.active = true;

        float angle = RandRange(-0.4f, 0.4f);  // near horizontal angle with slight random variation
        if (i % 2 == 0) angle += 3.14159f;     // alternate sides: even = right, odd = left

        float length = RandRange(DAMAGE_LINE_LENGTH_MIN, DAMAGE_LINE_LENGTH_MAX); // random line length
        float near_dist = 10.0f;               // gap between impact center and line start

        // calculate start and end points along the angle
        l.x1 = x + cosf(angle) * near_dist;
        l.y1 = y + sinf(angle) * near_dist;
        l.x2 = x + cosf(angle) * (near_dist + length);
        l.y2 = y + sinf(angle) * (near_dist + length);

        l.r = 1.0f; l.g = 0.85f; l.b = 0.1f; // yellow color
        l.life = 1.0f;                         // start fully alive
        l.decay = 1.0f / DAMAGE_LINE_LIFE;     // decay so line fades over DAMAGE_LINE_LIFE seconds
        l.active = true;
    }

    // --- Screen flash ---
    g_flash.timer = DAMAGE_FLASH_DURATION;    // start flash countdown
    g_flash.duration = DAMAGE_FLASH_DURATION; // store for fade calculation
    g_flash.alpha = DAMAGE_FLASH_PEAK_ALPHA;  // start at max opacity

    // --- Screen shake ---
    g_shake.timer = DAMAGE_SHAKE_DURATION;    // start shake countdown
    g_shake.duration = DAMAGE_SHAKE_DURATION; // store for fade calculation
    g_shake.intensity = DAMAGE_SHAKE_INTENSITY; // max camera offset
}

// spawns a coloured burst of particles at the given world position when the player kills an enemy,
// colour is chosen by word_length (3=blue, 4=green, 5=yellow, 6=orange, 7=red, 8=purple),
// no shake or flash since this is a positive reward event, not a punishment
void Particles_SpawnKillImpact(float x, float y, int word_length)
{
    // get primary burst color based on word length (3=blue, 4=green ... 8=purple)
    float cr, cg, cb;
    WordLengthToColor(word_length, cr, cg, cb);

    // slightly lighter secondary color for variety, alternated with primary
    float cr2 = cr * 0.6f + 0.4f;
    float cg2 = cg * 0.6f + 0.4f;
    float cb2 = cb * 0.6f + 0.4f;

    for (int i = 0; i < KILL_BURST_COUNT; ++i)
    {
        int slot = AllocParticle(); // find a free slot in the pool
        if (slot < 0) break;        // pool full, stop spawning

        Particle& p = g_particles[slot];
        p.active = true;
        p.x = x + RandRange(-10.0f, 10.0f); // scatter spawn pos around impact point
        p.y = y + RandRange(-4.0f, 4.0f);

        // full 360 degree burst with upward bias
        float angle = RandRange(-3.14159f, 3.14159f);           // random direction in full circle
        float speed = RandRange(KILL_BURST_SPEED * 0.4f, KILL_BURST_SPEED); // random speed
        p.vx = cosf(angle) * speed;                              // horizontal component
        p.vy = fabsf(sinf(angle)) * KILL_BURST_UPSPEED + RandRange(30.0f, 80.0f); // always upward

        p.size = RandRange(KILL_PARTICLE_SIZE_MIN, KILL_PARTICLE_SIZE_MAX); // random size
        p.life = 1.0f;                                           // start fully alive
        p.decay = 1.0f / (KILL_PARTICLE_LIFE * RandRange(0.7f, 1.3f)); // randomise lifespan slightly

        // alternate between primary and lighter secondary color for visual variety
        if (i % 2 == 0) { p.r = cr;  p.g = cg;  p.b = cb; }
        else { p.r = cr2; p.g = cg2; p.b = cb2; }
    }
    // note: no screen shake or flash � kill is a reward event, not a punishment
}

// starts a screen shake with the given duration in seconds and max pixel intensity,
// overwrites any currently running shake - safe to call at any time,
// used directly by lightning powerup and other systems that need custom shake strength
void Particles_TriggerShake(float duration, float intensity)
{
    // overwrite any ongoing shake with new values
    g_shake.timer = duration;
    g_shake.duration = duration;
    g_shake.intensity = intensity;
}

// plays the shield break sprite sheet animation centred on the enemy position,
// call this when a shielded enemy's shield is broken by the player typing the first word,
// silently skips if the sprite effect pool is full
void Particles_SpawnShieldBreak(float x, float y)
{
    int slot = AllocSpriteEffect();
    if (slot < 0) return;                              // pool full, skip silently

    SpriteEffect& se = g_sprite_effects[slot];
    se.active = true;
    se.x = x;
    se.y = y;
    se.frame = 0;
    se.frame_count = 4;                                // shieldbreaksheet.png has 4 frames
    se.timer = 0.0f;
    se.frame_dur = SHIELD_BREAK_FRAME_DUR;
    se.alpha = 1.0f;
    se.type = SpriteEffectType::SHIELD_BREAK;
}

// plays the friendly heal sprite sheet animation centred on the enemy position,
// call this when a friendly enemy is typed and flies away, giving the player health,
// silently skips if the sprite effect pool is full
void Particles_SpawnFriendlyHeal(float x, float y)
{
    int slot = AllocSpriteEffect();
    if (slot < 0) return;                              // pool full, skip silently

    SpriteEffect& se = g_sprite_effects[slot];
    se.active = true;
    se.x = x;
    se.y = y;
    se.frame = 0;
    se.frame_count = 4;                                // friendlyeffect.png has 4 frames
    se.timer = 0.0f;
    se.frame_dur = FRIENDLY_HEAL_FRAME_DUR;
    se.alpha = 1.0f;
    se.type = SpriteEffectType::FRIENDLY_HEAL;
}
#pragma warning(pop)