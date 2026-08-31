#include <doctest/doctest.h>

#include "app/run.h"

using flappy::GameTuning;
using flappy::GateField;
using flappy::Run;
using flappy::Screen;

static constexpr float kDt = 1.0f / 60.0f;

TEST_CASE("reset returns every field to a fresh state") {
    GameTuning tuning;
    Run        run;

    // Dirty everything a real run would have touched.
    run.screen            = Screen::Dying;
    run.score             = 42;
    run.dead_timer        = 0.9f;
    run.step_count        = 1234;
    run.player.y          = 10.0f;
    run.player.velocity_y = 500.0f;
    run.field.step(tuning, kDt, tuning.scroll_speed, tuning.gate_gap);

    run.reset(tuning, 7u);

    CHECK(run.screen == Screen::Ready);
    CHECK(run.score == 0);
    CHECK(run.dead_timer == doctest::Approx(0.0f));
    CHECK(run.step_count == 0);
    CHECK(run.player.velocity_y == doctest::Approx(0.0f));
    CHECK(run.player.y == doctest::Approx(tuning.reference_height * 0.5f));
    CHECK(run.field.active_count() == 0);
}

TEST_CASE("the same seed produces the same course after reset") {
    GameTuning tuning;
    Run a;
    Run b;

    a.reset(tuning, 99u);
    b.reset(tuning, 99u);

    for (int i = 0; i < 300; ++i) {
        a.field.step(tuning, kDt, tuning.scroll_speed, tuning.gate_gap);
        b.field.step(tuning, kDt, tuning.scroll_speed, tuning.gate_gap);
    }

    for (int g = 0; g < GateField::kMaxGates; ++g) {
        CHECK(a.field.gates[g].x == b.field.gates[g].x);
        CHECK(a.field.gates[g].gap_center == b.field.gates[g].gap_center);
    }
}