#include "profile/profile.h"

#include <charconv>
#include <fstream>
#include <string>

namespace flappy {
namespace {

// std::stoi would do this, but it throws on bad input and silently accepts
// trailing junk ("12abc" becomes 12). from_chars has neither problem: no
// exceptions, and checking ptr == end proves the WHOLE string was a number.
bool parse_int(const std::string& text, int& out) {
    const char* begin = text.data();
    const char* end   = text.data() + text.size();

    const auto result = std::from_chars(begin, end, out);
    return result.ec == std::errc{} && result.ptr == end;
}

}  // namespace

bool Profile::load(const std::filesystem::path& path) {
    // Defaults FIRST. However badly the rest of this goes, the caller is left
    // with a usable profile rather than half-parsed garbage.
    *this = Profile{};

    std::ifstream in(path);
    if (!in) {
        return false;   // no save yet: normal on a first launch
    }

    Profile parsed;
    int version = 0;
    std::string line;

    while (std::getline(in, line)) {
        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;   // ignore blanks and comments

        const std::string key   = line.substr(0, eq);
        const std::string value = line.substr(eq + 1);

        if (key == "version") {
            if (!parse_int(value, version)) return false;
        } else if (key == "best_standard") {
            if (!parse_int(value, parsed.best_standard)) return false;
        }
        // Unknown keys are skipped on purpose: a newer build's save should
        // still load in an older build, minus the fields it does not know.
    }

    if (version != kSchemaVersion)   return false;
    if (parsed.best_standard < 0)    return false;   // impossible; distrust it

    *this = parsed;   // only now do we accept it
    return true;
}

bool Profile::save(const std::filesystem::path& path) const {
    // Make sure the folder exists before trying to write into it. On desktop
    // the path is a bare filename and this does nothing. On a phone it is a
    // nested path inside the app's private storage, and ofstream will not
    // create missing directories for us — it would just fail, silently, and
    // the high score would never persist.
    std::error_code dir_ec;
    const std::filesystem::path parent = path.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent, dir_ec);
        if (dir_ec) return false;
    }

    // Write to a temporary, then rename over the real file. Rename on one
    // volume is atomic, so a crash or power cut mid-save cannot corrupt the
    // profile: the old file survives untouched until the new one is complete.
    const std::filesystem::path tmp = path.string() + ".tmp";

    {
        std::ofstream out(tmp, std::ios::trunc);
        if (!out) return false;

        out << "version="       << kSchemaVersion << '\n'
            << "best_standard=" << best_standard  << '\n';

        if (!out) return false;
    }   // scope ends: the file is CLOSED here, which must happen before the
        // rename — Windows refuses to move a file that is still open.

    std::error_code ec;
    std::filesystem::rename(tmp, path, ec);
    if (ec) {
        std::filesystem::remove(tmp, ec);   // do not leave litter behind
        return false;
    }
    return true;
}

}  // namespace flappy