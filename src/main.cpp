#include <SFML/Graphics.hpp>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <cmath>

#include "app/time_step.h"
#include "gameplay/collision.h"
#include "gameplay/gate_field.h"
#include "gameplay/player.h"
#include "gameplay/tuning.h"
#include "gameplay/scoring.h"
#include "gameplay/difficulty.h"
#include "presentation/art.h"
#include "presentation/audio.h"

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

// A gate pillar: a seamless tiled shaft with one cap on the end facing the gap.
// cap_at_bottom is true for the pillar hanging from the ceiling.
void draw_pipe(sf::RenderTarget& target, const flappy::Art& art, const flappy::Rect& dest, bool cap_at_bottom) {
    if (dest.w <= 0.0f || dest.h <= 0.0f) return;

    // Derive the scale from the destination width rather than assuming it, so
    // changing gate_width can never desync the art from the collision box.
    const float scale   = dest.w / static_cast<float>(flappy::Art::kPipeTileW);
    const float cap_h   = flappy::Art::kPipeCapH * scale;
    const float shaft_h = std::max(0.0f, dest.h - cap_h);

    if (shaft_h > 0.0f) {
        sf::Sprite shaft(art.pipe_body);
        shaft.setTextureRect(sf::IntRect({0, 0}, {flappy::Art::kPipeTileW, static_cast<int>(std::ceil(shaft_h / scale))}));
        shaft.setScale({scale, scale});
        shaft.setPosition({dest.x, cap_at_bottom ? dest.y : dest.y + cap_h});
        target.draw(shaft);
    }

    // The source cell has a cap at BOTH ends, so a ceiling pillar just reads
    // the lower one. No mirroring, no negative scale.
    const int cap_src_y = cap_at_bottom ? flappy::Art::kPipeCapH + flappy::Art::kPipeBodyH : 0;

    sf::Sprite cap(art.pipe, sf::IntRect({0, cap_src_y}, {flappy::Art::kPipeTileW, flappy::Art::kPipeCapH}));
    cap.setScale({scale, scale});
    cap.setPosition({dest.x, cap_at_bottom ? dest.y + dest.h - cap_h : dest.y});
    target.draw(cap);
}

// One frame out of the bird atlas, tilted by how fast it is rising or falling.
void draw_bird(sf::RenderTarget& target, const sf::Texture& texture,
               const flappy::Rect& dest, int frame, float tilt_degrees) {
    constexpr int kSize = flappy::Art::kBirdFrame;

    sf::Sprite sprite(texture, sf::IntRect({frame * kSize, 0}, {kSize, kSize}));

    // Rotate about the middle of the frame, not the top-left corner, or the
    // bird swings around its own shoulder.
    sprite.setOrigin({kSize * 0.5f, kSize * 0.5f});
    sprite.setScale({dest.w / static_cast<float>(kSize), dest.h / static_cast<float>(kSize)});
    sprite.setPosition({dest.x + dest.w * 0.5f, dest.y + dest.h * 0.5f});
    sprite.setRotation(sf::degrees(tilt_degrees));
    target.draw(sprite);
}

// Shift a rect for render interpolation. The simulation only knows whole
// steps; alpha() says how far past the last one we are, so we draw ahead.
flappy::Rect nudged(flappy::Rect r, float dx, float dy) {
    r.x += dx;
    r.y += dy;
    return r;
}

void draw_text(sf::RenderTarget& target, const sf::Font& font, const std::string& value, unsigned size, float cx, float cy) 
{
    sf::Text text(font, value, size);
    text.setFillColor(sf::Color::White);
    text.setOutlineColor(sf::Color(20, 20, 20));
    text.setOutlineThickness(static_cast<float>(size) * 0.12f);

    const sf::FloatRect bounds = text.getLocalBounds();
    text.setOrigin({bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f});
    text.setPosition({cx, cy});
    target.draw(text);
}

// ===========================================================================
// DEVELOPMENT TOOLING — TEMPORARY
//
// The collision-box overlay, toggled with H. Before shipping, delete this
// block and its one call site in main().
// ===========================================================================

sf::RectangleShape outlined(const flappy::Rect& r, sf::Color color) {
    sf::RectangleShape shape({r.w, r.h});
    shape.setPosition({r.x, r.y});
    shape.setFillColor(sf::Color::Transparent);
    shape.setOutlineColor(color);
    shape.setOutlineThickness(-2.0f);   // negative draws inward
    return shape;
}

// The 80% collision bodies drawn over the art. Deliberately NOT interpolated:
// these show where the simulation actually thinks things are.
void draw_hitboxes(sf::RenderTarget& target, const flappy::GameTuning& tuning,
                   const flappy::Player& player, const flappy::GateField& field) {
    for (const flappy::Gate& gate : field.gates) {
        if (!gate.active) continue;
        target.draw(outlined(flappy::gate_top_body(tuning, gate),
                             sf::Color(80, 200, 255)));
        target.draw(outlined(flappy::gate_bottom_body(tuning, gate),
                             sf::Color(80, 200, 255)));
    }
    target.draw(outlined(flappy::player_body(tuning, player),
                         sf::Color(255, 80, 80)));
}

}  // namespace

int main() {
    // const again now that nothing mutates it. The compiler will refuse any
    // accidental write, which is exactly what you want from a config object.
    const flappy::GameTuning tuning;

    flappy::Art art;
    if (!art.load("assets")) {
        return 1;   // art.cpp already reported which file was missing
    }

    flappy::Audio audio;
    if(!audio.load("assets"))
    {
        return 1;
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
                    case K::H: show_hitboxes = !show_hitboxes; break;   // debug
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
                audio.flap->play();
            }
        }

        const double elapsed_seconds = static_cast<double>(frame_clock.restart().asMicroseconds()) / 1'000'000.0;
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
            ++step_count;   // drives the bird's animation frame

            const int gained = flappy::score_passed_gates(tuning, field);
            if(gained > 0 )
            {
                audio.score->play();
            }
            score += gained;

            if (flappy::hits_boundary(tuning, player) ||
                flappy::hits_any_gate(tuning, player, field)) {
                dead = true;
                dead_timer = 0.2f;
                audio.hit->play();
                break;
            }
        }

        // ---- draw ----------------------------------------------------------
        window.clear(sf::Color(18, 22, 34));

        // One copy, scaled to cover, anchored to the bottom so the ground strip
        // sits on the floor. Tiling this vertically is what put orange bands
        // across the sky: the image is a whole scene, not a repeating pattern.
        {
            constexpr float kBgScale = 8.0f;   // 256 x 8 = 2048, covers 1080x1920
            const float bg_size = flappy::Art::kBackground * kBgScale;
            sf::Sprite bg(art.background);
            bg.setScale({kBgScale, kBgScale});
            bg.setPosition({0.0f, tuning.reference_height - bg_size});
            window.draw(bg);
        }

        // The simulation only advances in whole 1/60 s steps, but the display
        // does not land on them — some frames get 0 steps, some get 2, and that
        // is the stutter. alpha() is how far past the last step we are, so we
        // draw everything that fraction ahead.
        //
        // Gates move at a constant speed, so their offset is exact arithmetic.
        // The bird is accelerating, so its offset is a first-order estimate
        // from current velocity — which is what every engine does here.
        const bool simulating = flying && !dead;
        const float alpha   = simulating ? static_cast<float>(timestep.alpha()) : 0.0f;
        const float gate_dx = -flappy::current_scroll_speed(tuning, score) * alpha * dt;
        const float bird_dy = player.velocity_y * alpha * dt;

        for (const flappy::Gate& gate : field.gates) {
            if (!gate.active) continue;
            draw_pipe(window, art, nudged(flappy::gate_top_body(tuning, gate), gate_dx, 0.0f), true);
            draw_pipe(window, art, nudged(flappy::gate_bottom_body(tuning, gate), gate_dx, 0.0f), false);
        }

        // Nose up when rising, dive when falling — the whole tilt comes from
        // one number the physics already tracks.
        const float tilt = std::clamp(player.velocity_y / tuning.max_fall_speed,
                                      -1.0f, 1.0f) * 55.0f;
        const int frame = static_cast<int>(
            (step_count / kFramesPerBirdFrame) % flappy::Art::kFrameCount);

        draw_bird(window, art.bird,
                  nudged(flappy::player_sprite(tuning, player), 0.0f, bird_dy),
                  dead ? 0 : frame, tilt);

        draw_text(window, art.font, std::to_string(score), 96, tuning.reference_width * 0.5f, 220.0f);

        if(!flying && !dead) 
        {
            draw_text(window, art.font, " TAP TO START", 48, tuning.reference_width * 0.5f, tuning.reference_height * 0.62f);
        }

        if (show_hitboxes) {   // debug: press H
            draw_hitboxes(window, tuning, player, field);
        }

        window.display();
    }

    return 0;
}
