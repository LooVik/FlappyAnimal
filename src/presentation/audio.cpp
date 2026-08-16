#include "presentation/audio.h"

#include <iostream>

namespace flappy {
    namespace 
    {
        bool load_buffer(sf::SoundBuffer& buffer, const std::string& path)
        {
            if(!buffer.loadFromFile(path))
            {
                std::cerr << "audio: failed to load " << path << "\n";
                return false;
            }
            return true;
        }
    }

    bool Audio::load(const std::string& asset_dir) 
    {
        if(!load_buffer(flap_buffer, asset_dir + "/audio/flap.wav")) 
        {
            return false;
        }

        if(!load_buffer(score_buffer, asset_dir + "/audio/score.wav")) 
        {
            return false;
        }

        if(!load_buffer(hit_buffer, asset_dir + "/audio/hit.wav")) 
        {
            return false;
        }

        flap.emplace(flap_buffer);
        score.emplace(score_buffer);
        hit.emplace(hit_buffer);

        flap->setVolume(55.0f);
        score->setVolume(80.0f);
        hit->setVolume(90.0f);

        return true;
    }
}