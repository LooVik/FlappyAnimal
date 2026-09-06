#pragma once

namespace flappy {
    enum class Screen
    {
        Home,         // title, PLAY and LEADERBOARD — nothing is simulating
        Ready,        // bird placed, waiting for the first tap
        Playing,
        Dying,
        Results,
        Leaderboard,
    };
}
