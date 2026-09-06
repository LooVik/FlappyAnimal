# iOS build

Everything here is consumed by the **root** `CMakeLists.txt`, not by a separate
build system. That is the main difference from Android: Gradle owns the Android
build and drives its own `CMakeLists.txt` under `platform/android/`, whereas
Xcode is happy to let CMake generate the whole project.

So there is nothing to open in this folder. You generate an Xcode project from
the repository root and open that.

| File | What it is |
| --- | --- |
| `Info.plist` | The app manifest. Counterpart of `AndroidManifest.xml`. |
| `LaunchScreen.storyboard` | Static screen iOS draws while the app loads. Also how iOS knows the app supports full-size modern screens. |

## What you need on the Mac

- **Xcode** (from the App Store). Includes the iOS SDK and the Simulator.
- **CMake 3.28+** — `brew install cmake`
- An **Apple ID**. A free one is enough to run on your own iPhone; builds
  expire after 7 days and you re-run from Xcode to renew. A paid Apple
  Developer Program membership ($99/year) is only needed for TestFlight and
  the App Store.

No Xcode project is checked in, and none should be — it is generated output.

## Simulator first (no Apple ID, no signing)

```bash
cmake --preset ios-simulator
```

That writes `build/ios-simulator/FlappyAnimals.xcodeproj`. Open it, pick an
iPhone simulator at the top of the window, and press Run.

The first configure clones and builds SFML for iOS from source, which takes a
while. Later configures reuse it.

## On a real iPhone

Find your Team ID: <https://developer.apple.com/account> → Membership. It is a
10-character string like `AB12CD34EF`. Then:

```bash
cmake --preset ios-device -DFLAPPY_IOS_TEAM_ID=AB12CD34EF
```

Open `build/ios-device/FlappyAnimals.xcodeproj`, select your iPhone from the
device menu, press Run. The first launch on a new device needs
**Settings → General → VPN & Device Management** on the phone to trust the
developer certificate.

If you skip `FLAPPY_IOS_TEAM_ID`, the project still generates — Xcode will just
ask you to pick a team in *Signing & Capabilities* before it will run on a
device.

## Things worth knowing

**Assets.** `assets/` is copied into the `.app` bundle. SFML changes the working
directory to the bundle's resource path before `main()` runs, so
`art.load("assets")` in `src/main.cpp` needs no iOS-specific code. To confirm a
build actually packaged them:

```bash
ls build/ios-device/Debug-iphoneos/flappy.app/assets
```

**Saved data.** `profile.txt` cannot live next to the executable — the bundle is
signed and read-only. `profile_path()` in `src/main.cpp` returns
`$HOME/Documents/profile.txt` on iOS, which is the app's private sandbox.

**Audio.** SFML uses OpenAL, and on iOS it links Apple's system OpenAL
framework. Nothing to install.

**No tests target.** The root `CMakeLists.txt` skips `flappy_tests` on iOS — a
console test runner cannot be launched on a phone. Run them from the desktop
build instead; the code they cover has no platform-specific paths.

## Publishing later

Not needed to build and play, listed here so it is not a surprise:

1. Paid Apple Developer Program membership.
2. An app record in App Store Connect matching the bundle ID
   `com.loovik.flappyanimals`.
3. A real app icon set (there is none yet — the home screen currently shows a
   blank placeholder).
4. Archive in Xcode → upload → TestFlight → App Review.
5. A privacy policy URL and App Privacy answers, required even for an app that
   collects nothing.
6. Original art and audio. The current placeholders are not clearable for
   release.
