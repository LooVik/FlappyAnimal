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
    sf::Texture pipe;        // whole 32x80 cell: cap, shaft, cap
    sf::Texture pipe_body;   // just the 32x48 shaft, repeating
    sf::Texture background;  // 256x256, repeating

    //Fonts
    sf::Font font;

    // Source pixel sizes. Everything is drawn at kPixelScale, so these times
    // the scale must match the matching values in GameTuning.
    static constexpr int kBirdFrame  = 16;   // x6 = 80 = player_width/height
    static constexpr int kFrameCount = 4;
    static constexpr int kPipeTileW  = 32;   // x5 = 160 = gate_width
    static constexpr int kPipeCapH   = 16;
    static constexpr int kPipeBodyH  = 48;
    static constexpr int kPipeShaftSlice = 8;
    static constexpr int kBackground = 256;

    // Returns false and reports to stderr if any file is missing.
    bool load(const std::string& asset_dir);
};

}  // namespace flappy
