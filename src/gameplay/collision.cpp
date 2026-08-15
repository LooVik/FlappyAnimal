#include "gameplay/collision.h"

namespace flappy {
    bool overlaps(const Rect& a, const Rect& b) {
        return a.x < b.x + b.w && 
                b.x < a.x + a.w &&
                a.y < b.y + b.h &&
                b.y < a.y + a.h;
    }

    Rect player_sprite(const GameTuning& tuning, const Player& player) {
        const float center_x = tuning.reference_width * tuning.player_anchor_x;
        return Rect{center_x - tuning.player_width * 0.5f,
                    player.y - tuning.player_height * 0.5f,
                    tuning.player_width,
                    tuning.player_height};
    }

    Rect player_body(const GameTuning& tuning, const Player& player) {
        const Rect sprite = player_sprite(tuning, player);
        const float width = sprite.w * tuning.collision_scale;
        const float height = sprite.h * tuning.collision_scale;

        return Rect{sprite.x + (sprite.w - width) * 0.5f,
                    sprite.y + (sprite.h - height) * 0.5f,
                    width,
                    height};
    }


    Rect gate_top_body(const GameTuning& tuning, const Gate& gate) {
        const float gap_top = gate.gap_center - gate.gap * 0.5f;
        return Rect{gate.x, 0.0f, tuning.gate_width, gap_top};
    }

    Rect gate_bottom_body(const GameTuning& tuning, const Gate& gate) {
        const float gap_bottom = gate.gap_center + gate.gap * 0.5f;
        return Rect{gate.x, gap_bottom, tuning.gate_width,
                    tuning.reference_height - gap_bottom};
    }

    bool hits_boundary(const GameTuning& tuning, const Player& player) {
        const Rect body = player_body(tuning,player);

        return body.y < 0.0f || body.y + body.h > tuning.reference_height;
    }

    bool hits_any_gate(const GameTuning& tuning, const Player& player, const GateField& field) {
        const Rect body = player_body(tuning, player);

        for(const Gate& gate : field.gates) {
            if(!gate.active)
            {
                continue;
            }

            if(overlaps(body, gate_top_body(tuning, gate)) || overlaps (body, gate_bottom_body(tuning, gate))) 
            {
                return true;
            }
        }
        return false;
    }

}