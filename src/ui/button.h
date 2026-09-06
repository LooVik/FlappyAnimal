#pragma once

#include "core/rect.h"

#include <string>

namespace flappy {

// A tappable region with a label.
//
// The Unity comparison: this is the RectTransform — not the Image, not the
// Button component. It knows WHERE it is and WHAT IT SAYS, and nothing about
// how it is drawn or what a press does. Drawing lives in main.cpp because that
// needs SFML; what a press does lives in the screen transitions. Keeping those
// three apart is why this file can be unit tested with no window open at all.
struct Button {
    Rect        bounds{};
    std::string label;
    bool        enabled = true;

    // Is this point inside the button?
    //
    // The coordinates are VIEW coordinates (0..reference_width by
    // 0..reference_height), never raw screen pixels. The caller converts,
    // because the letterbox means a pixel and a view coordinate are not the
    // same thing — a tap at pixel (0,0) is inside the black bar, not the game.
    bool contains(float x, float y) const;
};

// Build a button centred horizontally on cx with its middle at cy.
// Every panel in this game is centre-aligned, so laying buttons out from their
// centre is less arithmetic at each call site than from a corner.
Button centered_button(std::string label, float cx, float cy, float w, float h);

}  // namespace flappy
