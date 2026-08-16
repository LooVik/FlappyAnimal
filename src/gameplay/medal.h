# pragma once

namespace flappy {
    enum class Medal
    {
        None,
        Bronze,
        Silver,
        Gold,
        Platinum
    };

    inline constexpr int kBronzeScore = 10;
    inline constexpr int kSilverScore = 40;
    inline constexpr int kGoldScore = 150;
    inline constexpr int kPlatinumScore = 200;

    Medal medal_for_score(int score);

    const char* medal_name(Medal medal);
}