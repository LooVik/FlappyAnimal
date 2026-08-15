#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>
#include <vector>

#include "gameplay/gate_generator.h"

using flappy::GameTuning;
using flappy::GateGenerator;

// Generate `count` gap centres from a seed. Returned by value — a vector of
// floats is cheap to move, and this keeps every test independent.
static std::vector<float> course(std::uint32_t seed, int count) {
    GameTuning    tuning;
    GateGenerator gen;
    gen.reset(seed);

    std::vector<float> centers;
    for (int i = 0; i < count; ++i) {
        centers.push_back(gen.next_gap_center(tuning, tuning.gate_gap));
    }
    return centers;
}

TEST_CASE("the same seed always produces the same course") {
    CHECK(course(12345u, 200) == course(12345u, 200));
}

TEST_CASE("different seeds produce different courses") {
    CHECK(course(1u, 50) != course(2u, 50));
}

TEST_CASE("a seed of zero still produces a varied course") {
    // xorshift is stuck at zero forever, so reset() must substitute.
    const auto centers = course(0u, 20);
    CHECK(centers.front() != centers.back());
}

TEST_CASE("every gap stays fully on screen") {
    GameTuning  tuning;
    const float half_gap = tuning.gate_gap * 0.5f;

    for (const float center : course(777u, 500)) {
        CHECK(center - half_gap >= tuning.gate_margin);
        CHECK(center + half_gap <= tuning.reference_height - tuning.gate_margin);
    }
}

TEST_CASE("consecutive gaps are always physically reachable") {
    GameTuning tuning;
    const auto centers = course(999u, 500);

    for (std::size_t i = 1; i < centers.size(); ++i) {
        CHECK(std::abs(centers[i] - centers[i - 1]) <= tuning.max_gap_shift);
    }
}

TEST_CASE("the course varies rather than repeating one height") {
    const auto centers = course(42u, 100);
    const auto [lo, hi] = std::minmax_element(centers.begin(), centers.end());
    CHECK(*hi - *lo > 300.0f);
}