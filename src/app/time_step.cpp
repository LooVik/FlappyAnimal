#include "app/time_step.h"

namespace flappy {

// Converts real elapsed time into a whole number of fixed simulation steps.
int FixedTimestep::accumulate(double elapsed_seconds) {
    accumulator += elapsed_seconds;

    int steps = 0;
    while (accumulator >= step_seconds) {
        accumulator -= step_seconds;
        ++steps;
    }
    return steps;
}

double FixedTimestep::alpha() const {
    return accumulator / step_seconds;
}

}  // namespace flappy