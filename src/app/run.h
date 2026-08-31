#pragma once

#include <cstdint>

#include "app/screen.h"
#include "gameplay/gate_field.h"
#include "gameplay/player.h"
#include "gameplay/tuning.h"

namespace flappy {
    struct Run {
        Player player;
        GateField field;
        Screen screen = Screen::Ready;
        int score = 0;
        float dead_timer = 0.0f;
        long long step_count = 0;

        void reset(const GameTuning& tuning, std::uint32_t seed);
    };
}