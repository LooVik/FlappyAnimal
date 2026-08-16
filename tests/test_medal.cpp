#include <doctest/doctest.h>

#include <string>

#include "gameplay/medal.h"

using flappy::Medal;

TEST_CASE("no medal below the first threshold") {
    CHECK(flappy::medal_for_score(0) == Medal::None);
    CHECK(flappy::medal_for_score(flappy::kBronzeScore - 1) == Medal::None);
}

TEST_CASE("each threshold awards its medal exactly on the boundary") {
    CHECK(flappy::medal_for_score(flappy::kBronzeScore)   == Medal::Bronze);
    CHECK(flappy::medal_for_score(flappy::kSilverScore)   == Medal::Silver);
    CHECK(flappy::medal_for_score(flappy::kGoldScore)     == Medal::Gold);
    CHECK(flappy::medal_for_score(flappy::kPlatinumScore) == Medal::Platinum);
}

TEST_CASE("a medal is held until the next threshold") {
    CHECK(flappy::medal_for_score(flappy::kSilverScore   - 1) == Medal::Bronze);
    CHECK(flappy::medal_for_score(flappy::kGoldScore     - 1) == Medal::Silver);
    CHECK(flappy::medal_for_score(flappy::kPlatinumScore - 1) == Medal::Gold);
}

TEST_CASE("a huge score still returns the top medal") {
    CHECK(flappy::medal_for_score(999999) == Medal::Platinum);
}

TEST_CASE("a negative score is harmless") {
    CHECK(flappy::medal_for_score(-5) == Medal::None);
}

TEST_CASE("every medal has a label except None") {
    CHECK(std::string(flappy::medal_name(Medal::None)).empty());
    CHECK_FALSE(std::string(flappy::medal_name(Medal::Bronze)).empty());
    CHECK_FALSE(std::string(flappy::medal_name(Medal::Platinum)).empty());
}