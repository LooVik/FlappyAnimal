#pragma once

#include <cstdint>
#include "gameplay/tuning.h"

namespace flappy {
    struct GateGenerator {
        std::uint32_t state = 1u;
        float last_gap_center = 0.0f;
        bool has_previous = false;

        void reset(std::uint32_t seed);

        float next_gap_center(const GameTuning& tuning, float gate_gap);

        std::uint32_t next_u32();
        float next_unit();
    };
}