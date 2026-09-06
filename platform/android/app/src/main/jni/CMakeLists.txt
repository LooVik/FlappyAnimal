cmake_minimum_required(VERSION 3.22)
project(flappy CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# This file lives at platform/android/app/src/main/jni, so the repository root
# is six directories up. Everything below reaches back into the SAME sources
# the desktop build uses — there is no Android copy of the game code, and
# there must never be one.
set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../..")

# The library name must match android.app.lib_name in AndroidManifest.xml, and
# Android prefixes it with "lib": this produces libflappy.so.
add_library(flappy SHARED
    ${REPO_ROOT}/src/main.cpp

    ${REPO_ROOT}/src/app/time_step.cpp
    ${REPO_ROOT}/src/app/run.cpp

    ${REPO_ROOT}/src/gameplay/player.cpp
    ${REPO_ROOT}/src/gameplay/gate_generator.cpp
    ${REPO_ROOT}/src/gameplay/gate_field.cpp
    ${REPO_ROOT}/src/gameplay/collision.cpp
    ${REPO_ROOT}/src/gameplay/scoring.cpp
    ${REPO_ROOT}/src/gameplay/difficulty.cpp
    ${REPO_ROOT}/src/gameplay/medal.cpp

    ${REPO_ROOT}/src/profile/profile.cpp

    ${REPO_ROOT}/src/ui/button.cpp

    ${REPO_ROOT}/src/presentation/art.cpp
    ${REPO_ROOT}/src/presentation/audio.cpp
)

target_include_directories(flappy PRIVATE ${REPO_ROOT}/src)

# On Android there is no FetchContent: SFML is cross-compiled once per ABI and
# installed into the NDK ahead of time. SFML_DIR points find_package straight at
# the folder holding SFMLConfig.cmake — CMake will not go looking on its own.
set(SFML_DIR "${CMAKE_ANDROID_NDK}/sources/third_party/sfml/lib/${CMAKE_ANDROID_ARCH_ABI}/cmake/SFML")

# Work around a gap in SFML's Android support.
#
# The exported SFML::Window target links EGL::EGL and GLES::GLES, but SFML's own
# SFMLWindowDependencies.cmake skips both on Android:
#
#     if(NOT FIND_SFML_OS_ANDROID AND NOT FIND_SFML_OS_IOS)
#         find_dependency(OpenGL COMPONENTS OpenGL)
#     endif()
#
# So the export names targets nothing ever creates, and the generate step fails.
# SFML does ship FindEGL.cmake and FindGLES.cmake next to its config — they are
# simply never called. Putting that folder on the module path and calling them
# ourselves creates the two targets before SFML's export asks for them.
list(APPEND CMAKE_MODULE_PATH "${SFML_DIR}")
find_package(EGL REQUIRED)
find_package(GLES REQUIRED)

find_package(SFML 3 REQUIRED COMPONENTS Graphics Audio Main)

target_link_libraries(flappy PRIVATE
    SFML::Graphics
    SFML::Audio
    android
    log
)

# SFML::Main provides ANativeActivity_onCreate, the entry point Android calls.
# It sits in a static library, and the linker drops unreferenced symbols from
# static libraries by default — nothing in our code calls it, so it would be
# stripped and the app would fail to start. --whole-archive forces it in.
target_link_libraries(flappy PUBLIC
    -Wl,--whole-archive
    SFML::Main
    -Wl,--no-whole-archive
)

# Google Play requires 16 KB page-size support for new apps.
target_link_options(flappy PRIVATE "-Wl,-z,max-page-size=16384")
