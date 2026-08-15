#include "gameplay/gate_field.h"

namespace flappy {
    void GateField::reset(std::uint32_t seed) {
        for(Gate& gate : gates) {
            gate = Gate{};
        }
        generator.reset(seed);
        spawn_timer = 0.0f;
    }

    void GateField::spawn(const GameTuning& tuning, float gate_gap) {
        for (Gate& gate: gates) {
            if(gate.active) 
            {
                continue;
            }

            gate.x = tuning.reference_width;
            gate.gap = gate_gap;
            gate.gap_center = generator.next_gap_center(tuning, gate_gap);
            gate.active = true;
            gate.scored = false;
            return;
        }
    }

    void GateField::step(const GameTuning& tuning, float dt, float scroll_speed, float gate_gap) {
        for (Gate& gate: gates) {
            if(!gate.active) 
            {
                continue;
            }

            gate.x -=scroll_speed * dt;

            if(gate.x + tuning.gate_width < 0.0f) {
                gate.active = false;
            }
        }

        spawn_timer -= dt;
        if(spawn_timer <= 0.0f)
        {
            spawn_timer += tuning.spawn_interval;
            spawn(tuning, gate_gap);
        }
    }

    int GateField::active_count() const {
        int count = 0;
        for (const Gate& gate : gates)
        {
            if(gate.active) 
            {
                ++count;
            }
        }
        return count;
    }
}