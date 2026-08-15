#include "gameplay/difficulty.h"

namespace flappy {
    
    namespace 
    {
        int ramp_steps(const GameTuning& tuning, int score){
            if(score <= 0 || tuning.ramp_every_points <=0 )
            {
                return 0;
            }

            return score / tuning.ramp_every_points;
        }
    }

    float current_scroll_speed(const GameTuning& tuning, int score) {
        const float steps = static_cast<float>(ramp_steps(tuning, score));
        const float speed = tuning.scroll_speed + tuning.scroll_speed_step * steps;

        return speed > tuning.max_scroll_speed ? tuning.max_scroll_speed : speed;
    }

    float current_gate_gap(const GameTuning& tuning, int score) {
        const float steps = static_cast<float>(ramp_steps(tuning, score));
        const float gap = tuning.gate_gap - tuning.gate_gap_step * steps;

        return gap < tuning.min_gate_gap ? tuning.min_gate_gap : gap;
    }
}