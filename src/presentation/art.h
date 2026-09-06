#pragma once

#include <SFML/Graphics.hpp>

#include <string>

namespace flappy {

// Every texture the game draws, loaded once at startup.
//
// This lives OUTSIDE flappy_core because it includes SFML. Gameplay code must
// never see a texture — that boundary is what lets the tests run without a
// window, and the build enforces it: flappy_core does not link SFML at all.
// One button's two drawn states.
//
// The asset pack ships three per colour: raised, hover, pressed. Hover is left
// out on purpose — a phone has no cursor, so a hover state would exist on the
// desktop build and nowhere else, and the game should not look like two
// different games.
//
// Both images are the same canvas size (415x111). The pressed one simply has
// the button face painted lower inside it, with a shorter shadow beneath. That
// is why swapping the texture is the whole effect: draw both into the same
// rectangle and the button visibly sinks.
struct ButtonSkin {
    sf::Texture normal;
    sf::Texture pressed;
};

struct Art {
    sf::Texture bird;        // 64x16 atlas: four 16x16 animation frames
    sf::Texture pipe;        // whole 32x80 cell: cap, shaft, cap
    sf::Texture pipe_body;   // just the 32x48 shaft, repeating
    sf::Texture background;  // 256x256, repeating

    // UI buttons. Colour carries meaning here: green goes, blue is neutral,
    // red backs out.
    ButtonSkin button_green;   // PLAY, RETRY
    ButtonSkin button_blue;    // LEADERBOARD
    ButtonSkin button_red;     // HOME, CLOSE

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

    // The button images, and where the face sits inside them. Measured from the
    // pixels, not guessed: in the raised image the lit face runs from row 0 to
    // 88 of 111, and in the pressed image from 15 to 103. A label centred in
    // the whole rectangle would float over the shadow, so it is centred on the
    // FACE instead — and moves down with it when the button is pushed.
    static constexpr int   kButtonW           = 415;
    static constexpr int   kButtonH           = 111;
    static constexpr float kButtonFaceNormal  = 0.396f;   // (0 + 88) / 2 / 111
    static constexpr float kButtonFacePressed = 0.532f;   // (15 + 103) / 2 / 111

    // Returns false and reports to stderr if any file is missing.
    bool load(const std::string& asset_dir);
};

}  // namespace flappy
