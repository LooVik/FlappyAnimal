#include "ui/button.h"

#include <utility>

namespace flappy {

bool Button::contains(float x, float y) const {
    if (!enabled) {
        return false;   // a disabled button is not a hole in the screen either
    }

    // Half-open on both axes: the left and top edges count as inside, the
    // right and bottom edges do not. That is the same rule overlaps() uses in
    // collision.cpp, and it matters the moment two buttons sit edge to edge —
    // a tap on the shared boundary must hit exactly one of them, never both.
    return x >= bounds.x && x < bounds.x + bounds.w &&
           y >= bounds.y && y < bounds.y + bounds.h;
}

Button centered_button(std::string label, float cx, float cy, float w, float h) {
    Button button;
    button.bounds = Rect{cx - w * 0.5f, cy - h * 0.5f, w, h};
    // std::move because label was passed by value: we own that string already,
    // so copying it into the struct would be a second allocation for nothing.
    button.label  = std::move(label);
    return button;
}

}  // namespace flappy
