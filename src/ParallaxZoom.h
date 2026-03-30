#pragma once

namespace ParallaxZoom
{
    // total duration of the zoom + fade in seconds
    static const float ZOOM_DURATION = 2.0f;

    // starts the zoom transition, param is the game state to switch to mid-transition
    void Start(int target_state);

    // advances the timer and triggers the state switch at 98% through, param is dt in seconds
    void Update(float dt);

    // draws the zooming parallax layers and fade overlay, param is the vertical camera offset
    void Draw(float v_cam_y);

    // returns true while the transition is running
    bool IsActive();

    // cancels the transition and frees meshes immediately
    void Reset();
}