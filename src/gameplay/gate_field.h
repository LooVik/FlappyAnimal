#pragma once

#include <array>
#include <cstdint>

#include "gameplay/gate_generator.h"
#include "gameplay/tuning.h"

namespace flappy {
    struct Gate {
        float x = 0.0f;
        float gap_center = 0.0f;
        bool active = false;
        bool scored = false;
    };

    struct GateField {
        static constexpr int kMaxGates = 8;

        std::array<Gate, kMaxGates> gates{};
        GateGenerator generator;
        float spawn_timer = 0.0f;

        void reset(std::uint32_t seed);

        void step(const GameTuning& tuning, float dt, float scroll_speed);

        int active_count() const;

        private: void spawn(const GameTuning& tuning);
    };
}