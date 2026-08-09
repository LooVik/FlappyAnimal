#include <SFML/Graphics.hpp>

#include <iostream>
#include <optional>

#include "app/time_step.h"

namespace {
constexpr unsigned kRefWidth    = 1080;  // spec 5.2 reference resolution
constexpr unsigned kRefHeight   = 1920;
constexpr unsigned kWindowScale = 3;     // a 360 x 640 window on the desktop

// SFML has no one-call letterbox, so we compute the viewport ourselves.
// A viewport is expressed as a fraction of the window, not in pixels: we want
// the largest 9:16 rectangle that fits, centred, with bars filling the rest.
sf::View letterboxed(sf::View view, sf::Vector2u window_size) {
    const float window_ratio = static_cast<float>(window_size.x) /
                               static_cast<float>(window_size.y);
    const float view_ratio = view.getSize().x / view.getSize().y;

    float width  = 1.0f;
    float height = 1.0f;
    float left   = 0.0f;
    float top    = 0.0f;

    if (window_ratio > view_ratio) {  // window too wide: bars left and right
        width = view_ratio / window_ratio;
        left  = (1.0f - width) / 2.0f;
    } else {                          // window too tall: bars top and bottom
        height = window_ratio / view_ratio;
        top    = (1.0f - height) / 2.0f;
    }

    view.setViewport(sf::FloatRect({left, top}, {width, height}));
    return view;
}
}  // namespace

int main() {
    sf::RenderWindow window(
        sf::VideoMode({kRefWidth / kWindowScale, kRefHeight / kWindowScale}),
        "FlappyAnimals");

    // Pin presentation to the monitor. Comment this out and frames/sec below
    // jumps into the thousands while steps/sec stays at 60 — which is the
    // entire point of the fixed timestep, made obvious.
    window.setVerticalSyncEnabled(true);

    // Draw in 1080x1920 coordinates forever; the view maps them onto whatever
    // the real window — or phone screen — happens to be.
    sf::View view(sf::FloatRect({0.0f, 0.0f},
                                {static_cast<float>(kRefWidth),
                                 static_cast<float>(kRefHeight)}));
    window.setView(letterboxed(view, window.getSize()));

    flappy::FixedTimestep timestep;
    sf::Clock frame_clock;
    sf::Clock report_clock;
    long long step_count      = 0;
    int       steps_this_second  = 0;
    int       frames_this_second = 0;

    sf::RectangleShape marker({18.0f, 80.0f});
    marker.setFillColor(sf::Color(240, 200, 90));

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            } else if (const auto* resized = event->getIf<sf::Event::Resized>()) {
                window.setView(letterboxed(view, resized->size));
            }
        }

        // Microseconds, not asSeconds(), because asSeconds() returns a float
        // and we want the accumulator's remainder to survive at double width.
        const double elapsed_seconds =
            static_cast<double>(frame_clock.restart().asMicroseconds()) / 1'000'000.0;

        const int steps = timestep.accumulate(elapsed_seconds);
        for (int i = 0; i < steps; ++i) {
            // Milestone 1's gravity, gates and collision go here.
            ++step_count;
        }
        steps_this_second += steps;
        ++frames_this_second;

        window.clear(sf::Color(18, 22, 34));

        // One lap per second: 60 steps at 1080/60 px each.
        marker.setPosition(
            {static_cast<float>((step_count % 60) * (kRefWidth / 60)),
             static_cast<float>(kRefHeight) * 0.5f - 40.0f});
        window.draw(marker);

        window.display();

        if (report_clock.getElapsedTime().asSeconds() >= 1.0f) {
            std::cout << "steps/sec = " << steps_this_second
                      << "   frames/sec = " << frames_this_second << '\n'
                      << std::flush;
            steps_this_second  = 0;
            frames_this_second = 0;
            report_clock.restart();
        }
    }

    // No teardown. sf::RenderWindow's destructor closes the window when it
    // goes out of scope — this is the RAII that SDL3 would have made us write
    // by hand, and the reason the error-handling ladder above it is gone too.
    return 0;
}
