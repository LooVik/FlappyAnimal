#pragma once

#include "gameplay/gate_field.h"
#include "gameplay/player.h"
#include "gameplay/tuning.h"

namespace flappy {
    struct Rect {
        float x = 0.0f;
        float y = 0.0f;
        float w = 0.0f;
        float h = 0.0f;
    };

    bool overlaps(const Rect& a, const Rect& b);

    Rect player_sprite(const GameTuning& tuning, const Player& player);
    Rect player_body(const GameTuning& tuning, const Player& player);

    Rect gate_top_body(const GameTuning& tuning, const Gate& gate);
    Rect gate_bottom_body(const GameTuning& tuning, const Gate& gate);

    bool hits_boundary(const GameTuning& tuning, const Player& player);
    bool hits_any_gate(const GameTuning& tuning, const Player& player, const GateField& field);
    
}