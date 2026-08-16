#pragma once

#include <SFML/Audio.hpp>
#include <optional>
#include <string> 

namespace flappy {
    struct Audio {
        // Every sound the game plays, loaded once at startup.
        //
        // The buffers and the sounds live in the SAME object on purpose. An sf::Sound
        // holds a POINTER to its sf::SoundBuffer, not a copy — if the buffer is
        // destroyed first, the sound plays silence or crashes, and nothing warns you.
        // Keeping both here makes that impossible: they die together or not at all.
        // Same rule as sf::Sprite and sf::Texture in Art.
        sf::SoundBuffer flap_buffer;
        sf::SoundBuffer score_buffer;
        sf::SoundBuffer hit_buffer;

         // sf::Sound has no default constructor in SFML 3 — it needs its buffer up
        std::optional<sf::Sound> flap;
        std::optional<sf::Sound> score;
        std::optional<sf::Sound> hit;

        bool load(const std::string& asset_dir);
    };
}