#include <doctest/doctest.h>

#include "gameplay/player.h"

using flappy::GameTuning;
using flappy::Player;

static constexpr float kDt = 1.0f / 60.0f;

TEST_CASE("gravity accelerates the player downward") {
    GameTuning t;
    Player p;
    p.step(t, kDt);
    CHECK(p.velocity_y == doctest::Approx(t.gravity * kDt));
    CHECK(p.y > 0.0f);
}

TEST_CASE("tap replace velocity rather than adding to it") {
    GameTuning t;

    Player falling;
    falling.velocity_y = 900.0f;
    falling.flap(t);

    Player rising;
    rising.velocity_y = -400.0f;
    rising.flap(t);

    CHECK(falling.velocity_y == doctest::Approx(t.tap_impulse));
    CHECK(rising.velocity_y == doctest::Approx(t.tap_impulse));

}

TEST_CASE("fall speed is clamped to terminal velocity") {
    GameTuning t;
    Player p;

    for (int i = 0; i < 600; ++i)
    {
        p.step(t, kDt);
    }
    CHECK(p.velocity_y == doctest::Approx((t.max_fall_speed)));
    CHECK(p.velocity_y < t.gravity);

}

TEST_CASE("the clamp never limits upward motion") {
    GameTuning t;
    Player p;
    p.flap(t);
    p.step(t, kDt);
    CHECK(p.velocity_y < 0.0f);
    CHECK(p.velocity_y > t.tap_impulse);

}

TEST_CASE("a dlap arrests a fall in a single step") {
    GameTuning t;
    Player p;

    for (int i = 0; i < 60; ++i) 
    {
        p.step(t, kDt);
    }

    const float before = p.y;
    p.flap(t);
    p.step(t, kDt);
    CHECK(p.y < before);
}