#pragma once

namespace flappy {

    struct GameTuning {
        float reference_width = 1080.0f;
        float reference_height = 1920.0f;

        //player
        float player_anchor_x = 0.28f;
        float gravity = 3400.0f;
        float tap_impulse = -850.0f;
        float max_fall_speed = 1200.0f;
        float collision_scale = 0.80f;

        float player_width = 96.0f;   // 16 px sprite at 5x
        float player_height = 96.0f;  // 16 px sprite at 5x

        //gates
        float scroll_speed = 420.0f;
        float gate_gap = 440.0f;
        float gate_width = 160.0f;    // 32 px tile at 5x
        float spawn_interval = 2.0f;
        float gate_margin   = 160.0f;  //keeps a gap clear of the screen edge
        float max_gap_shift = 320.0f;   // reachability cap between consecutive gates

        //difficulty ramp, spec 5.2
        int ramp_every_points = 10;
        float scroll_speed_step = 20.0f;
        float gate_gap_step = 12.0f;
        float max_scroll_speed = 620.0f;
        float min_gate_gap = 330.0f;

    };
}