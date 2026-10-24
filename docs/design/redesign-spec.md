# Tactility mobile — redesign spec

Status: design complete (mockups + spec, no firmware changes)
Target: ESP32-S3 ES3C28P, 2.8" TFT, 240 × 320 portrait, ~143 ppi, RGB565, capacitive touch, LVGL
Source context: `docs/mobile/ios-gestures-spec.md` (window stack, quick panel, recents)
Review: independent design critique + `impeccable` audit/critique pass, `frontend-design` direction, `web-design-guidelines` accessibility checklist
Mockups: `docs/design/mockups/` · renders: `docs/design/png/`

This spec describes a visual redesign of the nine front-end surfaces. It deliberately does **not**
change firmware. Where the redesign requires a firmware change (recents stack, dock, Settings app,
wake screen, icons, fonts, keyboard height), the delta is listed in §9.

## 1. Intent

A dark, instrument-like handheld UI: quiet surfaces, one blue accent, crisp type, and a single
memorable gesture target — the 48 px clock with its accent colon. The UI is dense enough for a 240 px
screen but never decorative: every colour, line and pixel earns its place. Inter replaces Montserrat;
the type scale is fixed for bitmap rendering. The accent appears only for state and action, never as
decoration.

Physical scene: a small desk/handheld device, dim room, glanced at and touched with a thumb. Dark is a
functional choice (glare-free, low-power, high contrast at 143 ppi), not a stylistic default.

## 2. Hard constraints (global)

| Constraint | Value |
|---|---|
| Canvas | 240 × 320, portrait. 1× = 240 × 320; 2× = 480 × 640. |
| Font family | **Inter only** (Regular 400, Medium 500). No second family. |
| Text rendering | LVGL bitmap fonts, **4 bpp**; clock is a **48 px subset of `0123456789:`** only. |
| Icon set | One colour, A2 2 bpp alpha masks, recoloured at draw time. Stroke **2 px at 24 px**, **2.25 px at 32 px**; 1.8 px at 18 px; 1.5 px at 16 px. |
| Backgrounds | Solid `#0E1116`, or one 2-stop vertical gradient (lock only). No photos, no textures, no blur. |
| Alpha | One solid **60% black** dim layer for quick panel/dialog. Recents is **opaque**. No other runtime alpha. |
| Accent | Exactly one: `#4C8DFF`. Tint `#1A2B45`/`#9CC0FF` are precomputed solids, not runtime alpha. |
| Radii | **12 px** cards (tiles, list cards, recents cards, dock, dialog, sheet bottom); **8 px** controls (buttons, fields, keys, quick tiles); switch pill 10 px; slider track 3 px, knob round. |
| Motion | Transitions only, **≤ 200 ms**; translate only. No idle animations, spinners, pulses or blinking carets. Stop when animations are disabled. |
| Chrome | Status bar 24 px; toolbar 44 px; content starts 80 px; list rows 44 px. |
| Touch | 44 × 44 ideal; **40 × 40 minimum**; never below 32. Exceptions: keyboard keys are 40 px high but 21–24 px wide (11-key QWERTY on 240 px); recents close X is 40 × 44. |
| Gesture rails | 20 px bottom strip and 20 px left strip belong to the system on normal screens. Taps still work; drags do not. Overlays disable system gestures. |
| Contrast | Secondary text ≥ **#8A94A3** on `#0E1116` (≥ 4.5:1). On cards `#1A1F27`, nothing darker than `#8A94A3` (5.39:1). |
| Focus / selection | Accent is the only selection/active colour. No semantic red; destructive copy is explicit. |
| Shadows | None. Structure comes from 1 px lines, surface steps and spacing. |
| Image RAM | None: all fonts/icons are const flash; no decoded buffers, no thumbnails. |

## 3. Tokens

### 3.1 Palette

| Token | Hex | Use | Contrast on bg |
|---|---|---|---|
| `bg` | `#0E1116` | screen base | — |
| `surface` | `#1A1F27` | cards, tiles, dock, sheet | — |
| `surface-2` | `#242A33` | tracks, secondary buttons, quick tiles | — |
| `line` / `line-soft` | `#242A33` / `#1F2530` | borders and 1 px dividers | — |
| `text` | `#E8ECF1` | primary text, icons | **15.9:1** |
| `muted` | `#9AA4B2` | secondary text, labels, icons | **7.5:1** |
| `faint` | `#8A94A3` | weak values, chevrons, locks (minimum) | **6.16:1** |
| `accent` | `#4C8DFF` | active state, actions, switch on, slider fill | **5.91:1** |
| `accent-text` | `#9CC0FF` | small accent text on accent-soft | **7.72:1** |
| `accent-soft` | `#1A2B45` | selected Wi-Fi card / active tile | — |
| `accent-line` | `#2B4C7E` | selected card border | — |
| `on-accent` | `#0B1118` | text/icon on accent fills | **5.92:1** |

The lock gradient is `#0E1116 → #141C29` (2 stops, vertical). All other screens use solid `bg`.
Card tint values are precomputed solids; no runtime alpha is used outside the one 60% dim layer.

### 3.2 Type scale (Inter)

| Role | Size / line / weight | Use |
|---|---|---|
| Clock | **48 / 52 / 500** | lock/home clock, digits + colon only |
| Title | **22 / 26 / 500** | Settings, Wi-Fi, Files, Rowan, Forget network? |
| Recents title | **20 / 24 / 500** | mandated overlay exception |
| Body | **15 / 20 / 500** | rows, buttons, dialog body, recents names |
| Dense interactive | **14 / 16 / 500** | keyboard keys, quick-panel tile labels |
| Secondary | **13 / 15–16 / 500** | dates, values, status time, section labels, paths, app-grid labels |
| Caption | **12 / 14 / 400** | TACTILITY, recents hint, lock hint, list footer |

Primary scale: **13 / 15 / 22 / 48**. The mandated additions are **12 (captions)**, **14 (keys/tiles)**
and **20 (recents title)**. 500 is used for everything except 12 px captions; 400 is reserved for
captions.

Letter-spacing: clock `-1.5 px`; titles `-0.01em`; app-grid labels `-0.02em` (to fit 52 px); captions
`0.04–0.18em`; everything else default. No text is horizontally scaled; no dynamic text scaling.

App-grid labels are **one line, 52 px max, ellipsised**; the tile is the 44 px control, the label is a
13 px caption. "App Hub" is labelled **Hub**; "Settings" and "Grok Bot" keep their full names and a
visible gap (5–6 px) at 240 px.

## 4. Layout and chrome

- **Status bar** 24 px: time 13/500 at x=16; Wi-Fi 24 px at right 14; no battery (board has none).
- **Toolbar** y=24, 44 px: back/close 24 px icon inside a **44 × 44** hit; title 22/500 at x=52; one
  44 × 44 action max; 1 px `line-soft` divider at y=68.
- **Content** starts at 80 px; cards use 16 px side margins; rows 44 px; dividers inset 46 px.
- **Rows**: icon 24, label 15/500, value 13/500, chevron 16, 12 px padding.
- **Bottom rail**: 20 px free of interactive items on normal screens. Home dock items end 24 px above
  the bottom; the dock surface ends 14 px above. The recents overlay (gestures disabled) and the
  keyboard bottom row are the documented exceptions.
- **Left rail**: 20 px; the toolbar back hit starts at x=0, so drags from the left are the system back
  gesture while taps still hit the button.
- **Recents header**: its own 40 px band at y=28; the opaque screen covers the status bar.
- **States**: pressed tiles/rows step surface → surface-2; accent buttons → `#3F7BE0`; no animation.
  Focus (keypad/encoder devices only) is a 2 px accent ring at 2 px offset, radius +2; touch screens
  show no focus ring. Disabled controls keep `#8A94A3` text with no accent. Empty states read
  "No networks", "Empty folder", "No active apps" at 15/500 muted. Loading (Wi-Fi scan) is a static
  "Scanning…" 15/500, never a spinner. Errors use the accent outline plus explicit copy; there is no
  red. Sliders accept a tap on the 40 px row/track as well as a drag; the lock accepts tap or swipe;
  the quick panel dismisses by tap outside or swipe up; recents closes by X or swipe; back has the
  toolbar button as well as the left-edge gesture.

## 5. Components

- **App tile**: 44 × 44, 12 radius, surface; icon 32; label 13/500, max 52, one line. Four columns of
  48 px with 8 px gaps, 12 px side margin; 2 rows.
- **Dock (part of home)**: 216 × 56, 12 radius, 14 px above bottom; four 44 × 44 items, 32 px icons,
  4 px `#4C8DFF` running dot. It is a component of the launcher window — **not a separate app/window**.
- **List card**: 12 radius; rows 44; 1 px inset dividers; no shadow.
- **Button**: 40 or 44 high, 8 radius; primary accent + `on-accent`; secondary surface-2; one accent.
- **Switch**: visual 36 × 20 pill, 16 knob; hit 40 × 44; on = accent.
- **Slider**: track 6 (radius 3), thumb 20, touch row 40; fill accent; tap the track to set, drag to
  adjust; no numeric value text.
- **Quick tile**: 68 × 68, 8 radius, surface-2; active surface `accent-soft` + accent icon; label
  14/500; sub-label 13/500.
- **Dialog**: 208 wide, 12 radius, 16 padding; title 22; body 15/20; buttons 40; 60% dim.
- **Keyboard**: field 40; eye hit 40; 4 rows of 40 px keys, 1 px gaps, 8 radius, letters 14;
  space/Go row; Go accent.
- **Recents card**: 44 high, 12 radius, 32 icon, 15 name; close X 40 × 44 hit with 18 icon; whole card
  opens the app; current card 3 px accent bar.
- **Sheet**: quick panel 246 high, 12 px side padding, bottom radius 12; opaque surface.

### 5.1 Interaction states

- **Pressed**: neutral controls change to `surface-2`; accent controls to `#3A6FC4`; instant, no scale.
- **Focus** (keypad/encoder variants): 2 px accent ring, 2 px offset, radius follows the control; never
  colour-only. The touch-only mockups show the resting state; the ring is a firmware state.
- **Disabled**: text/icon `#8A94A3` on `surface`; no accent; no alpha.
- **Empty**: Wi-Fi `No networks` 15/500 muted; Files `Empty folder`; recents is not opened when the
  window stack is empty.
- **Loading**: `Scanning…` 13/500 muted, static; no spinner, no idle animation.
- **Error**: no red; accent outline plus explicit copy (for example `Wrong password. Try again.`).
- **Alternatives to gestures**: lock taps or swipes; sliders tap or drag; recents X or swipe up; quick panel
  tap outside or swipe; app close via toolbar back or left-edge swipe.

## 6. Screens

### 01 · Lock

Purpose: wake/boot glance screen. There is **no PIN or security**; tap anywhere or swipe up is the wake gesture.
Layout: 240 × 320; 2-stop vertical gradient; no status bar. `TACTILITY` 12/400 caption at top 20;
Wi-Fi 24 at top 14 right 16; clock 48/52 at y=90; date 13/15 at y=152; 36 × 4 accent bar + `Swipe up or tap`
12/400 at bottom 44. Unlock by swiping up or tapping anywhere; 160 ms slide-up, returning to the
previous window (home if none).
Constraints: no alpha, no status bar, no idle animation, one gradient only; the whole screen is the
unlock target, so the gesture has a tap alternative.

### 02 · Home

Purpose: launcher. Status bar 24 (time 13, Wi-Fi 24). Clock 48/52 at y=30; date 13/15; app grid at
y=106, 4 × 48 columns, 8 gaps, 12 side margin; 8 tiles (Settings, Grok Bot, Wi-Fi, Files, Hub, Notes,
Terminal, Camera). Labels 13/500, 52 max, one line. Dock at y=250, 216 × 56, 14 above bottom; 4 apps
(Grok Bot, Notes, Files, Camera), 32 icons, 4 px running dot. Grid ends 238; dock gap 12. Status bar
swipe-down opens the quick panel.
Constraints: solid bg; no alpha; dock is part of home; 20 px bottom rail free of items; no animation;
labels are captions (13) while the 44 px tile is the control; `Hub` replaces `App Hub`; Settings and
Grok Bot must not touch.

### 03 · Quick panel

Purpose: top sheet for time, brightness, volume, Wi-Fi/BT and recents. Full width, 246 high, 12 side
padding, bottom radius 12; 60% black dim over home. Handle; time 22/26 + date 13 at y=22. Two 40 px
slider rows (y=58 and 108): icon 24, track 6, thumb 20, fill accent. Three 68 × 68 tiles at y=160,
6 px gap, 12 margins: Wi-Fi (HomeNet, active), Bluetooth (Off), Recents (3 apps). Labels 14, subs 13.
Transition: 160 ms slide-in from top; instant out. Dismiss swipe-up/tap outside. Brightness row is
**conditional**: on ES3C28P the devicetree backlight is disabled, so the row is omitted and the sheet
is 200 high; the mockup shows the dimmable variant.
Constraints: the only alpha is the 60% dim (one full-screen layer, no blur); sheet opaque; no spinner;
no out-animation; recents tile opens the switcher.

### 04 · Recents

Purpose: window switcher, max 5, topmost first. **Opaque `#0E1116` full screen**, including the status
bar area; the app behind never shows. Header 40 px at y=28: title 20/24 at y=28; hint `Swipe up to
close` 12/400 right-aligned on the same baseline; 1 px line at y=68. List at y=76 (8 px gap): five
208 × 44 cards, 4 px gaps, 16 side margins; cards end at y=312 (8 px bottom). Card: 32 icon, 15 name,
close X 40 × 44 hit with 18 icon; current card 3 px accent bar. Whole card opens; X closes; swipe up on
a card closes; swipe up on the screen dismisses. No thumbnails (only the topmost window has widgets).
Transition: 140 ms slide-in; instant out.
Constraints: opaque, no dim, no status bar, no base app; header 40; title 20/24; hint 12; list 76;
8 px breathing; X 40 × 44; no scroll; the bottom 8 px is card (overlay gestures are disabled).

> Design delta: this replaces the shipped horizontal 66%-wide carousel with a vertical stack so all
> five cards are visible at 240 px. The close X is an addition; swipe-up still closes.

### 05 · Settings

Purpose: root settings list. Status bar + toolbar (back 44, title `Settings` 22). One card at y=80:
five 44 px rows, ends 300 (20 px bottom rail). Display (Dark), Sound (60%), Wi-Fi (HomeNet), Bluetooth
(Off), Power (5 min). Icons 24; labels 15; values 13; chevrons 16; 1 px inset dividers. No section
labels on the root; sub-screens scroll. Back/swipe-right closes. No animation.
Constraints: single card; 44 rows; card ends 300; 20 px rail; no alpha.

### 06 · Wi-Fi

Purpose: network list. Status bar + toolbar: back 44, title `Wi-Fi` 22, refresh 44, switch 40 × 44 hit
(36 × 20 visual). Connected card at y=78, 208 × 60, accent-soft + accent-line; icon 24 accent; name 15;
state `Connected` 13 `#9CC0FF`; chevron 16. `AVAILABLE` 13 at y=150. List at y=164: three 44 px rows,
ends 296 (24 px bottom). Signal 16 (1/2/3 variants), name 15, lock 16, chevron 16. Scan on entry and
refresh. Tapping a row opens the password/keyboard or detail. No spinner animation.
Constraints: 3 rows visible; 44 rows; switch hit 40 × 44; refresh 44; no alpha; bottom 24.

### 07 · App (Files)

Purpose: generic app chrome. Status bar + toolbar: back 44, title `Files` 22, more 44. Path `Internal
storage` 13 at y=78; list at y=96 with four 44 px rows (Documents, Music, Pictures, notes.txt), ends
272; footer `4 items · 12.4 GB free` 12/400 at y=284. Rows: icon 24, label 15, value 13, chevron 16.
No animation; list scrolls under the fixed toolbar.
Constraints: 4 rows visible; 44; card 12; footer caption 12; bottom 20 rail.

### 08 · Keyboard

Purpose: text entry (Wi-Fi password shown). Status bar + toolbar: back 44, title `Rowan` 22, `Join`
15 in a 44 hit. `Password` 13 at y=74; field 40 at y=88 with dots 15 and eye 18 in a 40 × 40 hit;
`WPA2 Personal` 13 at y=133. Keyboard at y=149, 171 high: 4 rows of 40 px keys, 1 px gaps, 4 px
padding, 8 radius, letters 14. Rows: QWERTY 10, ASDF 9, ZXCV 7 + shift/backspace, `123`/space/`Go`.
Go accent. Instant show; no key pop-ups, no suggestions, no idle animation.
Constraints: keys 40 high, 1 gap, letters 14; key width 21–24 is the **only sub-32 touch exception**;
bottom row sits in the 20 px rail (taps work, drags go home); field 40; eye 40; keyboard 171.

### 09 · Dialog

Purpose: confirm destructive action (Forget network). 60% dim over Wi-Fi. Card 208 wide at x=16, y=69,
16 padding, 12 radius. Title 22/26; body 15/20 (3 lines); two 40 px buttons, 8 gap: Cancel (surface-2)
and Forget (accent/on-accent). Card ends 251. Transition: 140 ms slide-up; instant out. No blur, no
red; the destructive word is explicit.
Constraints: dim 60; title 22; body 15; buttons 40; one accent; no idle.

## 7. Motion

| Transition | Duration | Notes |
|---|---|---|
| Lock unlock | 160 ms | translate up; previous window retained |
| Quick panel in | 160 ms | translate from top; out instant (overlay deletes widgets) |
| Recents in | 140 ms | translate; out instant |
| Dialog in | 140 ms | 8 px translate; out instant |
| Keyboard show | 0 ms | instant; typing must not wait |
| App/window switch | 0 ms | immediate |
| Idle | none | no pulses, spinners, blinking carets |

Respect the theme's animations setting; when off, all transitions are instant. No full-screen alpha
animation; no text scaling/rotation.

## 8. Flash / RAM budget

### 8.1 Fonts (Inter, 4 bpp, compiled; clock 4 bpp subset)

| Asset | Size / weight | Subset | Glyphs | Est. flash |
|---|---|---|---|---|
| Caption | 12 / 400 | ASCII | 95 | 6.0 KB |
| Secondary | 13 / 500 | ASCII | 95 | 6.7 KB |
| Dense / keyboard | 14 / 500 | ASCII | 95 | 7.3 KB |
| Body | 15 / 500 | Latin-1 | 216 | 18.0 KB |
| Recents title | 20 / 500 | `Recents` | 7 | 1.6 KB |
| Title | 22 / 500 | ASCII | 95 | 14.4 KB |
| Clock | 48 / 500 | `0123456789:` | 11 | 7.2 KB |
| **Total** | | | **728** | **61.2 KB** |

Estimates use the measured LVGL 4 bpp glyph density (157-glyph Montserrat sample) and include glyph
descriptors/cmaps. The 20 px subset is only `Recents`; if that title becomes dynamic, use the 22 px
subset instead (+13 KB). The 13/14/22 subsets are ASCII; extend 13/22 to Latin-1 (+8/+12 KB) if
localized app names or SSIDs appear in those roles — the 15 px body already covers user content.
Optional flash saving: render 12 captions with the 13 subset and 14 labels with the 15 subset
(−13.3 KB), at the cost of the exact mandated sizes.

### 8.2 Icons (A2 2 bpp, one colour, recoloured at draw time)

| Size | Count | Icons | Bytes each | Total |
|---|---|---|---|---|
| 32 | 9 | settings, bot, wifi, folder, package, note, terminal, camera, home | 264 | 2,376 |
| 24 | 13 | sun, volume, bluetooth, recents, back, more, refresh, wifi, folder, display, power, shift, backspace | 152 | 1,976 |
| 18 | 2 | close, eye | 89 | 178 |
| 16 | 5 | chevron, lock, signal-1, signal-2, signal-3 | 72 | 360 |
| Descriptors | 29 | `lv_image_dsc_t` | 32 | 928 |
| **Total** | **29** | | | **5.8 KB** |

The 3 signal variants use 30% alpha for unfilled bars; still one colour. No per-state images: active,
muted and accent states are recolours of the same A2 mask. Strokes: 2 px at 24, 2.25 px at 32, 1.8 px
at 18, 1.5 px at 16.

### 8.3 Other assets and RAM

- Backgrounds: **0 bytes**; solid fill or a 2-stop gradient computed at draw (lock only).
- Dim layer: **0 bytes**; one 240 × 320 60% black layer, 76,800 px composited per paint, only while
  quick panel/dialog is open. Recents uses no layer (opaque screen).
- Images: **none** (no photos, no thumbnails, no decoded buffers).
- Static descriptors: 29 × 32 B ≈ **0.9 KB**.
- Screen heap (LVGL objects, estimates; one screen + overlay at a time): lock 1.5 KB; home 7 KB;
  quick panel 11 KB; recents 8 KB; settings 6 KB; Wi-Fi 6 KB; Files 6 KB; keyboard **20 KB peak**;
  dialog 7 KB.
- **Total new flash ≈ 61.2 + 5.8 = 67.0 KB.** Net ≈ **+37 KB** after removing the built-in
  Montserrat 14 (≈11.3 KB) and the AdwaitaSans TTF (≈18.9 KB); the Material Symbols TTF (20.9 KB)
  stays for apps that still use glyph icons. That is <2% of the 3.88 MB firmware image. Fonts/icons
  are const flash: **0 bytes of decoded RAM**, no runtime font generation.

## 9. Implementation deltas (for the next spec/implementation pass)

- **Inter bitmap subsets** replace the runtime-generated AdwaitaSans text fonts; the 48 px clock is
  a digits+colon subset. Text is 4 bpp.
- **New A2 icon set** replaces the shell's Material Symbols glyphs; app manifests need an icon id and
  the system app mapping changes. The Material Symbols font can remain for third-party apps.
- **Dock is part of home** (not a window/app); running dots read the window stack.
- **Recents**: opaque full-screen overlay, 40 px header, vertical 5-card stack, 40 × 44 close X,
  whole card opens; replaces the shipped horizontal carousel and the dimmed backdrop. No thumbnails.
- **Settings** is promoted to an app with a toolbar/back (the shipped settings mode lives inside the
  launcher); the home grid can launch it.
- **Lock/wake screen** is new: no PIN, shown on boot/wake, swipe up returns to the previous window.
- **Quick panel**: 246 high (200 without dimmable brightness), 68 px tiles, 6 px gaps, 40 px slider
  rows; brightness row hidden on ES3C28P today (backlight disabled).
- **Keyboard**: 40 px keys, 1 px gaps, 171 px high, 149..320; skinned `lv_keyboard`, no new widget.
- **Toolbars**: 44 × 44 hit targets, 22 px titles, 24 px statusbar Wi-Fi, 13 px status time.
- **Theme/radii**: primary `#4C8DFF`, bg `#0E1116`, 12/8 radii, dark surfaces; the current Material
  theme defaults must be overridden.
- No firmware code is included in this change.

## 10. Verification and files

Renders verified: nine PNGs at **240 × 320 (1×)** and **480 × 640 (2×)**, contact sheet at **1×**
(900 × 1310 canvas, 3 × 3 screens), no overflow/clipping, all text legible at 1×, contrast checked,
touch targets checked, one font family, one accent, no photos/blur.

```
docs/design/
├── redesign-spec.md
├── mockups/
│   ├── index.html            # 9 screens, 240 × 320 each
│   ├── styles.css            # tokens + screen CSS
│   ├── contact-sheet.html    # 1× contact sheet source
│   └── fonts/                # Inter 400/500 woff2 (sources only; not firmware assets)
└── png/
    ├── 1x/*.png              # 240 × 320
    ├── 2x/*.png              # 480 × 640
    └── contact-sheet.png     # 900 × 1310, 1×
```

Reproduce with Playwright Chromium headless from a temp directory (never in the firmware build):
`index.html` with `deviceScaleFactor` 1 and 2, screenshot each `.phone`; then `contact-sheet.html` at
1× with `fullPage`. The fonts are local woff2; no network at render time.

### Review notes (impeccable audit/critique + independent critique)

Resolved in this pass: recents ghosting/statusbar bleed → opaque screen, 40 px header, title 20/24 at
y=28, hint 12 on the same baseline, list at 76 with 8 px breathing; touch targets → 40–44 px
throughout, X 40 × 44, sliders 40/6/20, keyboard 40 px keys/1 px gap/14 px, dialog 40 px buttons,
4 px dock dot; typography → Inter 400/500, 13/15/22/48 primary scale with mandated 12/14/20, grid
labels one line/52 max (full names; `Hub`); contrast → `#8A94A3` minimum, `#9CC0FF` on accent-soft;
icons → 2/2.25 px strokes, one colour, A2; home date 13/15, grid up 8 px, dock breathing 12 px; lock
`TACTILITY` as a 12 px caption; quick-panel tiles 68/6; Wi-Fi refresh + switch hit areas; keyboard
field/eye targets; dialog body 15.

Audit: the impeccable detector found only mandated single-family/Inter notices and contact-sheet
document patterns (flat doc hierarchy, numbered index); no contrast, touch-target or overflow findings.
The accessibility checklist (contrast, touch, alt text, focus/reduced-motion, tap alternatives) is
satisfied or explicitly documented below. The mockup HTML is a static design board, not the shipped
LVGL DOM; ARIA and web form rules do not map to the firmware.

Audit result: the `impeccable` detector returned only font-family warnings (Inter/single family),
which the brief mandates, plus contact-sheet document warnings (the 12/13/22 page scale and the
01–09 index); it found no contrast, touch or overflow defects. The visual critique found no remaining
P0/P1 after the mandatory corrections. The web checklist added a tap alternative for the lock, focus/
pressed/disabled/empty/loading states, and contact-sheet image dimensions/alt text.

Residual risks / deliberate exceptions: keyboard key width 21–24 px (10/9-key QWERTY at 240 px; the
mandate fixes height at 40, not width); app-grid labels are 13 px captions (the 44 px tile is the
control) so the full names fit 52 px; the 60% dim is one full-screen composite (no blur; keep it
static); brightness is absent on ES3C28P; recents is opaque rather than a dim; Settings, lock and
vertical recents are new behaviours for the implementation pass, not shipped firmware.

Skills used: `impeccable` (critique / audit / polish / typeset / layout; product register) and
`frontend-design` (subject-grounded direction, one-family type, restraint), with
`web-design-guidelines` only as an accessibility checklist. `liquid-glass-design` ignored.
