#pragma once

#include "gameplay/tuning.h"

namespace flappy {
    float current_scroll_speed(const GameTuning& tuning, int score);
    float current_gate_gap(const GameTuning& tuning, int score);
}