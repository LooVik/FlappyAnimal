#include <SFML/Graphics.hpp>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <initializer_list>

// --- platform detection -----------------------------------------------------
// These are the ONLY blocks in the file that care which OS we are on. Below
// them the code says FLAPPY_MOBILE and stops thinking about it. Compilers hand
// us __ANDROID__ for free; Apple needs TargetConditionals.h to tell a phone
// apart from a Mac, since both define __APPLE__.
#if defined(__ANDROID__)
    #include <SFML/System/NativeActivity.hpp>
    #include <android/native_activity.h>
    #include <android/window.h>          // AWINDOW_FLAG_KEEP_SCREEN_ON
#elif defined(__APPLE__)
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE
        #define FLAPPY_IOS 1
    #endif
#endif

#if defined(__ANDROID__) || defined(FLAPPY_IOS)
    #define FLAPPY_MOBILE 1
#endif

#include "app/time_step.h"
#include "app/screen.h"
#include "app/run.h"

#include "gameplay/collision.h"
#include "gameplay/gate_field.h"
#include "gameplay/player.h"
#include "gameplay/tuning.h"
#include "gameplay/scoring.h"
#include "gameplay/difficulty.h"
#include "gameplay/medal.h"

#include "presentation/art.h"
#include "presentation/audio.h"

#include "profile/profile.h"

#include "ui/button.h"

namespace {

constexpr std::uint32_t kSeed = 20260808u;  // fixed: same course every run

#if !defined(FLAPPY_MOBILE)
// Desktop only. A phone gets the whole screen, so there is nothing to scale
// down — leaving this defined there would just be an unused-variable warning.
constexpr unsigned kWindowScale = 3;
#endif

// How long each bird animation frame is held, in milliseconds. 100 ms is a
// 10 fps flap, which reads well and is cheap.
//
// This is wall-clock time, not simulation steps, and that is deliberate. The
// flap is pure decoration — no gameplay reads it — so it should keep running on
// the menus, where nothing is simulating at all. Driving it from the step
// counter is what left the bird frozen mid-flap behind the title.
constexpr int kBirdFrameMs = 100;

// Where the saved profile lives.
//
// This has to be a function called from main(), not a global. On Android the
// answer comes from the NativeActivity, and that does not exist yet when
// globals are initialised — the shared library is loaded before Android creates
// the activity, so a global here would dereference a null pointer at startup.
//
// The desktop answer is a bare filename, which resolves against the working
// directory. Neither phone has a writable one:
//   * Android starts the process in "/", which is read-only.
//   * iOS is worse than useless — SFML chdir's into the app bundle, so a bare
//     name would try to write inside the signed, read-only bundle.
// Both platforms hand each app a private writable folder instead. That folder
// is what we want anyway: it survives updates, it is backed up with the device,
// and it disappears when the user uninstalls.
std::filesystem::path profile_path() {
#if defined(__ANDROID__)
    // e.g. /data/data/com.loovik.flappyanimals/files
    return std::filesystem::path(sf::getNativeActivity()->internalDataPath) / "profile.txt";
#elif defined(FLAPPY_IOS)
    // Inside the sandbox, HOME is the app's container, and Documents is the
    // one folder in it meant for user-visible data we create.
    const char* home = std::getenv("HOME");
    return std::filesystem::path(home ? home : ".") / "Documents" / "profile.txt";
#else
    return "profile.txt";
#endif
}

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

// Buttons keep the artwork's 415x111 shape. Choosing a width and deriving the
// height from it means a button can never come out stretched, whatever size we
// pick — the aspect ratio is computed, not typed in and hoped for.
constexpr float button_height(float width) {
    return width * static_cast<float>(flappy::Art::kButtonH)
                 / static_cast<float>(flappy::Art::kButtonW);
}

// The first button in the list containing the point, or nullptr if none do.
// An initializer_list of pointers allocates nothing, and the order doubles as
// priority if two buttons ever overlap.
const flappy::Button* hit_test(std::initializer_list<const flappy::Button*> buttons, sf::Vector2f point) {
    for (const flappy::Button* button : buttons) {
        if (button->contains(point.x, point.y)) {
            return button;
        }
    }
    return nullptr;
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

// A button: its artwork, then its label on top.
//
// Button itself holds no texture, no colour and no font — it is pure geometry
// in flappy_core, which is why it unit tests with no window. Which skin a
// button wears is decided at the call site, so PLAY can be green and HOME red
// without Button knowing either of those words exist.
void draw_button(sf::RenderTarget& target, const sf::Font& font, const flappy::ButtonSkin& skin,
                 const flappy::Button& button, bool pressed, unsigned text_size)
{
    const sf::Texture& texture = pressed ? skin.pressed : skin.normal;

    sf::Sprite sprite(texture);
    sprite.setScale({button.bounds.w / static_cast<float>(flappy::Art::kButtonW),
                     button.bounds.h / static_cast<float>(flappy::Art::kButtonH)});
    sprite.setPosition({button.bounds.x, button.bounds.y});

    // A disabled button is drawn greyed and dimmed rather than swapped for a
    // fourth image — the pack has no disabled state, and washing the colour out
    // is the convention players already read as "not now".
    if (!button.enabled) {
        sprite.setColor(sf::Color(150, 150, 150, 190));
    }
    target.draw(sprite);

    // Follow the face down when the button is pushed. Centring on the whole
    // rectangle would leave the text sitting over the drop shadow.
    const float face = pressed ? flappy::Art::kButtonFacePressed : flappy::Art::kButtonFaceNormal;

    draw_text(target, font, button.label, text_size,
              button.bounds.x + button.bounds.w * 0.5f,
              button.bounds.y + button.bounds.h * face);
}

// The dark wash behind every panel. Two jobs: it makes the panel readable over
// whatever the game was drawing, and it visually says "the thing underneath is
// not accepting taps right now".
void draw_dim(sf::RenderTarget& target, const flappy::GameTuning& tuning)
{
    sf::RectangleShape dim({tuning.reference_width, tuning.reference_height});
    dim.setFillColor(sf::Color(10, 12, 20, 170));
    target.draw(dim);
}

void draw_panel(sf::RenderTarget& target, float cx, float mid_y, float w, float h)
{
    sf::RectangleShape panel({w, h});
    panel.setOrigin({w * 0.5f, h * 0.5f});
    panel.setPosition({cx, mid_y});
    panel.setFillColor(sf::Color(28, 34, 48, 235));
    panel.setOutlineColor(sf::Color(240, 200, 90));
    panel.setOutlineThickness(6.0f);
    target.draw(panel);
}

void draw_result(sf::RenderTarget& target, const flappy::Art& art, const flappy::GameTuning& tuning,
                 int score, int best, const flappy::Button& retry, const flappy::Button& home,
                 const flappy::Button* held)
{
    const float cx      = tuning.reference_width * 0.5f;
    const float panel_w = 800.0f;
    const float panel_h = 900.0f;
    const float mid_y   = tuning.reference_height * 0.45f;

    draw_dim(target, tuning);
    draw_panel(target, cx, mid_y, panel_w, panel_h);

    const float top = mid_y - panel_h * 0.5f;

    draw_text(target, art.font, "GAME OVER", 56, cx, top + 90.0f);
    draw_text(target, art.font, "SCORE", 32, cx, top + 220.0f);
    draw_text(target, art.font, std::to_string(score), 72, cx, top + 295.0f);
    draw_text(target, art.font, "BEST", 32, cx, top + 405.0f);
    draw_text(target, art.font, std::to_string(best), 72, cx, top + 480.0f);

    const flappy::Medal medal = flappy::medal_for_score(score);
    if(medal != flappy::Medal::None)
    {
        draw_text(target, art.font, flappy::medal_name(medal), 40, cx, top + 580.0f);
    }

    draw_button(target, art.font, art.button_green, retry, held == &retry, 32);
    draw_button(target, art.font, art.button_red,   home,  held == &home,  32);
}

// Local scores only. The online ranks this screen is really for arrive with
// the backend in Milestone 6 — the panel exists now so the navigation around
// it can be built and tested first.
void draw_leaderboard(sf::RenderTarget& target, const flappy::Art& art, const flappy::GameTuning& tuning,
                      int best, const flappy::Button& close, const flappy::Button* held)
{
    const float cx      = tuning.reference_width * 0.5f;
    const float panel_w = 860.0f;
    const float panel_h = 820.0f;
    const float mid_y   = tuning.reference_height * 0.45f;

    draw_dim(target, tuning);
    draw_panel(target, cx, mid_y, panel_w, panel_h);

    const float top = mid_y - panel_h * 0.5f;

    draw_text(target, art.font, "LEADERBOARD", 44, cx, top + 90.0f);
    draw_text(target, art.font, "YOUR BEST", 32, cx, top + 250.0f);
    draw_text(target, art.font, std::to_string(best), 96, cx, top + 350.0f);
    draw_text(target, art.font, "WORLD RANKS", 24, cx, top + 520.0f);
    draw_text(target, art.font, "COMING SOON", 24, cx, top + 570.0f);

    draw_button(target, art.font, art.button_red, close, held == &close, 32);
}

void draw_home(sf::RenderTarget& target, const flappy::Art& art, const flappy::GameTuning& tuning,
               int best, const flappy::Button& play, const flappy::Button& leaderboard,
               const flappy::Button* held)
{
    const float cx = tuning.reference_width * 0.5f;

    draw_text(target, art.font, "FLAPPY",  72, cx, 420.0f);
    draw_text(target, art.font, "ANIMALS", 72, cx, 530.0f);

    draw_text(target, art.font, "BEST " + std::to_string(best), 28, cx, 660.0f);

    draw_button(target, art.font, art.button_green, play,        held == &play,        36);
    draw_button(target, art.font, art.button_blue,  leaderboard, held == &leaderboard, 32);
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

#if defined(FLAPPY_MOBILE)
    // A phone has no window manager: the window IS the screen, and any size we
    // ask for is ignored. Saying so explicitly — desktop mode, fullscreen — is
    // clearer than passing a made-up size and hoping, and Fullscreen also hides
    // the status bar so the whole panel is play area.
    sf::RenderWindow window(sf::VideoMode::getDesktopMode(), "FlappyAnimals", sf::State::Fullscreen);

    // Phones ship 90 Hz and 120 Hz panels. Vsync would render every one of
    // those frames — double the GPU work, double the battery, double the heat —
    // for a simulation that only advances 60 times a second. Nothing on screen
    // would look better for it. Cap the frame rate instead.
    window.setFramerateLimit(60);
#else
    sf::RenderWindow window(
        sf::VideoMode({static_cast<unsigned>(tuning.reference_width)  / kWindowScale,
                       static_cast<unsigned>(tuning.reference_height) / kWindowScale}),
        "FlappyAnimals");
    window.setVerticalSyncEnabled(true);
#endif

#if defined(__ANDROID__)
    // Android dims and locks the screen on an inactivity timer, and only counts
    // touches as activity. This game can go a long time between taps, so
    // without this flag the screen dies mid-run.
    ANativeActivity_setWindowFlags(sf::getNativeActivity(), AWINDOW_FLAG_KEEP_SCREEN_ON, 0);
#endif

    sf::View view(sf::FloatRect({0.0f, 0.0f}, {tuning.reference_width, tuning.reference_height}));
    window.setView(letterboxed(view, window.getSize()));

    flappy::FixedTimestep timestep;
    sf::Clock frame_clock;

    // Never restarted. It only feeds the cosmetic flap cycle, so it can just
    // count upwards for the whole session.
    sf::Clock anim_clock;

    flappy::Run run;
    run.reset(tuning, kSeed);
    run.screen = flappy::Screen::Home;   // reset() lands in Ready; the app opens on Home

    // --- UI layout ---------------------------------------------------------
    // Built once, in view coordinates (1080 x 1920), never per frame. These are
    // the equivalent of laying out a Canvas in the Unity editor — except the
    // numbers are here in the source instead of in a scene file, so a diff
    // actually shows when a button moves.
    const float ui_cx = tuning.reference_width * 0.5f;

    constexpr float kMenuW  = 600.0f;   // Home: room for the word LEADERBOARD
    constexpr float kPanelW = 480.0f;   // inside a panel, so narrower

    const flappy::Button home_play        = flappy::centered_button("PLAY",        ui_cx, 1150.0f, kMenuW, button_height(kMenuW));
    const flappy::Button home_leaderboard = flappy::centered_button("LEADERBOARD", ui_cx, 1340.0f, kMenuW, button_height(kMenuW));

    // The results panel is 900 tall centred at 45% of the screen, so its top
    // edge is at 864 - 450 = 414. These two sit at top + 700 and top + 830.
    const flappy::Button results_retry = flappy::centered_button("RETRY", ui_cx, 1114.0f, kPanelW, button_height(kPanelW));
    const flappy::Button results_home  = flappy::centered_button("HOME",  ui_cx, 1244.0f, kPanelW, button_height(kPanelW));

    const flappy::Button leaderboard_close = flappy::centered_button("CLOSE", ui_cx, 1130.0f, kPanelW, button_height(kPanelW));

    // The button the finger or mouse is currently holding down, if any.
    // Compared by address, which is why the buttons above are stable locals.
    const flappy::Button* held = nullptr;

    // Spans runs, so it is not part of Run. load() returns false on a first
    // launch, which is not an error — there is simply nothing saved yet.
    flappy::Profile profile;
    const std::filesystem::path profile_file = profile_path();
    profile.load(profile_file);

    bool show_hitboxes = false;  // debug

    // Whether the app is in the foreground. Only ever false on a phone, where
    // the user can switch away mid-run.
    bool has_focus = true;

    while (window.isOpen()) {
        // Three separate facts about this frame's input, because they answer
        // different questions:
        //   tapped         — did a press happen at all? Space counts, and it
        //                    has no position.
        //   press_point    — WHERE the press landed, in view coordinates.
        //   release_point  — where the finger or button came back up.
        //
        // Gameplay uses the press: a flap has to happen the instant you touch
        // the screen. UI uses the release, which is what every real button
        // does — press to arm it, release on it to fire, slide off to cancel.
        bool tapped = false;
        std::optional<sf::Vector2f> press_point;
        std::optional<sf::Vector2f> release_point;

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
            } else if (const auto* mouse = event->getIf<sf::Event::MouseButtonPressed>()) {
                tapped      = true;
                // Pixels are NOT view coordinates. The letterbox puts black
                // bars around the game, so pixel (0,0) is a corner of the
                // window, not of the play area — and on a phone the whole
                // 1080x1920 design is squeezed onto a real panel of some other
                // size. mapPixelToCoords undoes both, using the view currently
                // on the window. Without it every button would be hit in the
                // wrong place, and differently wrong on each screen size.
                press_point = window.mapPixelToCoords(mouse->position);
            } else if (const auto* released = event->getIf<sf::Event::MouseButtonReleased>()) {
                release_point = window.mapPixelToCoords(released->position);
            } else if (const auto* touch = event->getIf<sf::Event::TouchBegan>()) {
                // A phone never sends MouseButtonPressed for a finger — SFML
                // treats touch and mouse as different devices and picks one.
                // Both branches stay: desktop keeps the mouse, phone gets this.
                tapped      = true;
                press_point = window.mapPixelToCoords(touch->position);
            } else if (const auto* lifted = event->getIf<sf::Event::TouchEnded>()) {
                release_point = window.mapPixelToCoords(lifted->position);
            } else if (event->is<sf::Event::FocusLost>()) {
                has_focus = false;
                // Whatever was being held is not being held any more — the
                // release will land in another app. Without this the button
                // would still be drawn pushed in when the player comes back.
                held = nullptr;
            } else if (event->is<sf::Event::FocusGained>()) {
                has_focus = true;

                // Android rebuilds its drawing surface here and updates the
                // window size WITHOUT sending Resized — that event simply does
                // not exist on Android. Re-apply the letterbox by hand or the
                // view keeps the pre-suspend size, which is wrong the moment
                // the user rotated or opened a split-screen app.
                window.setView(letterboxed(view, window.getSize()));

                // Throw away however long the app sat in the background.
                // Without this the next frame reports minutes of elapsed time
                // and the game fast-forwards through the run.
                frame_clock.restart();
            }
        }

        // Backgrounded: the user has moved on. On Android the drawing surface
        // is already destroyed by this point, so there is genuinely nothing to
        // draw to — and spinning the loop at 60 Hz would keep the CPU and GPU
        // hot in someone's pocket. Idle until they come back.
        if (!has_focus) {
            sf::sleep(sf::milliseconds(100));
            continue;
        }

        // --- press: arm whichever button is under the finger ------------------
        // Only the current screen's buttons are live. A press anywhere else
        // arms nothing, which is how a press on the panel background or the
        // letterbox bars quietly does nothing at all.
        if (press_point) {
            switch (run.screen) {
                case flappy::Screen::Home:
                    held = hit_test({&home_play, &home_leaderboard}, *press_point);
                    break;
                case flappy::Screen::Results:
                    held = hit_test({&results_retry, &results_home}, *press_point);
                    break;
                case flappy::Screen::Leaderboard:
                    held = hit_test({&leaderboard_close}, *press_point);
                    break;
                default:
                    held = nullptr;
                    break;
            }
        }

        // --- flap: gameplay reacts to the PRESS, not the release --------------
        // Waiting for the release would add the length of your own tap to the
        // input latency, and a flap has to feel instant. A screen showing
        // buttons never flaps — pressing HOME must not also flap the bird
        // behind the panel.
        if (tapped && (run.screen == flappy::Screen::Ready || run.screen == flappy::Screen::Playing)) {
            run.screen = flappy::Screen::Playing;
            run.player.flap(tuning);
            audio.flap->play();
        }

        // --- release: fire the armed button, if the finger is still on it -----
        if (release_point) {
            if (held && held->contains(release_point->x, release_point->y)) {
                if (held == &home_play || held == &results_retry) {
                    run.reset(tuning, kSeed);           // lands in Ready
                } else if (held == &home_leaderboard) {
                    run.screen = flappy::Screen::Leaderboard;
                } else if (held == &results_home || held == &leaderboard_close) {
                    run.reset(tuning, kSeed);
                    run.screen = flappy::Screen::Home;
                }
                audio.flap->play();
            }
            held = nullptr;   // released is released, on the button or not
        }

        const double elapsed_seconds = static_cast<double>(frame_clock.restart().asMicroseconds()) / 1'000'000.0;
        const float dt = static_cast<float>(timestep.step_seconds);

        for (int i = 0, steps = timestep.accumulate(elapsed_seconds); i < steps; ++i) {

            if(run.screen == flappy::Screen::Dying)
            {
                run.dead_timer -= dt;
                if(run.dead_timer <= 0.0f)
                {
                    if (run.score > profile.best_standard)
                    {
                        profile.best_standard = run.score;
                        profile.save(profile_file);   // only on a NEW best
                    }
                    run.screen = flappy::Screen::Results;
                }
                continue;
            }

            if(run.screen != flappy::Screen::Playing)
            {
                break;
            }

            run.player.step(tuning, dt);
            run.field.step(tuning, dt, flappy::current_scroll_speed(tuning, run.score), flappy::current_gate_gap(tuning, run.score));
            // How many fixed steps this run has lasted. Not the animation any
            // more — this is deterministic run state, and it is what a server
            // would check a submitted score against in Milestone 6: a score of
            // 40 from a run that only lasted 30 steps did not happen.
            ++run.step_count;

            const int gained = flappy::score_passed_gates(tuning, run.field);
            if(gained > 0 )
            {
                audio.score->play();
            }
            run.score += gained;
            
            //collison between birds and pipe.
            if (flappy::hits_boundary(tuning, run.player) ||
                flappy::hits_any_gate(tuning, run.player, run.field)) {
                run.screen = flappy::Screen::Dying;
                run.dead_timer = 0.2f;
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
        const bool simulating = (run.screen == flappy::Screen::Playing);
        const float alpha   = simulating ? static_cast<float>(timestep.alpha()) : 0.0f;
        const float gate_dx = -flappy::current_scroll_speed(tuning, run.score) * alpha * dt;
        const float bird_dy = run.player.velocity_y * alpha * dt;

        for (const flappy::Gate& gate : run.field.gates) {
            if (!gate.active) continue;
            draw_pipe(window, art, nudged(flappy::gate_top_body(tuning, gate), gate_dx, 0.0f), true);
            draw_pipe(window, art, nudged(flappy::gate_bottom_body(tuning, gate), gate_dx, 0.0f), false);
        }

        // Nose up when rising, dive when falling — the whole tilt comes from
        // one number the physics already tracks.
        const float tilt = std::clamp(run.player.velocity_y / tuning.max_fall_speed, -1.0f, 1.0f) * 55.0f;
        const int frame = static_cast<int>(
            (anim_clock.getElapsedTime().asMilliseconds() / kBirdFrameMs) % flappy::Art::kFrameCount);

        // The bird keeps flapping on Home and Leaderboard too — a frozen bird
        // behind the title reads as a crash. It only goes still when it is
        // actually dead.
        const bool alive = (run.screen != flappy::Screen::Dying && run.screen != flappy::Screen::Results);

        draw_bird(window, art.bird, nudged(flappy::player_sprite(tuning, run.player), 0.0f, bird_dy), alive ? frame : 0, tilt);

        // The live score belongs to a run in progress. Home and Leaderboard
        // have no run, so a "0" floating there would just be noise.
        const bool in_run = (run.screen == flappy::Screen::Ready ||
                             run.screen == flappy::Screen::Playing ||
                             run.screen == flappy::Screen::Dying ||
                             run.screen == flappy::Screen::Results);
        if (in_run)
        {
            draw_text(window, art.font, std::to_string(run.score), 96, tuning.reference_width * 0.5f, 220.0f);
        }

        if(run.screen == flappy::Screen::Ready)
        {
            draw_text(window, art.font, " TAP TO START", 48, tuning.reference_width * 0.5f, tuning.reference_height * 0.62f);
        }

        if (show_hitboxes) {   // debug: press H
            draw_hitboxes(window, tuning, run.player, run.field);
        }

        // Panels last, so they sit over everything the run drew.
        if(run.screen == flappy::Screen::Home)
        {
            draw_home(window, art, tuning, profile.best_standard, home_play, home_leaderboard, held);
        }

        if(run.screen == flappy::Screen::Results)
        {
            draw_result(window, art, tuning, run.score, profile.best_standard, results_retry, results_home, held);
        }

        if(run.screen == flappy::Screen::Leaderboard)
        {
            draw_leaderboard(window, art, tuning, profile.best_standard, leaderboard_close, held);
        }

        window.display();
    }

    return 0;
}
