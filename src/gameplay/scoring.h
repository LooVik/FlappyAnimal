#pragma once

#include "gameplay/gate_field.h"
#include "gameplay/tuning.h"

namespace flappy {
    int score_passed_gates(const GameTuning& tuning, GateField& field);
    
}