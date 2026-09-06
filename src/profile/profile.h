#pragma once

#include <filesystem>

namespace flappy
{
    struct Profile
    {
        static constexpr int kSchemaVersion = 1;

        int best_standard = 0;
        bool load(const std::filesystem::path& path);
        bool save(const std::filesystem::path& path) const;
    };
} 
