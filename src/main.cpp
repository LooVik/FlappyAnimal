#include <SFML/Graphics.hpp>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>

#include "app/time_step.h"
#include "gameplay/collision.h"
#include "gameplay/gate_field.h"
#include "gameplay/player.h"
#include "gameplay/tuning.h"
#include "gameplay/scoring.h"
#include "gameplay/difficulty.h"
#include "presentation/art.h"

namespace {

constexpr unsigned kWindowScale = 3;
constexpr std::uint32_t kSeed = 20260808u;  // fixed: same course every run

// How many fixed steps each bird animation frame is held for. 6 steps at 60 Hz
// is a 10 fps flap, which reads well and is cheap.
constexpr long long kFramesPerBirdFrame = 6;

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

sf::RectangleShape outlined(const flappy::Rect& r, sf::Color color) {
    sf::RectangleShape shape({r.w, r.h});
    shape.setPosition({r.x, r.y});
    shape.setFillColor(sf::Color::Transparent);
    shape.setOutlineColor(color);
    shape.setOutlineThickness(-2.0f);   // negative draws inward
    return shape;
}

// Fill a rect by REPEATING the texture rather than stretching it. Stretching
// pixel art distorts it; tiling keeps every pixel square. This works because
// art.cpp calls setRepeated(true) on these textures — a texture rect larger
// than the texture then wraps instead of clamping.
void draw_tiled(sf::RenderTarget& target, const sf::Texture& texture,
                const flappy::Rect& dest, float scale) {
    if (dest.w <= 0.0f || dest.h <= 0.0f) return;

    sf::Sprite sprite(texture);
    sprite.setTextureRect(sf::IntRect(
        {0, 0},
        {static_cast<int>(dest.w / scale), static_cast<int>(dest.h / scale)}));
    sprite.setPosition({dest.x, dest.y});
    sprite.setScale({scale, scale});
    target.draw(sprite);
}

// One frame out of the bird atlas, tilted by how fast it is rising or falling.
void draw_bird(sf::RenderTarget& target, const sf::Texture& texture,
               const flappy::Rect& dest, int frame, float tilt_degrees) {
    constexpr int kSize = flappy::Art::kBirdFrame;

    sf::Sprite sprite(texture, sf::IntRect({frame * kSize, 0}, {kSize, kSize}));

    // Rotate about the middle of the frame, not the top-left corner, or the
    // bird swings around its own shoulder.
    sprite.setOrigin({kSize * 0.5f, kSize * 0.5f});
    sprite.setScale({dest.w / static_cast<float>(kSize),
                     dest.h / static_cast<float>(kSize)});
    sprite.setPosition({dest.x + dest.w * 0.5f, dest.y + dest.h * 0.5f});
    sprite.setRotation(sf::degrees(tilt_degrees));
    target.draw(sprite);
}

std::string status_line(const flappy::GameTuning& tuning, int score) {
    return "FlappyAnimals  " + std::to_string(score) +
           "   |  grav "  + std::to_string(static_cast<int>(tuning.gravity)) +
           "  flap "      + std::to_string(static_cast<int>(tuning.tap_impulse)) +
           "  fall "      + std::to_string(static_cast<int>(tuning.max_fall_speed)) +
           "  gap "       + std::to_string(static_cast<int>(tuning.gate_gap)) +
           "  speed "     + std::to_string(static_cast<int>(tuning.scroll_speed));
}

}  // namespace

int main() {
    // NOT const: the tuning keys change these while the game runs so you can
    // feel a change instead of rebuilding for it.
    flappy::GameTuning tuning;

    flappy::Art art;
    if (!art.load("assets")) {
        return 1;   // art.cpp already reported which file was missing
    }

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
    bool dead = false;
    bool show_hitboxes = false;
    int score = 0;
    float dead_timer = 0.0f;
    long long step_count = 0;
    std::string shown_status;

    auto start_run = [&] {
        player   = flappy::Player{};
        player.y = tuning.reference_height * 0.5f;
        field.reset(kSeed);
        flying   = false;
        dead = false;
        dead_timer = 0.0f;
        score = 0;
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
                using K = sf::Keyboard::Key;
                switch (key->code) {
                    case K::Space:  tapped = true;  break;
                    case K::Escape: window.close(); break;
                    case K::H: show_hitboxes = !show_hitboxes; break;

                    // --- live tuning, temporary. Remove before shipping. ----
                    case K::Q: tuning.gravity        -= 100.0f; break;
                    case K::W: tuning.gravity        += 100.0f; break;
                    case K::A: tuning.tap_impulse    +=  25.0f; break;  // weaker flap
                    case K::S: tuning.tap_impulse    -=  25.0f; break;  // stronger flap
                    case K::Z: tuning.max_fall_speed -=  50.0f; break;
                    case K::X: tuning.max_fall_speed +=  50.0f; break;
                    case K::E: tuning.gate_gap       -=  10.0f; break;
                    case K::R: tuning.gate_gap       +=  10.0f; break;
                    case K::D: tuning.scroll_speed   -=  20.0f; break;
                    case K::F: tuning.scroll_speed   +=  20.0f; break;
                    case K::T: tuning = flappy::GameTuning{};   break;  // defaults
                    default: break;
                }
            } else if (event->is<sf::Event::MouseButtonPressed>()) {
                tapped = true;
            }
        }

        if (tapped) {
            if (dead)
            {
                if (dead_timer <= 0.0f)
                {
                    start_run();
                }
            }
            else
            {
                flying = true;            // the first tap starts the run
                player.flap(tuning);
            }
        }

        const double elapsed_seconds =
            static_cast<double>(frame_clock.restart().asMicroseconds()) / 1'000'000.0;

        const float dt = static_cast<float>(timestep.step_seconds);

        for (int i = 0, steps = timestep.accumulate(elapsed_seconds); i < steps; ++i) {

            if (dead)
            {
                dead_timer -= dt;
                continue;
            }

            if (!flying)
            {
                break;
            }       // Ready: nothing moves until you tap

            player.step(tuning, dt);
            field.step(tuning, dt, flappy::current_scroll_speed(tuning, score), flappy::current_gate_gap(tuning, score));
            score += flappy::score_passed_gates(tuning, field);
            ++step_count;

            if (flappy::hits_boundary(tuning, player) ||
                flappy::hits_any_gate(tuning, player, field)) {
                dead = true;
                dead_timer = 0.2f;
                break;
            }
        }

        const std::string status = status_line(tuning, score);
        if (status != shown_status) {
            window.setTitle(status);
            shown_status = status;
        }

        // ---- draw ----------------------------------------------------------
        window.clear(sf::Color(18, 22, 34));

        draw_tiled(window, art.background,
                   flappy::Rect{0.0f, 0.0f, tuning.reference_width, tuning.reference_height},
                   flappy::Art::kPixelScale);

        for (const flappy::Gate& gate : field.gates) {
            if (!gate.active) continue;
            draw_tiled(window, art.pipe, flappy::gate_top_body(tuning, gate),
                       flappy::Art::kPixelScale);
            draw_tiled(window, art.pipe, flappy::gate_bottom_body(tuning, gate),
                       flappy::Art::kPixelScale);
        }

        // Nose up when rising, dive when falling — the whole tilt comes from
        // one number the physics already tracks.
        const float tilt = std::clamp(player.velocity_y / tuning.max_fall_speed,
                                      -1.0f, 1.0f) * 55.0f;
        const int frame = static_cast<int>(
            (step_count / kFramesPerBirdFrame) % flappy::Art::kFrameCount);

        draw_bird(window, art.bird, flappy::player_sprite(tuning, player),
                  dead ? 0 : frame, tilt);

        // Press H to see the 80% collision body over the art.
        if (show_hitboxes) {
            for (const flappy::Gate& gate : field.gates) {
                if (!gate.active) continue;
                window.draw(outlined(flappy::gate_top_body(tuning, gate),
                                     sf::Color(80, 200, 255)));
                window.draw(outlined(flappy::gate_bottom_body(tuning, gate),
                                     sf::Color(80, 200, 255)));
            }
            window.draw(outlined(flappy::player_body(tuning, player),
                                 sf::Color(255, 80, 80)));
        }

        window.display();
    }

    return 0;
}
