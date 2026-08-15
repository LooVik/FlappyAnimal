#include "gameplay/gate_generator.h"

namespace flappy {
    void GateGenerator::reset(std::uint32_t speed) {
        state = (speed == 0u) ? 0x9E3779B9u : speed;
        last_gap_center = 0.0f;
        has_previous = false;
    }

    std::uint32_t GateGenerator::next_u32() {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }

    float GateGenerator::next_unit() {
        return static_cast<float>(next_u32() >> 8) / 16777216.0f;
    }

    float GateGenerator::next_gap_center(const GameTuning& tuning, float gate_gap) {
        const float half_gap = gate_gap * 0.5f;

        float min_center = tuning.gate_margin + half_gap;
        float max_center = tuning.reference_height - tuning.gate_margin - half_gap;

        if(has_previous) {
            const float reachable_min = last_gap_center - tuning.max_gap_shift;
            const float reachable_max = last_gap_center + tuning.max_gap_shift;

            if(reachable_min > min_center) 
            {
                min_center = reachable_min;
            }

            if(reachable_max < max_center) 
            {
                max_center = reachable_max;
            }
        }

        const float center = min_center + next_unit() * (max_center - min_center);
        last_gap_center = center;
        has_previous = true;
        return center;
    }
}