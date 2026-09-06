#pragma once

namespace flappy {

// An axis-aligned rectangle in view coordinates: x,y is the TOP-LEFT corner,
// and +y points DOWN — the screen convention, not Unity's.
//
// This sits on its own rather than inside collision.h because two unrelated
// systems need it now: collision (where the bird and gates are) and the UI
// (where buttons are). Leaving it in collision.h would force every button to
// include gate and player headers it has no use for.
struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
};

}  // namespace flappy
