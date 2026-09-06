#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>

#include "profile/profile.h"

using flappy::Profile;

namespace {

// A unique path per test, cleaned up by the destructor. Tests must never leave
// files behind, or they start passing and failing depending on run order.
struct TempFile {
    std::filesystem::path path;

    explicit TempFile(const char* name)
        : path(std::filesystem::temp_directory_path() / name) {
        std::filesystem::remove(path);
    }
    ~TempFile() {
        std::error_code ec;
        std::filesystem::remove(path, ec);
        std::filesystem::remove(path.string() + ".tmp", ec);
    }
};

void write_file(const std::filesystem::path& path, const char* contents) {
    std::ofstream out(path, std::ios::trunc);
    out << contents;
}

}  // namespace

TEST_CASE("a saved profile loads back identically") {
    TempFile file("flappy_test_roundtrip.txt");

    Profile saved;
    saved.best_standard = 47;
    CHECK(saved.save(file.path));

    Profile loaded;
    CHECK(loaded.load(file.path));
    CHECK(loaded.best_standard == 47);
}

TEST_CASE("a missing file leaves defaults and is not a crash") {
    TempFile file("flappy_test_missing.txt");

    Profile profile;
    profile.best_standard = 99;

    CHECK_FALSE(profile.load(file.path));   // nothing to load
    CHECK(profile.best_standard == 0);      // reset to defaults, not left at 99
}

TEST_CASE("a corrupt file leaves defaults rather than garbage") {
    TempFile file("flappy_test_corrupt.txt");
    write_file(file.path, "version=1\nbest_standard=not-a-number\n");

    Profile profile;
    CHECK_FALSE(profile.load(file.path));
    CHECK(profile.best_standard == 0);
}

TEST_CASE("trailing junk after a number is rejected") {
    TempFile file("flappy_test_junk.txt");
    write_file(file.path, "version=1\nbest_standard=12abc\n");

    Profile profile;
    CHECK_FALSE(profile.load(file.path));
    CHECK(profile.best_standard == 0);
}

TEST_CASE("a save from a different schema version is ignored") {
    TempFile file("flappy_test_version.txt");
    write_file(file.path, "version=99\nbest_standard=1234\n");

    Profile profile;
    CHECK_FALSE(profile.load(file.path));
    CHECK(profile.best_standard == 0);   // NOT 1234
}

TEST_CASE("unknown keys are skipped, known ones still load") {
    TempFile file("flappy_test_unknown.txt");
    write_file(file.path, "version=1\nfeathers=500\nbest_standard=8\n");

    Profile profile;
    CHECK(profile.load(file.path));
    CHECK(profile.best_standard == 8);
}

TEST_CASE("saving leaves no temporary file behind") {
    TempFile file("flappy_test_tmp.txt");

    Profile profile;
    profile.best_standard = 3;
    CHECK(profile.save(file.path));

    CHECK(std::filesystem::exists(file.path));
    CHECK_FALSE(std::filesystem::exists(file.path.string() + ".tmp"));
}

TEST_CASE("saving twice overwrites cleanly") {
    TempFile file("flappy_test_overwrite.txt");

    Profile first;
    first.best_standard = 5;
    CHECK(first.save(file.path));

    Profile second;
    second.best_standard = 11;
    CHECK(second.save(file.path));

    Profile loaded;
    CHECK(loaded.load(file.path));
    CHECK(loaded.best_standard == 11);
}