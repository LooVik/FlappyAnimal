#include "app/time_step.h"

namespace flappy {

// Converts real elapsed time into a whole number of fixed simulation steps.
int FixedTimestep::accumulate(double elapsed_seconds) {
    if(elapsed_seconds > 0.0)
    {
        accumulator += elapsed_seconds;
    }

    const double max_accumulated = step_seconds * max_steps_per_frame;
    if(accumulator > max_accumulated)
    {
        accumulator = max_accumulated;
    }

    int steps = 0;
    while (accumulator >= step_seconds) 
    {
        accumulator -= step_seconds;
        ++steps;
    }
    return steps;
}

double FixedTimestep::alpha() const {
    return accumulator / step_seconds;
}

}  // namespace flappy