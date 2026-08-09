#include "gameplay/player.h"

namespace flappy {

void Player::step(const GameTuning& tuning, float dt) {

    velocity_y += tuning.gravity * dt;

    if (velocity_y > tuning.max_fall_speed) {
        velocity_y = tuning.max_fall_speed;
    }

    y += velocity_y * dt;
}

void Player::flap(const GameTuning& tuning) {

    velocity_y = tuning.tap_impulse;
}

}