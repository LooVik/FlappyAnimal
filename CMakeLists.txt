cmake_minimum_required(VERSION 3.28)
project(FlappyAnimals LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Keep every executable in one predictable folder.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

include(FetchContent)

# --- doctest: unit test framework, header-only ---------------------------
FetchContent_Declare(doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG        v2.5.3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(doctest)

# --- SFML 3 ----------------------------------------------------------------
# Set these BEFORE MakeAvailable — SFML reads them while configuring itself.
# Static libs are CMake's default, which means nothing to ship beside the .exe.
set(BUILD_SHARED_LIBS OFF)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)  # this game will never need it
# Audio stays ON deliberately: sound effects arrive in a later milestone, and
# flipping this later would force a full SFML rebuild.

FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG        3.1.0
    GIT_SHALLOW    TRUE
    EXCLUDE_FROM_ALL
    SYSTEM
)
FetchContent_MakeAvailable(SFML)

# --- tests ----------------------------------------------------------------
enable_testing()
add_executable(flappy_tests
    tests/test_time_step.cpp
    tests/test_player.cpp
    src/app/time_step.cpp
    src/gameplay/player.cpp
)
target_include_directories(flappy_tests PRIVATE src)
target_link_libraries(flappy_tests PRIVATE doctest::doctest)
add_test(NAME flappy_tests COMMAND flappy_tests)

# --- game executable -------------------------------------------------------
add_executable(flappy
    src/main.cpp
    src/app/time_step.cpp
    src/gameplay/player.cpp
)
target_include_directories(flappy PRIVATE src)
target_link_libraries(flappy PRIVATE SFML::Graphics)
