#pragma once
namespace xiffects {
// Four-point Catmull-Rom interpolation. Same nominal read position as linear;
// no extra delay/window or allocation. Not an anti-alias filter for large shifts.
inline float cubicInterpolate(float previous,float current,float next,float after,float t) {
    const float a=.5f*(after-previous)+1.5f*(current-next);
    const float b=previous-2.5f*current+2.f*next-.5f*after;
    const float c=.5f*(next-previous);
    return ((a*t+b)*t+c)*t+current;
}
}
