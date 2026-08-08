cmake_minimum_required(VERSION 3.24)
project(FlappyAnimals LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Put every executable and DLL in one folder so the game can find SDL3.dll.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

include(FetchContent)

# --- doctest: unit test framework, header-only ---------------------------
FetchContent_Declare(doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG        v2.5.3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(doctest)

# --- tests ----------------------------------------------------------------
enable_testing()
add_executable(flappy_tests
    tests/test_time_step.cpp
    src/app/time_step.cpp
)
target_include_directories(flappy_tests PRIVATE src)
target_link_libraries(flappy_tests PRIVATE doctest::doctest)
add_test(NAME flappy_tests COMMAND flappy_tests)