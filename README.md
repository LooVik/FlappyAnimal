# FlappyAnimals

An original portrait arcade game for Android and iOS, written in C++20 with SFML 3.
The player travels an endless obstacle course, scoring one point per gate cleared,
and competes on regional and worldwide leaderboards.

Currently at **Milestone 0**: a portrait window driven by a fixed 1/60 s simulation
loop. No gameplay yet.

## Prerequisites

- **Visual Studio Build Tools 2026** with the *Desktop development with C++* workload.
  This provides MSVC and bundles both CMake and Ninja.
- **Git**

Nothing else. SFML 3 and doctest are downloaded and built by CMake automatically.

## Building on Windows

CMake and Ninja live inside Build Tools and are not on the system PATH, so every
session starts by opening a developer shell:

```powershell
& "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\Tools\Launch-VsDevShell.ps1" -Arch amd64 -SkipAutomaticLocation
```

Then from the repository root:

```powershell
cmake --preset windows-debug
```

> The first configure clones and compiles SFML 3 from source. Expect several minutes,
> once — it is cached afterwards and later configures take under a second.

```powershell
cmake --build --preset windows-debug
ctest --preset windows-debug
.\build\windows-debug\bin\flappy.exe
```

You should see a portrait window with a yellow marker crossing it once per second,
and a console reporting `steps/sec` and `frames/sec`.

## Day to day

After a code change, only these two are needed:

```powershell
cmake --build --preset windows-debug; ctest --preset windows-debug
```

Re-run `cmake --preset windows-debug` only when `CMakeLists.txt` changes — a new
dependency, source file, or target.

## Layout

```
src/app/      gameplay logic — never includes SFML, so tests run with no window
src/main.cpp  SFML window, event pump, fixed-step loop
tests/        doctest unit tests
docs/         product spec and plans (kept local, not tracked)
```

## Platforms

Windows only so far. Android is Milestone 2, iOS is Milestone 7.