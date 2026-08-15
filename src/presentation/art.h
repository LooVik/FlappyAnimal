#pragma once

#include <SFML/Graphics.hpp>

#include <string>

namespace flappy {

// Every texture the game draws, loaded once at startup.
//
// This lives OUTSIDE flappy_core because it includes SFML. Gameplay code must
// never see a texture — that boundary is what lets the tests run without a
// window, and the build enforces it: flappy_core does not link SFML at all.
struct Art {
    sf::Texture bird;        // 64x16 atlas: four 16x16 animation frames
    sf::Texture pipe;        // one 32x48 tile, loaded out of the atlas, repeating
    sf::Texture background;  // 256x256, repeating

    // Source pixel sizes. Everything is drawn at kPixelScale, so these times
    // the scale must match the matching values in GameTuning.
    static constexpr int kBirdFrame  = 16;   // x5 = 80 = player_width/height
    static constexpr int kFrameCount = 4;
    static constexpr int kPipeTileW  = 32;   // x5 = 160 = gate_width
    static constexpr int kPipeTileH  = 80;
    static constexpr int kBackground = 256;

    static constexpr float kPixelScale = 5.0f;

    // Returns false and reports to stderr if any file is missing.
    bool load(const std::string& asset_dir);
};

}  // namespace flappy
