# Code review: iOS-style gestures (feat/ios-gestures)

Reviewed range: `7462d32...HEAD` (commits f92726b2, d117b591, 6c307e93, 9e726f31, 2a428bc9, plus the
fix commit that follows this document).
Spec: `docs/mobile/ios-gestures-spec.md`. Tickets: `docs/mobile/ios-gestures-tickets.md`.
Two axes, reviewed separately by independent reviewers: **Standards** (repo standards +
Fowler smell baseline) and **Spec** (does the code do what the spec asked).

## Standards

Hard violations of the repo's documented standards: **none**. `CODING_STYLE_C.md`,
`CODING_STYLE_CPP.md`, `CONTRIBUTING.md`, the doc-comment shape of the existing `SystemBars.h` /
`SystemMenu.h`, the file locations and the `enum class`/no-exceptions rules were all respected. The
touched files are clang-format clean against the repo's `.clang-format`.

Judgement calls (smell baseline):

1. **Duplicated Code** - `raise_on_main_dispatcher` + `raiseWindow` were verbatim copies in
   `SystemGestures.cpp` and `SystemRecents.cpp`.
2. **Duplicated Code + Shotgun Surgery** - the backlight percentage mapping was copied from the
   Display app into the quick panel, and the "find the display backlight" lookup with it.
3. **Duplicated Code** - two window-stack scan strategies: `findLauncherWindow` used a fixed
   16-entry array, `findAppInstance`/`collectEntries` used chunked reads.
4. **Middle Man** - `hideOverlay()` only delegated to `window_manager_overlay_hide()`.
5. **Primitive Obsession / repeated cast** - `reinterpret_cast<void*>(static_cast<uintptr_t>(...))`
   and its inverse recurred five times, and `0` stood in for "no window" without a name.
6. **Doc nit** - one sentence in `app/manager.h` referred to "that case" with no antecedent; an
   unused `<cstdio>` include in `SystemMenu.cpp`.

## Spec

Blockers:

1. **(c) Back closed the wrong app after a raise.** The back gesture used the newest *active app
   instance*, not the window manager's top window. After Home or a recents switch, the launcher is on
   top while the newest instance is buried, so back would close a buried app. Spec: "The window stack
   stays the single source of truth for 'which apps are open and in what order'." The same bug existed
   in the toolbar's default close action, which the spec says back mirrors.
2. **(c) 0-1 backlights got a useless slider.** The check was `max > min`, so an on/off backlight
   showed a 0-100 slider in which every position means "on". Spec: "a brightness slider when the
   device has a dimmable backlight".

Suggestions / partial requirements:

3. **(a)** Home only scanned the first 16 windows, while the spec calls the stack unbounded.
4. **(c)** The pointer device's `LV_EVENT_PRESS_LOST` callback never fires (LVGL sends that event to
   the active object), so a cancelled press could leave the gesture state armed.
5. **(a)** `app_manager_get_instance_app_id` was missing from the app module's exported symbol table,
   so dynamically loaded apps could not call it.
6. **(a)** Ticket 09 (footprint + verification record) was still open.

Scope creep: none.

## Disposition

| # | Axis | Finding | Decision |
|---|------|---------|----------|
| S1 | Standards | duplicated raise helper | Fixed: one `raiseWindow()` in the new shared system-window header, used by both. |
| S2 | Standards | duplicated backlight mapping | Fixed: `Tactility/Private/Tactility/Backlight.h` holds the display-backlight lookup, the dimmable test and the percentage mapping; the Display app and the quick panel both use it. |
| S3 | Standards | two scan strategies | Fixed: one chunked `forEachWindow()` walks the unbounded stack; both lookups use it. |
| S4 | Standards | middle man | Fixed: callers call the window manager directly. |
| S5 | Standards | repeated casts / magic 0 | Fixed: `toWindowContext()`/`fromWindowContext()` in the shared header, and `WINDOW_ID_NONE` in the window manager's public header. |
| S6 | Standards | doc nit, unused include | Fixed. |
| P1 | Spec | back closed the wrong app | Fixed: back closes the app of the **topmost window**; the toolbar's default close action was corrected the same way. |
| P2 | Spec | 0-1 backlight slider | Fixed: the shared `isDimmable()` excludes on/off ranges, so the slider only appears where it means something. |
| P3 | Spec | 16-window home scan | Fixed by the same `forEachWindow()`; the limit and its constant are gone. |
| P4 | Spec | `PRESS_LOST` never reaches the indev | Fixed differently: the callback is removed, and the hold timer plus the next press self-heal from `lv_indev_get_state()`, which covers cancelled presses whatever event they produce. |
| P5 | Spec | missing module symbol | Fixed: registered in the app module's symbol table. |
| P6 | Spec | ticket 09 open | Fixed: baseline measured in a worktree at `7462d32` with an out-of-repo build directory, values recorded in the tickets doc. |

Rejected: none.

## Measurements after the fixes

`idf.py size`, device `es3c28p`, baseline `7462d32` vs `feat/ios-gestures`:

| Metric | Before | After | Delta |
|---|---|---|---|
| `Tactility.bin` | 3,881,072 B | 3,885,008 B | **+3,936 B** |
| Flash `.text` (code) | 2,766,982 B | 2,770,866 B | +3,884 B |
| Flash `.rodata` | 961,872 B | 961,920 B | +48 B |
| Static DRAM (`.data` + `.bss`) | 64,975 B | 65,007 B | **+32 B** |
| DIRAM in use | 167,494 B (49.01%) | 167,526 B (49.02%) | +32 B |
| IRAM | 16,384 B (100%) | 16,384 B (100%) | 0 |

The whole feature costs ~3.9 KB of flash and 32 bytes of static RAM, with no IRAM and no `.data`
growth. With `CONFIG_TT_SYSTEM_GESTURES` off, the gesture layer, the switcher and the panel's recents
entry compile out.

## Verdict

Both axes green after the fixes above. Build for `es3c28p` passes; the gesture geometry unit test
passes when compiled and run standalone (the repository's doctest target could not be built in this
environment - see the retro).
