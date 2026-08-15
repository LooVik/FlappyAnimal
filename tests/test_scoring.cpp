#include <doctest/doctest.h>

#include "gameplay/scoring.h"

using flappy::GameTuning;
using flappy::GateField;

// Player anchor sits at 1080 * 0.28 = 302.4. A gate is 140 wide, so its right
// edge clears the anchor once gate.x drops below 162.4.
static GateField field_with_gate_at(float x) {
    GateField field;
    field.gates[0].x          = x;
    field.gates[0].gap_center = 800.0f;
    field.gates[0].active     = true;
    field.gates[0].scored     = false;
    return field;
}

TEST_CASE("a gate still ahead of the player scores nothing") {
    GameTuning tuning;
    GateField  field = field_with_gate_at(600.0f);
    CHECK(flappy::score_passed_gates(tuning, field) == 0);
    CHECK_FALSE(field.gates[0].scored);
}

TEST_CASE("a gate the player has cleared scores exactly one") {
    GameTuning tuning;
    GateField  field = field_with_gate_at(100.0f);
    CHECK(flappy::score_passed_gates(tuning, field) == 1);
    CHECK(field.gates[0].scored);
}

TEST_CASE("the same gate never scores twice") {
    GameTuning tuning;
    GateField  field = field_with_gate_at(100.0f);

    CHECK(flappy::score_passed_gates(tuning, field) == 1);

    // Sixty more frames of the player sitting past it — the real failure mode.
    int extra = 0;
    for (int i = 0; i < 60; ++i) {
        extra += flappy::score_passed_gates(tuning, field);
    }
    CHECK(extra == 0);
}

TEST_CASE("a gate exactly on the scoring line has not been cleared yet") {
    GameTuning tuning;
    // Right edge lands exactly on the anchor: 302.4 - 140 = 162.4
    GateField field = field_with_gate_at(302.4f - tuning.gate_width);
    CHECK(flappy::score_passed_gates(tuning, field) == 0);
}

TEST_CASE("an inactive gate never scores") {
    GameTuning tuning;
    GateField  field = field_with_gate_at(100.0f);
    field.gates[0].active = false;
    CHECK(flappy::score_passed_gates(tuning, field) == 0);
}

TEST_CASE("several gates can clear in the same step") {
    GameTuning tuning;
    GateField  field;
    for (int i = 0; i < 3; ++i) {
        field.gates[i].x          = 50.0f + static_cast<float>(i) * 10.0f;
        field.gates[i].gap_center = 800.0f;
        field.gates[i].active     = true;
    }
    CHECK(flappy::score_passed_gates(tuning, field) == 3);
}

TEST_CASE("a full run scores one point per gate, no more") {
    GameTuning tuning;
    GateField  field;
    field.reset(1234u);

    int total  = 0;
    int spawns = 0;
    int active_before = 0;

    // Twenty seconds at 60 Hz.
    for (int i = 0; i < 1200; ++i) {
        field.step(tuning, 1.0f / 60.0f, tuning.scroll_speed);
        if (field.active_count() > active_before) ++spawns;
        active_before = field.active_count();
        total += flappy::score_passed_gates(tuning, field);
    }

    // Every gate that spawned and left the screen scored exactly once, so the
    // total can never exceed the number of gates ever created.
    CHECK(total > 0);
    CHECK(total <= spawns + GateField::kMaxGates);
}