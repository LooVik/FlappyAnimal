cmake_minimum_required(VERSION 3.28)
project(FlappyAnimals LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Keep every executable in one predictable folder.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# --- iOS ------------------------------------------------------------------
# CMAKE_SYSTEM_NAME is only "iOS" when the build is explicitly cross-compiling
# for a phone (see CMakePresets.json). A normal macOS build reports "Darwin"
# and takes the desktop path below, unchanged.
#
# Android does NOT come through this file at all — Gradle drives its own
# CMakeLists at platform/android/app/src/main/jni. iOS has no equivalent build
# system of its own, so Xcode consumes this one directly.
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(FLAPPY_IOS TRUE)

    # SFML refuses anything below 13.0, and this also becomes the minimum
    # iOS version the App Store will offer the app to.
    set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "" FORCE)

    # Your Apple Developer Team ID (Membership page in the developer portal, a
    # 10-character string). Needed to sign a build for a real device; leave it
    # empty to build for the Simulator, which is not signed.
    set(FLAPPY_IOS_TEAM_ID "" CACHE STRING "Apple Developer Team ID for code signing")
endif()

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

# --- gameplay logic: never includes SFML, so tests run with no window ------
add_library(flappy_core
    src/app/time_step.cpp
    src/app/run.cpp
    src/gameplay/player.cpp
    src/gameplay/gate_generator.cpp
    src/gameplay/gate_field.cpp
    src/gameplay/collision.cpp
    src/gameplay/scoring.cpp
    src/gameplay/difficulty.cpp
    src/profile/profile.cpp
    src/gameplay/medal.cpp
    src/ui/button.cpp
)
target_include_directories(flappy_core PUBLIC src)

# --- tests ----------------------------------------------------------------
# Skipped on iOS: a console test runner cannot be launched on a phone, and the
# logic these cover is platform-independent anyway — running them once on the
# desktop build proves the same thing.
if(NOT FLAPPY_IOS)
    enable_testing()
    add_executable(flappy_tests
        tests/test_time_step.cpp
        tests/test_run.cpp
        tests/test_player.cpp
        tests/test_gate_generator.cpp
        tests/test_gate_field.cpp
        tests/test_collision.cpp
        tests/test_scoring.cpp
        tests/test_difficulty.cpp
        tests/test_profile.cpp
        tests/test_medal.cpp
        tests/test_button.cpp
    )
    target_link_libraries(flappy_tests PRIVATE flappy_core doctest::doctest)
    add_test(NAME flappy_tests COMMAND flappy_tests)
endif()

# --- game executable -------------------------------------------------------
set(FLAPPY_APP_SOURCES
    src/main.cpp
    src/presentation/art.cpp
    src/presentation/audio.cpp
)

if(FLAPPY_IOS)
    set(FLAPPY_IOS_DIR "${CMAKE_SOURCE_DIR}/platform/ios")

    # iOS will not run a bare executable — only an .app bundle. MACOSX_BUNDLE
    # is what turns the target into one. The storyboard and the assets folder
    # are listed as sources so Xcode puts them in the Copy Bundle Resources
    # phase; the RESOURCE property below is what marks them as such.
    add_executable(flappy MACOSX_BUNDLE
        ${FLAPPY_APP_SOURCES}
        "${FLAPPY_IOS_DIR}/LaunchScreen.storyboard"
        "${CMAKE_SOURCE_DIR}/assets"
    )

    set_target_properties(flappy PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${FLAPPY_IOS_DIR}/Info.plist"

        # Copied into the .app. iOS bundles are flat, so both land at the
        # bundle root — which is exactly where SFML looks, because it chdir's
        # into the bundle's resource path before main() runs. That is why
        # art.load("assets") is the same line on all three platforms.
        RESOURCE "${FLAPPY_IOS_DIR}/LaunchScreen.storyboard;${CMAKE_SOURCE_DIR}/assets"

        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.loovik.flappyanimals"
        XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "${CMAKE_OSX_DEPLOYMENT_TARGET}"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"   # 1 = iPhone, 2 = iPad
        XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES      # SFML's iOS layer is ARC
        XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic"
        XCODE_GENERATE_SCHEME ON
    )

    if(FLAPPY_IOS_TEAM_ID)
        set_target_properties(flappy PROPERTIES
            XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${FLAPPY_IOS_TEAM_ID}")
    endif()

    # SFML::Main provides the real main() that hands control to UIApplication,
    # then calls ours on a worker thread. Same role as on Android, where it
    # provides ANativeActivity_onCreate. Without it the app links but nothing
    # ever starts.
    target_link_libraries(flappy PRIVATE SFML::Main)
else()
    add_executable(flappy ${FLAPPY_APP_SOURCES})

    # The game loads textures by relative path, so assets/ must sit beside the
    # .exe. Same reasoning as shipping a DLL: nothing is implicit outside the
    # source tree. (On iOS the bundle handles this; see above.)
    add_custom_command(TARGET flappy POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:flappy>/assets"
        VERBATIM)
endif()

target_link_libraries(flappy PRIVATE flappy_core SFML::Graphics SFML::Audio)


# Warning Flag
if(MSVC)
    target_compile_options(flappy_core PRIVATE /W4)
    target_compile_options(flappy      PRIVATE /W4)
endif()