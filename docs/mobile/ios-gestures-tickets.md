# Tickets: iOS-style system gestures for touch devices

Spec: `docs/mobile/ios-gestures-spec.md`
Integration branch: `feat/ios-gestures` (origin: `Heldinhow/tactility-mobile`)
Definition of done for every ticket: `source ~/esp/esp-idf-v6.1/export.sh && python3 device.py es3c28p && idf.py build` exits 0.

Status vocabulary: `done` (merged on the branch, build green), `in progress`, `blocked`.

Vertical slices: each ticket below is complete through every layer it touches (data source → API → UI)
and verifiable on its own, either by a green build of the device or by the unit tests.

---

## 01: The window manager can list and raise windows

**What to build:** Any system UI can ask which windows are open, bottom-most first, and can bring one
of them to the front without restarting its app. Raising rebuilds the raised window's widgets from the
app's own Context; the app that was on top keeps running with its window buried.

**Blocked by:** None (can start immediately).

**Status:** done

- [x] Listing returns the stack order and copies into a caller-provided buffer, with an offset so a
      caller can read only the most recent entries.
- [x] Listing allocates nothing and takes no LVGL lock, so it is callable from an LVGL event.
- [x] Raising a buried window moves it to the top, deletes the previous top window's widgets, and
      rebuilds the raised window's widgets.
- [x] Raising the topmost window is a no-op; raising an unknown id fails without side effects.
- [x] Raising is serialized with window create/remove and is documented as not callable from an LVGL
      event callback.
- [x] Build green for `es3c28p`.

## 02: Every app instance resolves to its manifest, and icons come from one table

**What to build:** Given any app instance id (not just the topmost one), the app manager reports its
manifest id. The launcher's apps list, its settings list and the new switcher show the same icon for an
app, from one shared lookup.

**Blocked by:** None (can start immediately).

**Status:** done

- [x] Lookup by instance returns the manifest id, or a not-found error for an unknown instance.
- [x] The two launcher mode tables are replaced by one shared lookup; visible icons are unchanged.
- [x] The launcher has an icon for itself, used by the switcher only.
- [x] Build green for `es3c28p`.

## 03: Edge swipes are captured, and the left edge closes the topmost app

**What to build:** A touch drag that starts in a 20 px screen-edge strip belongs to the system; one that
starts anywhere else keeps going to the app. A rightward swipe of at least 40 px from the left edge
closes the topmost app (an app close event, like the toolbar button), and does nothing on the launcher.
The widget under the finger never also receives the click. Gestures are ignored while an overlay is
shown, and on devices without a pointer device nothing is installed.

**Blocked by:** 01, 02.

**Status:** done

- [x] Only presses that start in an edge strip arm a gesture; taps and drags elsewhere are untouched.
- [x] A left-edge drag that travels sideways enough and is mostly horizontal closes the topmost app.
- [x] The same drag on the launcher does nothing.
- [x] The release is consumed so no click reaches the widget under the finger.
- [x] The left edge is not handled when the system bars use the side layout.
- [x] Build green for `es3c28p`.

## 04: Swipe up from the bottom edge goes home

**What to build:** An upward swipe of at least 40 px starting in the bottom edge strip raises the
launcher's window, leaving the apps above it in the stack. When the launcher is already on top, nothing
happens.

**Blocked by:** 03.

**Status:** done

- [x] The launcher's window is found by app id, not by assuming it is the bottom-most window.
- [x] The raise is deferred off the LVGL task and does not block the gesture handler.
- [x] An upward drag that is mostly sideways does not go home.
- [x] Build green for `es3c28p`.

## 05: The recents switcher, from the hold gesture and from the panel

**What to build:** A bottom-edge swipe that stays held for 500 ms opens a switcher overlay: the window
stack as horizontal cards, most recent first, at most five, each showing the app's icon and name.
Tapping a card raises that app; swiping a card up closes that app; swiping the backdrop up or tapping
outside closes the switcher. With a keypad or encoder present, a focused close button is available.

**Blocked by:** 04.

**Status:** done

- [x] The switcher lists at most five windows, topmost first, with icon and name.
- [x] Tapping a card raises the app's window (state preserved, no restart) and hides the switcher.
- [x] Swiping a card up closes that app and hides the switcher.
- [x] The hold timer exists only while a bottom-edge press is down; the release does not also go home.
- [x] The switcher is not opened when no window exists.
- [x] Build green for `es3c28p`.

## 06: The quick panel hangs from the top edge with clock, Wi-Fi status, brightness and recents

**What to build:** The existing quick panel content stays, but the panel becomes a full-width sheet at
the top edge with a pull-down animation. It gains the clock, the Wi-Fi status next to the toggles, a
brightness slider when the display's backlight is dimmable (persisted once, when the panel closes), and
a recents tile. Swiping up or tapping outside still closes it.

**Blocked by:** 02, 05.

**Status:** done

- [x] The panel aligns to the top edge, full width, and slides in over ~180 ms, respecting the theme's
      animation setting.
- [x] A clock label shows the current time, with a placeholder before the clock is set.
- [x] The Wi-Fi status reads off / on / connected network name.
- [x] The brightness slider appears only when a dimmable backlight exists, applies immediately and is
      saved once when the panel closes.
- [x] The recents tile opens the switcher.
- [x] The keypad/encoder close button still works.
- [x] Build green for `es3c28p`.

## 07: The feature can be switched off at build time, and non-touch devices are untouched

**What to build:** A Kconfig option (default on) removes the gesture layer, the switcher and the panel's
recents entry from the firmware when off. With it on but no pointer device, no callbacks are installed.

**Blocked by:** 03.

**Status:** done

- [x] The option exists in the device configuration and defaults to on.
- [x] Turning it off compiles the feature out (the gesture layer and switcher reduce to no-op stubs).
- [x] Without a pointer input device, attaching does nothing.
- [x] Build green for `es3c28p` with the option on.

## 08: Gesture geometry is unit tested

**What to build:** The pure rules behind the gestures are tested without a display: which edge a press
belongs to, what counts as an upward swipe, what counts as a rightward swipe, thresholds, and diagonal
ambiguity.

**Blocked by:** 03.

**Status:** done

- [x] Edge strip boundaries and the bottom-corner precedence are covered.
- [x] Threshold boundaries (40 px counts, 39 does not) are covered for both directions.
- [x] Equal-axis diagonals and the wrong direction are rejected.
- [x] The tests sit with the existing system-bars layout tests, in the same suite.

## 09: Footprint measured against the starting commit, and the work published

**What to build:** The feature's cost is measured on the target device build: application image size and
static DRAM/IRAM from `idf.py size`, before (commit `7462d32`, built in a worktree outside the repo) and
after. The branch carries the spec, these tickets, the review findings, the retro, and a clear commit
history; it is pushed to `origin`.

**Blocked by:** 01–08.

**Status:** done

- [x] Baseline built at `7462d32` in a separate worktree (`/tmp/tm-baseline`, submodules and managed
      components linked from the main checkout). The build directory has to live inside the project
      directory for this repo's CMake (`Tactility/CMakeLists.txt` includes `../Buildscripts/
      device.cmake` by relative path), so it was `/tmp/tm-baseline/build` - outside this repository,
      inside the throwaway worktree.
- [x] `idf.py size` recorded for before and after:

      Metric                      Before        After         Delta
      Tactility.bin               3,881,072 B   3,885,008 B   +3,936 B
      Flash .text (code)          2,766,982 B   2,770,866 B   +3,884 B
      Flash .rodata                 961,872 B     961,920 B   +48 B
      Static DRAM (.data+.bss)       64,975 B      65,007 B   +32 B
      DIRAM in use                  167,494 B     167,526 B   +32 B
      IRAM                           16,384 B      16,384 B   0

- [x] `idf.py build` green for `es3c28p` on the final tree.
- [x] Spec, tickets, review and retro committed on `feat/ios-gestures` and pushed to `origin`.

## 10: Apply the code review's findings

**What to build:** The accepted findings from `docs/mobile/ios-gestures-review.md` are applied: back
acts on the topmost window rather than the newest app instance (and the toolbar's default close with
it), the brightness slider only appears for a dimmable backlight, the home scan walks the whole stack,
a cancelled press cannot leave the gesture state armed, the new app-manager lookup is exported, and
the duplicated helpers (raise, backlight mapping, stack scanning, id casts) are single-sourced.

**Blocked by:** 01–09.

**Status:** done

- [x] Back closes the app of the topmost window; the launcher is never closed.
- [x] On/off backlights do not get a brightness slider.
- [x] The launcher lookup is not limited to a fixed number of windows.
- [x] A press whose release never arrives is cleared by the hold timer or the next press.
- [x] The window-manager and app-manager additions are in the module symbol tables.
- [x] One shared backlight helper serves the Display app and the quick panel.
- [x] One shared system-window helper serves the gestures and the switcher.
- [x] Build green for `es3c28p`; clang-format clean against the repo's `.clang-format`.

---

## Notes on verification

- There is no automated UI harness in this repository, and this work is explicitly not flashed (no
  hardware in the loop for this branch). "Verified" therefore means: the pure logic is unit tested and
  the device firmware builds and links with the feature enabled. On-device feel (timing, thresholds,
  animation) is deliberately left for the next hardware session, and the thresholds are named
  constants so tuning stays a one-line change.
