# iOS-style system gestures for Touch Devices

Status: implemented (branch `feat/ios-gestures`, device `es3c28p`, 240x320)

## Problem Statement

On a Tactility touch device, nothing brings the user back to the launcher without closing every open
app through its toolbar. Apps stay resident when another app is started, but they become invisible:
the window manager only keeps widgets for the topmost window, so a buried app can only surface again
when the app above it is closed. The user has no gesture to go home, no way to switch between running
apps, and no edge gesture to leave the current app. The quick panel that the statusbar opens today
shows toggles only: no clock, no Wi-Fi status, no brightness, and no way to reach the running apps.

## Solution

Three edge gestures from the screen edges, plus a recents switcher:

- **Swipe up from the bottom edge** raises the launcher, keeping the apps above it in the window stack.
- **Swipe up from the bottom edge and hold** opens the recents switcher: the window stack as
  horizontal cards, most recent first, at most five. Tapping a card raises that app; swiping a card up
  closes that app.
- **Swipe right from the left edge** closes the topmost app, which reveals the window below it (the
  same effect as the toolbar close button). It does nothing on the launcher.

The quick panel (swipe down from the statusbar, as before) becomes a dimmed sheet that hangs from the
top edge and shows the clock, the Wi-Fi status next to its toggle, a brightness slider when the device
has a dimmable backlight, and a recents button beside the existing toggles.

Gestures only take drags that **start** in a 20 px strip along an edge, and only when the swipe travels
at least 40 px in the expected direction. Everywhere else, apps keep their own swipes (launcher page
swipes, list scrolling, carousels). The widget under the finger never also receives the click.

## User Stories

1. As a touch-device user, I want to swipe up from the bottom edge to go home, so that I can reach the
   launcher without closing the app I was using.
2. As a touch-device user, I want the app I left to keep running while I am on the launcher, so that
   returning to it does not lose my place.
3. As a touch-device user, I want to swipe up and keep my finger down to see the running apps, so that
   I can choose between them instead of guessing.
4. As a touch-device user, I want the recents switcher to show the most recent app first, so that the
   app I just left is the first card.
5. As a touch-device user, I want the recents switcher to show at most five cards, so that it stays
   readable on a 240x320 screen.
6. As a touch-device user, I want to tap a recents card to switch to that app, so that switching keeps
   the app's state.
7. As a touch-device user, I want to swipe a recents card up to close that app, so that I can free
   memory without hunting for the app.
8. As a touch-device user, I want each recents card to show the app's icon and name, so that I can
   recognise it at a glance.
9. As a touch-device user, I want the launcher to appear as a card in the switcher, so that I can go
   home from the switcher with the same gesture.
10. As a touch-device user, I want to swipe right from the left edge to leave the current app, so that I
    do not have to find its close button.
11. As a touch-device user, I want the back gesture to do nothing on the launcher, so that I cannot
    close my home screen by accident.
12. As a touch-device user, I want a swipe that starts in the middle of the screen to keep going to the
    app, so that paging and scrolling never fight the system gestures.
13. As a touch-device user, I want a drag that starts at an edge to not also click the widget under my
    finger, so that a gesture never triggers a button by accident.
14. As a touch-device user, I want the quick panel to show the current time, so that I do not have to
    close it to check the clock.
15. As a touch-device user, I want the quick panel to show whether Wi-Fi is on and which network it is
    on, so that I can tell the state without opening the Wi-Fi app.
16. As a touch-device user, I want to change the brightness from the quick panel, so that I can dim the
    screen without going to the Display settings app.
17. As a touch-device user, I want the brightness I chose to be remembered, so that it survives a
    reboot.
18. As a touch-device user, I want a recents button in the quick panel, so that I can reach the switcher
    without remembering the hold gesture.
19. As a touch-device user, I want the quick panel to slide in from the top edge, so that it reads as
    the thing my swipe pulled down.
20. As a touch-device user, I want to close the quick panel by swiping up or tapping beside it, so that
    dismissing it needs no target.
21. As a user of a device whose left edge holds the statusbar strip (side layout), I want the left-edge
    back gesture disabled, so that the strip keeps working as before.
22. As a user of a keypad- or encoder-only device, I want the firmware to behave exactly as before, so
    that no gesture code runs where there is no touch screen.
23. As a user of a keypad- or encoder-only device that does show the quick panel, I want to reach and
    close the recents switcher with the keys, so that I am not trapped in a screen I cannot dismiss.
24. As a device integrator, I want to switch the whole gesture feature off at build time, so that the
    code is not in the firmware on devices that do not need it.
25. As a device integrator, I want the gestures to cost no RAM or CPU while no finger is on the screen,
    so that the feature does not slow down the rest of the system.
26. As a device integrator, I want the feature to add no new dependencies, so that the firmware size
    stays predictable.
27. As a developer, I want the gesture geometry (edge strips, thresholds, direction rules) to be plain
    arithmetic that can be unit tested without a display, so that the edge cases stay covered.
28. As a developer, I want raising a window to be a window-manager operation, so that apps do not need
    to know about the switcher or the home gesture.

## Implementation Decisions

### Window stack

- The window manager gains two read/raise operations: enumerate the window stack (bounded copy, with
  an offset so a caller can read only the most recent entries) and raise a window by id.
- Raising moves the window's record to the top of the stack and rebuilds its widgets through the
  `create_widgets` callback it registered. The raised app is not restarted, so the state it keeps in
  its own Context survives. The widgets of the window that was topmost are deleted first, exactly as a
  new window would do to it.
- Raising is serialized with window creation and removal (the same lifecycle mutex), because it moves
  records while widgets are built. It therefore must not be called from an LVGL event callback, where
  the LVGL lock is already held: a concurrent create/remove could be holding the lifecycle mutex while
  waiting for that lock. Callers defer it to the main dispatcher.
- The window stack stays the single source of truth for "which apps are open and in what order". No new
  IPC, no per-app bookkeeping.

### App identity

- The app manager gains a lookup from any app instance to its manifest id, mirroring the existing
  topmost-instance lookup. Cards and the launcher search are built from it; names and icons come from
  the manifest.
- App icons move from two per-launcher-mode tables into one shared lookup, so the switcher shows the
  same icon the launcher does instead of a third table. The launcher gains an icon for itself (used
  only by the switcher, since it stays hidden from the app list).

### Gestures

- Gestures are captured on the pointer devices themselves (press/release/press-lost), not on widgets,
  so they work over any app, including scrollable content.
- Only drags that start in a 20 px edge strip belong to the system; a 40 px travel in the expected
  direction makes it a swipe. Sideways ambiguity resolves to the app.
- The bottom edge wins over the left edge in the corner, so going home always works there.
- The top edge is not handled by this layer: the statusbar already owns swipe-down (and the side-layout
  strip owns the top edge when the statusbar is on the left).
- The left edge is disabled when the system bars use the side layout, because the statusbar strip owns
  that edge there.
- The release is consumed when a gesture fires, so the widget under the finger does not also get a
  click.
- The hold variant uses a timer that is created when a bottom-edge press starts and deleted when it
  ends: no timer exists while the screen is untouched. The timer polls at 50 ms and only for the
  duration of one press.
- A shown overlay (quick panel, recents) disables the system gestures; each overlay has its own
  dismiss gestures.
- The geometry (edge strip choice, swipe direction and distance) is a pure function, unit tested.

### Recents switcher

- It is a window-manager overlay, like the quick panel: dimmed backdrop, cards, dismissed by swiping up
  on the backdrop or tapping outside.
- It lists the window stack topmost first, at most five entries, each with the app's shared icon and
  manifest name.
- Tapping a card raises that window (deferred to the main dispatcher) and hides the switcher. Swiping a
  card up closes that app the same way the toolbar close does (an app close event, not a forced stop),
  then hides the switcher.
- With a keypad or encoder present, a focused close button is added so the overlay can always be
  dismissed.

### Quick panel

- The existing quick panel content is reused and extended; it is not rewritten.
- It becomes a sheet aligned to the top edge and full width, sliding in over 180 ms with the theme's
  animations setting respected. No out-animation (the overlay deletes its widgets).
- It gains: a clock label (and a placeholder before the clock is set), a Wi-Fi status label next to the
  toggles ("off", "on", or the network name), a brightness slider when the display's backlight can be
  dimmed, and a recents tile.
- Brightness is applied immediately while dragging but persisted once, when the panel closes, on the
  main dispatcher, so dragging does not write a settings file per step.
- The panel is not shown when the gesture layer is compiled out.

### Configuration and footprint

- A Kconfig option (`TT_SYSTEM_GESTURES`, default on) guards the gesture layer, the switcher and the
  panel's recents entry. Turning it off compiles the code out.
- At runtime the layer also requires a pointer input device; keypad/encoder-only devices get no
  callbacks at all and behave exactly as before.
- No new dependencies, no STL containers on the gesture path, no heap allocation per gesture: the edge
  scan uses small fixed arrays sized by the number of entries it shows.
- Nothing polls while idle. Brightness UI updates only what changed; the panel animates only its own
  translation.

## Testing Decisions

- Good tests cover externally visible behaviour: which drag belongs to the system and which belongs to
  the app. They do not test LVGL widget internals or event plumbing.
- The gesture geometry is tested as a pure function (edge strip, threshold, diagonal rejection), next to
  the existing system-bars layout tests, using the same doctest suite.
- Raising a window, the switcher and the panel are verified on a real build of the target device; no
  test harness exists for LVGL interaction, and the device is the only place where the window stack,
  timers and input devices are real.
- Verification for this feature is a clean `idf.py build` for `es3c28p` plus `idf.py size`, compared
  against the commit the work started from.

## Out of Scope

- Live thumbnails of the app windows: only the topmost window has widgets, and capturing the others
  would need a snapshot buffer per app (far more RAM than a 240x320 device should spend here).
- An in-app navigation stack: "back" closes the topmost app, exactly like the toolbar close. Apps do
  not gain history.
- Gestures on non-touch devices (mouse, trackball, keypad, encoder) and gesture equivalents for them.
- Making the launcher auto-start or starting any app from the system.
- Backlight support on boards whose devicetree disables it (including `es3c28p` today): the brightness
  row simply does not appear there.
- Enabling backlight hardware, changing any devicetree, or adding a brightness setting of its own.

## Further Notes

- Because apps are resident and the window stack is unbounded, the switcher shows only the five most
  recent windows; older ones remain reachable by closing what is above them, as before.
- The launcher rebuilds its grid from its Context when it is raised, so its page and mode survive a
  round trip through another app.
- The side layout (wide landscape screens) keeps the statusbar strip on the left; the back gesture is
  deliberately absent there rather than competing with the strip.
