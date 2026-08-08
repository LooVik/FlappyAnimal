#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "app/time_step.h"

using flappy::FixedTimestep;

static constexpr double kStep = 1.0 / 60.0;

TEST_CASE("exactly one step of time yields exactly one step") {
    FixedTimestep ts;
    CHECK(ts.accumulate(kStep) == 1);
}

TEST_CASE("less than one step yields no steps and banks the time") {
    FixedTimestep ts;
    CHECK(ts.accumulate(kStep * 0.5) == 0);
    CHECK(ts.accumulator == doctest::Approx(kStep * 0.5));
}

TEST_CASE("banked time carries across calls") {
    FixedTimestep ts;
    CHECK(ts.accumulate(kStep * 0.6) == 0);   // 0.6 banked
    CHECK(ts.accumulate(kStep * 0.6) == 1);   // 1.2 banked -> spend one
    CHECK(ts.accumulator == doctest::Approx(kStep * 0.2));
}

TEST_CASE("a long frame yields several steps") {
    FixedTimestep ts;
    CHECK(ts.accumulate(kStep * 3.0) == 3);
    CHECK(ts.accumulator == doctest::Approx(0.0));
}

TEST_CASE("simulated time does not drift from real time") {
    FixedTimestep ts;
    int total = 0;
    for (int i = 0; i < 1000; ++i) total += ts.accumulate(0.007);
    // 1000 ragged 7 ms frames = 7.0 real seconds = 420 steps at 60 Hz,
    // allowing one partial step for floating-point remainder.
    CHECK(total >= 419);
    CHECK(total <= 420);
}

TEST_CASE("a huge stall is clamped to max_steps_per_frame") {
    FixedTimestep ts;                        // max_steps_per_frame == 5
    CHECK(ts.accumulate(10.0) == 5);
}

TEST_CASE("after a clamped stall the accumulator is drained, not still full") {
    FixedTimestep ts;
    ts.accumulate(10.0);
    CHECK(ts.accumulator < ts.step_seconds);
    CHECK(ts.accumulate(kStep) == 1);        // back to normal immediately
}

TEST_CASE("zero and negative elapsed time are harmless") {
    FixedTimestep ts;
    CHECK(ts.accumulate(0.0) == 0);
    CHECK(ts.accumulate(-1.0) == 0);
    CHECK(ts.accumulator >= 0.0);
}

TEST_CASE("alpha reports progress toward the next step") {
    FixedTimestep ts;
    ts.accumulate(kStep * 0.25);
    CHECK(ts.alpha() == doctest::Approx(0.25));
    CHECK(ts.alpha() >= 0.0);
    CHECK(ts.alpha() < 1.0);
}