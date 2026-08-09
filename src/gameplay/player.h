#pragma once

#include "gameplay/tuning.h"

namespace flappy {

    struct Player {
        float y = 0.0f;
        float velocity_y = 0.0f;

        //Advance one fixed simulation step.
        void step(const GameTuning& tuning, float dt);

        //replaces vertical velocity outright and does not add to it.
        void flap(const GameTuning& tuning);
    };
}