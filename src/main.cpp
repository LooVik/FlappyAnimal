#include <SFML/Graphics.hpp>

#include <cstdint>
#include <optional>

#include "app/time_step.h"
#include "gameplay/collision.h"
#include "gameplay/gate_field.h"
#include "gameplay/player.h"
#include "gameplay/tuning.h"

namespace {

constexpr unsigned kWindowScale = 3;
constexpr std::uint32_t kSeed = 20260808u;  // fixed: same course every run

sf::View letterboxed(sf::View view, sf::Vector2u window_size) {
    const float window_ratio = static_cast<float>(window_size.x) / static_cast<float>(window_size.y);
    const float view_ratio = view.getSize().x / view.getSize().y;

    float width = 1.0f, height = 1.0f, left = 0.0f, top = 0.0f;

    if (window_ratio > view_ratio) {
        width = view_ratio / window_ratio;
        left  = (1.0f - width) / 2.0f;
    } else {
        height = window_ratio / view_ratio;
        top    = (1.0f - height) / 2.0f;
    }

    view.setViewport(sf::FloatRect({left, top}, {width, height}));
    return view;
}

sf::RectangleShape filled(const flappy::Rect& r, sf::Color color) {
    sf::RectangleShape shape({r.w, r.h});
    shape.setPosition({r.x, r.y});
    shape.setFillColor(color);
    return shape;
}

sf::RectangleShape outlined(const flappy::Rect& r, sf::Color color) {
    sf::RectangleShape shape({r.w, r.h});
    shape.setPosition({r.x, r.y});
    shape.setFillColor(sf::Color::Transparent);
    shape.setOutlineColor(color);
    shape.setOutlineThickness(-2.0f);   // negative draws inward
    return shape;
}

}  // namespace

int main() {
    const flappy::GameTuning tuning;

    sf::RenderWindow window(
        sf::VideoMode({static_cast<unsigned>(tuning.reference_width)  / kWindowScale,
                       static_cast<unsigned>(tuning.reference_height) / kWindowScale}),
        "FlappyAnimals");
    window.setVerticalSyncEnabled(true);

    sf::View view(sf::FloatRect({0.0f, 0.0f}, {tuning.reference_width, tuning.reference_height}));
    window.setView(letterboxed(view, window.getSize()));

    flappy::FixedTimestep timestep;
    sf::Clock frame_clock;

    flappy::Player player;
    flappy::GateField field;
    bool flying = false;   // false = Ready, world frozen. Spec 4.4.

    auto start_run = [&] {
        player   = flappy::Player{};
        player.y = tuning.reference_height * 0.5f;
        field.reset(kSeed);
        flying   = false;
    };
    start_run();

    while (window.isOpen()) {
        bool tapped = false;

        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            } else if (const auto* resized = event->getIf<sf::Event::Resized>()) {
                window.setView(letterboxed(view, resized->size));
            } else if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                if (key->code == sf::Keyboard::Key::Space)  tapped = true;
                if (key->code == sf::Keyboard::Key::Escape) window.close();
            } else if (event->is<sf::Event::MouseButtonPressed>()) {
                tapped = true;
            }
        }

        if (tapped) {
            flying = true;            // the first tap starts the run
            player.flap(tuning);
        }

        const double elapsed_seconds =
            static_cast<double>(frame_clock.restart().asMicroseconds()) / 1'000'000.0;

        const float dt = static_cast<float>(timestep.step_seconds);

        for (int i = 0, steps = timestep.accumulate(elapsed_seconds); i < steps; ++i) {
            if (!flying) break;       // Ready: nothing moves until you tap

            player.step(tuning, dt);
            field.step(tuning, dt, tuning.scroll_speed);

            if (flappy::hits_boundary(tuning, player) ||
                flappy::hits_any_gate(tuning, player, field)) {
                start_run();
                break;
            }
        }

        window.clear(sf::Color(18, 22, 34));

        for (const flappy::Gate& gate : field.gates) {
            if (!gate.active) continue;
            window.draw(filled(flappy::gate_top_body(tuning, gate), sf::Color(70, 110, 80)));
            window.draw(filled(flappy::gate_bottom_body(tuning, gate), sf::Color(70, 110, 80)));
        }

        // Sprite filled, collision body outlined on top — so you can SEE the
        // 80% forgiveness rather than trusting a unit test about it.
        window.draw(filled(flappy::player_sprite(tuning, player),  sf::Color(240, 200, 90)));
        window.draw(outlined(flappy::player_body(tuning, player),  sf::Color(255, 80, 80)));

        window.display();
    }

    return 0;
}