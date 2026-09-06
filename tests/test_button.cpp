#include <doctest/doctest.h>

#include "ui/button.h"

using flappy::Button;
using flappy::Rect;

// A button spanning x 100..400, y 200..280.
static Button sample() {
    Button button;
    button.bounds = Rect{100.0f, 200.0f, 300.0f, 80.0f};
    return button;
}

TEST_CASE("a point inside the bounds is a hit") {
    const Button button = sample();

    CHECK(button.contains(100.0f, 200.0f));   // top-left corner counts as inside
    CHECK(button.contains(250.0f, 240.0f));   // dead centre
    CHECK(button.contains(399.9f, 279.9f));   // a hair inside the far corner
}

TEST_CASE("a point outside the bounds is a miss") {
    const Button button = sample();

    CHECK_FALSE(button.contains(99.0f,  240.0f));   // left of it
    CHECK_FALSE(button.contains(250.0f, 199.0f));   // above it
    CHECK_FALSE(button.contains(401.0f, 240.0f));   // right of it
    CHECK_FALSE(button.contains(250.0f, 281.0f));   // below it
}

TEST_CASE("the far edges are outside, so neighbouring buttons never both fire") {
    // Two buttons sharing an edge at x = 400. This is the case the half-open
    // rule exists for: without it, one tap would trigger both.
    Button left  = sample();
    Button right = sample();
    right.bounds.x = 400.0f;

    CHECK_FALSE(left.contains(400.0f, 240.0f));
    CHECK(right.contains(400.0f, 240.0f));
}

TEST_CASE("a disabled button swallows nothing") {
    Button button  = sample();
    button.enabled = false;

    CHECK_FALSE(button.contains(250.0f, 240.0f));
}

TEST_CASE("centered_button puts the middle where asked") {
    const Button button = flappy::centered_button("CLOSE", 540.0f, 1000.0f, 400.0f, 120.0f);

    CHECK(button.label == "CLOSE");
    CHECK(button.bounds.x == doctest::Approx(340.0f));    // 540 - 400/2
    CHECK(button.bounds.y == doctest::Approx(940.0f));    // 1000 - 120/2
    CHECK(button.bounds.w == doctest::Approx(400.0f));
    CHECK(button.bounds.h == doctest::Approx(120.0f));

    // The centre we asked for must actually land inside it.
    CHECK(button.contains(540.0f, 1000.0f));
}
