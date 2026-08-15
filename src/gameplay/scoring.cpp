#include "gameplay/scoring.h"

namespace flappy {
    int score_passed_gates(const GameTuning& tuning, GateField& field) {
        const float anchor_x = tuning.reference_width * tuning.player_anchor_x;

        int newly_scored = 0;
        for(Gate& gate : field.gates) 
        {
            if(!gate.active || gate.scored)
            {
                continue;
            }
            
            if(gate.x + tuning.gate_width < anchor_x) 
            {
                gate.scored = true;
                ++newly_scored;
            }
        }
        return newly_scored;
    }
}