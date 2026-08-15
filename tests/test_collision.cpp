#include <doctest/doctest.h>

#include "gameplay/collision.h"

using flappy::Gate;
using flappy::GameTuning;
using flappy::GateField;
using flappy::Player;
using flappy::Rect;

// A gate positioned so it definitely overlaps the player horizontally.
// Player centre x = 1080 * 0.28 = 302.4, so a gate at x=280 spans 280..420.
static GateField field_with_gate(float gap_center) {
    GateField field;
    field.gates[0].x          = 280.0f;
    field.gates[0].gap_center = gap_center;
    field.gates[0].gap        = 440.0f;
    field.gates[0].active     = true;
    return field;
}

TEST_CASE("overlapping rectangles are detected") {
    CHECK(flappy::overlaps(Rect{0, 0, 10, 10}, Rect{5, 5, 10, 10}));
    CHECK_FALSE(flappy::overlaps(Rect{0, 0, 10, 10}, Rect{20, 20, 10, 10}));
}

TEST_CASE("rectangles that merely touch are not a hit") {
    // Right edge of a at x=10, left edge of b at x=10. A perfect skim.
    CHECK_FALSE(flappy::overlaps(Rect{0, 0, 10, 10}, Rect{10, 0, 10, 10}));
}

TEST_CASE("the collision body is 80 percent of the sprite, same centre") {
    GameTuning tuning;
    Player     player;
    player.y = 900.0f;

    const Rect sprite = flappy::player_sprite(tuning, player);
    const Rect body   = flappy::player_body(tuning, player);

    CHECK(body.w == doctest::Approx(sprite.w * 0.8f));
    CHECK(body.h == doctest::Approx(sprite.h * 0.8f));
    CHECK(body.x + body.w * 0.5f == doctest::Approx(sprite.x + sprite.w * 0.5f));
    CHECK(body.y + body.h * 0.5f == doctest::Approx(sprite.y + sprite.h * 0.5f));
}

TEST_CASE("flying through the middle of a gap is safe") {
    GameTuning tuning;
    Player     player;
    player.y = 800.0f;
    CHECK_FALSE(flappy::hits_any_gate(tuning, player, field_with_gate(800.0f)));
}

TEST_CASE("flying into the top pillar ends the run") {
    GameTuning tuning;
    Player     player;
    player.y = 500.0f;   // gap runs 580..1020, so 500 is inside the top pillar
    CHECK(flappy::hits_any_gate(tuning, player, field_with_gate(800.0f)));
}

TEST_CASE("flying into the bottom pillar ends the run") {
    GameTuning tuning;
    Player     player;
    player.y = 1100.0f;
    CHECK(flappy::hits_any_gate(tuning, player, field_with_gate(800.0f)));
}

TEST_CASE("the 80 percent body forgives a visual clip") {
    GameTuning tuning;
    Player     player;

    // Gap top edge is at 800 - 220 = 580.
    // Sprite is 64 tall, body 51.2. At y = 607 the SPRITE reaches up to 575 —
    // five pixels inside the pillar — while the BODY stops at 581.4.
    player.y = 607.0f;

    const Gate& gate = field_with_gate(800.0f).gates[0];
    const Rect  top  = flappy::gate_top_body(tuning, gate);

    CHECK(flappy::overlaps(flappy::player_sprite(tuning, player), top));  // looks like a hit
    CHECK_FALSE(flappy::hits_any_gate(tuning, player, field_with_gate(800.0f)));  // isn't one
}

TEST_CASE("an inactive gate never collides") {
    GameTuning tuning;
    Player     player;
    player.y = 500.0f;

    GateField field = field_with_gate(800.0f);
    field.gates[0].active = false;

    CHECK_FALSE(flappy::hits_any_gate(tuning, player, field));
}

TEST_CASE("the ceiling and floor end the run") {
    GameTuning tuning;

    Player high;
    high.y = 20.0f;      // body top at -5.6
    CHECK(flappy::hits_boundary(tuning, high));

    Player low;
    low.y = 1900.0f;     // body bottom at 1925.6
    CHECK(flappy::hits_boundary(tuning, low));

    Player safe;
    safe.y = 900.0f;
    CHECK_FALSE(flappy::hits_boundary(tuning, safe));
}