#include <doctest/doctest.h>

#include "gameplay/difficulty.h"
#include "gameplay/gate_field.h"

using flappy::GameTuning;
using flappy::GateField;

TEST_CASE("difficulty does not move until the first ten points") {
    GameTuning tuning;
    for (int score = 0; score < 10; ++score) {
        CHECK(flappy::current_scroll_speed(tuning, score)
              == doctest::Approx(tuning.scroll_speed));
        CHECK(flappy::current_gate_gap(tuning, score)
              == doctest::Approx(tuning.gate_gap));
    }
}

TEST_CASE("every ten points ramps speed up and the gap down") {
    GameTuning tuning;
    CHECK(flappy::current_scroll_speed(tuning, 10) == doctest::Approx(440.0f));
    CHECK(flappy::current_gate_gap(tuning, 10)     == doctest::Approx(428.0f));
    CHECK(flappy::current_scroll_speed(tuning, 25) == doctest::Approx(460.0f));
    CHECK(flappy::current_gate_gap(tuning, 25)     == doctest::Approx(416.0f));
}

TEST_CASE("the ramp stops at the configured limits") {
    GameTuning tuning;
    CHECK(flappy::current_scroll_speed(tuning, 100000)
          == doctest::Approx(tuning.max_scroll_speed));
    CHECK(flappy::current_gate_gap(tuning, 100000)
          == doctest::Approx(tuning.min_gate_gap));
}

TEST_CASE("difficulty never exceeds its bounds at any score") {
    GameTuning tuning;
    for (int score = 0; score < 5000; ++score) {
        const float speed = flappy::current_scroll_speed(tuning, score);
        const float gap   = flappy::current_gate_gap(tuning, score);
        CHECK(speed >= tuning.scroll_speed);
        CHECK(speed <= tuning.max_scroll_speed);
        CHECK(gap   <= tuning.gate_gap);
        CHECK(gap   >= tuning.min_gate_gap);
    }
}

TEST_CASE("difficulty is monotonic - it never gets easier") {
    GameTuning tuning;
    for (int score = 1; score < 500; ++score) {
        CHECK(flappy::current_scroll_speed(tuning, score)
              >= flappy::current_scroll_speed(tuning, score - 1));
        CHECK(flappy::current_gate_gap(tuning, score)
              <= flappy::current_gate_gap(tuning, score - 1));
    }
}

TEST_CASE("a gate keeps the gap it was born with when difficulty ramps") {
    GameTuning tuning;
    GateField  field;
    field.reset(1u);

    field.step(tuning, 1.0f / 60.0f, tuning.scroll_speed, tuning.gate_gap);
    const float born_with = field.gates[0].gap;
    CHECK(born_with == doctest::Approx(tuning.gate_gap));

    // The player now scores 50, so new gates would be much narrower...
    const float narrowed = flappy::current_gate_gap(tuning, 50);
    CHECK(narrowed < tuning.gate_gap);

    // ...but the gate already in flight must not change shape underneath them.
    for (int i = 0; i < 30; ++i) {
        field.step(tuning, 1.0f / 60.0f,
                   flappy::current_scroll_speed(tuning, 50), narrowed);
    }
    CHECK(field.gates[0].gap == doctest::Approx(born_with));
}