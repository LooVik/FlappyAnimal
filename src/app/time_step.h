#pragma once

namespace flappy {

// Turns real elapsed wall-clock time into a whole number of fixed simulation
// steps. This is precisely what Unity's FixedUpdate did for you.
struct FixedTimestep {
    double step_seconds        = 1.0 / 60.0;  // source spec 5.2
    int    max_steps_per_frame = 5;           // spiral-of-death guard, Task 3
    double accumulator         = 0.0;         // unspent real time, in seconds

    // Feed the real seconds elapsed since the previous call.
    // Returns how many fixed steps the simulation should run right now.
    // Time too small to fill a whole step is carried into the next call.
    int accumulate(double elapsed_seconds);

    // Progress from the last completed step toward the next, in [0, 1).
    // Milestone 1 uses this to interpolate rendering. Unused until then.
    double alpha() const;
};

}  // namespace flappy