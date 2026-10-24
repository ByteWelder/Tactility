# Retro: iOS-style gestures session

Session scope: design (grilling) → spec → tickets → implementation → review → retrofit fixes, on
branch `feat/ios-gestures`. This document records improvements to the **environment** (repo, tooling,
steering files), not to the feature. Ordered by severity.

## 1. The repo's test target cannot be built on a stock macOS checkout (guardrail gap)

`tests.yml` builds everything through `cmake --build build --target build-tests`; locally that fails
in two ways before it ever reaches a test:

- `Tests/Tactility/CMakeLists.txt` (and the other generated-asset steps) invoke `python`, which does
  not exist on macOS. A `python3` symlink is needed on `PATH`.
- mbedtls' `generate_driver_wrappers.py` needs `jsonschema`, which is not in the system python.

Effect: the only automated guardrail for this kind of change (pure-logic tests in the Tactility suite)
is unreachable locally. The committed gesture test was therefore verified by compiling that single
translation unit with doctest directly (20 assertions, passing), not through the project's test
target.

Suggested fix: document (and script) the local test prerequisites - a `requirements-dev.txt` plus a
`tt.py test` command that creates a venv and points `python` at it. The current CI already proves the
build works on Linux; the gap is purely the local path.

## 2. `.clang-format` exists, but nothing enforces it (existing check, unwired)

The repo ships `.clang-format`, yet neither `build.yml` nor `tests.yml` runs it, and there is no
pre-commit hook. The diff happened to be format-clean when checked by hand, but nothing would have
caught it if it were not.

Suggested fix: one small CI job (or pre-commit hook) running
`clang-format --dry-run --Werror` on changed files. That is a mechanical standard, so it belongs in a
deterministic check rather than in a review checklist.

## 3. Navigation pointer: where "which app is showing" is answered

The hardest fact to establish in this session was that the window manager keeps **only the topmost
window's widgets** and rebuilds a buried window through its `create_widgets` callback, while the app
scheduler never stops a running app. Those two facts together are the whole "app stack" model, and
they are spread across `window_manager.h`, `scheduler.cpp` and `manager.cpp`.

Suggested fix: a two-line navigation pointer in `app/manager.h` (the header every app-facing
integrator reads) saying that visibility and ordering live in the window manager's stack, and that a
buried app keeps running. Cheap context load, removes an exploration detour.

## 4. Mechanical rule worth turning into a check: raising off the LVGL task

`window_manager_raise()` must not run on the LVGL task (lock-order inversion against
`window_manager_create()`/`remove()`). The diff encodes this with a helper (`raiseWindow()`) and a
`@warning`, and the reviewer had to re-derive it. The existing overlay APIs are safe from LVGL
callbacks, so "call the window manager from a widget callback" looks safe until it deadlocks.

Suggested fix: an assertion in `window_manager_raise()` that fails loudly when called with the LVGL
lock held by the current task (the LVGL port exposes a recursive lock; a debug-only `lvgl_try_lock`
probe distinguishes the cases). That converts a comment into a deterministic trap.

## 5. Tool economy: `idf.py size` re-runs the build

`idf.py size` regenerates assets/fonts and relinks before printing the table, which cost about a
minute per measurement (before and after). Reading `build/Tactility.map` directly with
`python -m esp_idf_size` gives the same numbers without touching the build. Worth a note next to the
size instructions if size comparisons become routine (they are part of embedded reviews here).

## 6. Information access: the brightness row cannot be seen on the target board

`Devices/es3c28p/es3c28p.dts` disables `display_backlight`, so the quick panel correctly hides the
brightness slider on the only device this branch targets. The feature's most visible new control is
therefore unverifiable until a board with a dimmable backlight is available, or the devicetree node is
enabled in a hardware session.

Suggested fix: none in code (deliberately out of scope); a follow-up ticket to test on a board with a
dimmable backlight would keep it from rotting.

## What went well

- Grilling before coding removed the two most expensive unknowns (whether "back" means close-the-app,
  and whether a raise API had to exist).
- Pinning the baseline commit in a separate worktree with an in-worktree build directory made the
  before/after numbers unambiguous. Note for future sessions: this repo's CMake includes
  `../Buildscripts/device.cmake` by relative path, so the build directory must be a direct child of
  the project directory - a truly external build directory does not configure.
- Two-axis review paid off: the spec axis found a real correctness bug (back closing the wrong app
  after a raise) that a standards-only pass would have missed.
