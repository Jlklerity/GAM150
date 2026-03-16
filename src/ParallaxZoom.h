#pragma once

namespace ParallaxZoom
{
    // Total duration of the zoom + fade (seconds)
    static const float ZOOM_DURATION = 2.0f;

    void Start(int targetState);
    void Update(float dt);
    void Draw(float vCamY);
    bool IsActive();
    void Reset();
}