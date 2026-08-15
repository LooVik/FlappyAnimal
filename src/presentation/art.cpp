#include "presentation/art.h"

#include <iostream>

namespace flappy {
namespace {

// Pixel art must never be smoothed. SFML interpolates textures by default,
// which turns a crisp 16x16 sprite into blurred mush the moment it is scaled.
bool load_pixel(sf::Texture& texture, const std::string& path, bool repeating, const sf::IntRect& area = {}) {
    if (!texture.loadFromFile(path, false, area)) {
        std::cerr << "art: failed to load " << path << '\n';
        return false;
    }
    texture.setSmooth(false);
    texture.setRepeated(repeating);
    return true;
}

}  // namespace

bool Art::load(const std::string& asset_dir) {
    // The bird atlas holds four frames side by side; we keep the whole strip
    // and pick a frame with setTextureRect at draw time.
    if (!load_pixel(bird, asset_dir + "/Player/StyleBird1/Bird1-1.png", false)) {
        return false;
    }

    // The pipe file is an 8-cell atlas of colour variants. We load ONE cell as
    // its own texture, because setRepeated() tiles a whole texture and would
    // otherwise repeat all eight colours. Change the cell offset below to pick
    // a different colour; change the filename to pick a different style.
    const std::string pipe_path = asset_dir + "/Tiles/Style 1/PipeStyle1.png";

    if(!load_pixel(pipe, pipe_path, false, sf::IntRect({0,0}, {kPipeTileW, kPipeBodyH + kPipeCapH + kPipeCapH})))
    {
        return false;
    }
    if(!load_pixel(pipe_body, pipe_path, true, sf::IntRect({0, kPipeCapH + 16}, {kPipeTileW, kPipeShaftSlice})))
    {
        return false;
    }

    if (!load_pixel(background, asset_dir + "/Background/Background1.png", false)) {
        return false;
    }

    if(!font.openFromFile(asset_dir + "/fonts/PressStart2P-Regular.ttf")) 
    {
        std::cerr << "art: failed to load fonts\n";
        return false;
    }
    font.setSmooth(false);

    return true;
}

}  // namespace flappy
