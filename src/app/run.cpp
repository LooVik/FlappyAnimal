#include "app/run.h"

namespace flappy
{
    void Run::reset(const GameTuning& tuning, std::uint32_t seed)
    {
        *this = Run{};
        player.y = tuning.reference_height * 0.5f;
        field.reset(seed);
    }
}