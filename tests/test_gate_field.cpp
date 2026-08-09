#include <doctest/doctest.h>

#include "gameplay/gate_field.h"

using flappy::Gate;
using flappy::GameTuning;
using flappy::GateField;

static constexpr float kDt = 1.0f / 60.0f;

TEST_CASE("the first step spawns a gate at the right edge") {
    GameTuning tuning;
    GateField  field;
    field.reset(1u);

    CHECK(field.active_count() == 0);
    field.step(tuning, kDt, tuning.scroll_speed);
    CHECK(field.active_count() == 1);
}

TEST_CASE("gates scroll left at the given speed") {
    GameTuning tuning;
    GateField  field;
    field.reset(1u);
    field.step(tuning, kDt, tuning.scroll_speed);

    float before = 0.0f;
    for (const Gate& gate : field.gates) {
        if (gate.active) before = gate.x;
    }

    field.step(tuning, kDt, tuning.scroll_speed);

    float after = 0.0f;
    for (const Gate& gate : field.gates) {
        if (gate.active) after = gate.x;
    }

    CHECK(before - after == doctest::Approx(tuning.scroll_speed * kDt));
}

TEST_CASE("a faster scroll speed moves gates further per step") {
    GameTuning tuning;

    GateField slow;
    slow.reset(1u);
    slow.step(tuning, kDt, 420.0f);
    slow.step(tuning, kDt, 420.0f);

    GateField fast;
    fast.reset(1u);
    fast.step(tuning, kDt, 620.0f);
    fast.step(tuning, kDt, 620.0f);

    CHECK(fast.gates[0].x < slow.gates[0].x);
}

TEST_CASE("gates are retired once fully off the left edge") {
    GameTuning tuning;
    GateField  field;
    field.reset(1u);

    // Run long enough for the first gate to cross the whole screen and leave.
    for (int i = 0; i < 60; ++i) {
        field.step(tuning, kDt, tuning.scroll_speed);
    }
    const int early = field.active_count();

    for (int i = 0; i < 300; ++i) {
        field.step(tuning, kDt, tuning.scroll_speed);
    }

    CHECK(early >= 1);
    CHECK(field.active_count() <= GateField::kMaxGates);
}

TEST_CASE("the pool never overflows over a long run") {
    GameTuning tuning;
    GateField  field;
    field.reset(7u);

    // Five minutes at 60 Hz. Without retirement this would need 150 gates.
    for (int i = 0; i < 18000; ++i) {
        field.step(tuning, kDt, tuning.scroll_speed);
        CHECK(field.active_count() <= GateField::kMaxGates);
    }
}

TEST_CASE("gates spawn on the configured interval") {
    GameTuning tuning;
    GateField  field;
    field.reset(1u);

    int spawns   = 0;
    int previous = 0;
    // Ten seconds should produce ceil(10 / 2.0) = 5 or 6 spawns.
    for (int i = 0; i < 600; ++i) {
        field.step(tuning, kDt, tuning.scroll_speed);
        const int now = field.active_count();
        if (now > previous) ++spawns;
        previous = now;
    }
    CHECK(spawns >= 5);
}

TEST_CASE("the same seed produces the same course") {
    GameTuning tuning;

    GateField a;
    GateField b;
    a.reset(4242u);
    b.reset(4242u);

    for (int i = 0; i < 600; ++i) {
        a.step(tuning, kDt, tuning.scroll_speed);
        b.step(tuning, kDt, tuning.scroll_speed);
        for (int g = 0; g < GateField::kMaxGates; ++g) {
            CHECK(a.gates[g].x == b.gates[g].x);
            CHECK(a.gates[g].gap_center == b.gates[g].gap_center);
        }
    }
}