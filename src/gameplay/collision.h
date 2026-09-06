#pragma once

#include "core/rect.h"
#include "gameplay/gate_field.h"
#include "gameplay/player.h"
#include "gameplay/tuning.h"

namespace flappy {

    bool overlaps(const Rect& a, const Rect& b);

    Rect player_sprite(const GameTuning& tuning, const Player& player);
    Rect player_body(const GameTuning& tuning, const Player& player);

    Rect gate_top_body(const GameTuning& tuning, const Gate& gate);
    Rect gate_bottom_body(const GameTuning& tuning, const Gate& gate);

    bool hits_boundary(const GameTuning& tuning, const Player& player);
    bool hits_any_gate(const GameTuning& tuning, const Player& player, const GateField& field);
    
}