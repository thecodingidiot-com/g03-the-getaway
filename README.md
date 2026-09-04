# g03-the-getaway

Companion repository for **g03 — The Getaway** at
[thecodingidiot.com](https://thecodingidiot.com) — the first Rendering
Journey capstone, unlocked once r02 is done.

---

## Follow my journey

Working through g03 alongside the implementation pages? Build
`getaway` step by step, then run the tester.

Clone this repository — it carries everything needed, including the
`libtci/` subdirectory with all library source:

```bash
git clone https://github.com/thecodingidiot-com/g03-the-getaway.git g03-practice
cd g03-practice/solution
make -C libtci re
bash gen_assets.sh
bash gen_audio.sh
make re
bash ../test.sh
```

All tests must pass before the chapter is complete.

---

## Follow your journey

Building `getaway` independently? Here is the full project brief.

A dodge-and-survive run across open ground, built entirely on r01/r02's
scaler technique — no new rendering math, only what a real game does
with it. No road, no lanes: obstacles scale in against open ground,
the same framing Space Harrier used on the same class of hardware
r01/r02 already built.

- `vec2.c`, `camera.c`, and `scaler.c` ported unchanged from r01 — the
  same `forward`/`right` camera, the same `WINDOW_H / depth` scaler.
- A map file (a finish distance, then a list of obstacles: `x y
  sprite_id`), loaded with `libtci`'s `tci_getline`/`tci_atoi` — this
  is g-tier, not r-tier, so it's `libtci` again, not raw `fopen`.
- Real collision: driving within `COLLISION_DIST` world units of an
  obstacle, at or below `OBSTACLE_HEIGHT`, ends the run. Reaching
  `finish_dist` world units from the start wins it regardless of
  altitude.
- Forward speed is constant, not player-controlled — the real Space
  Harrier (Sega Master System) never gives you a throttle either; the
  whole d-pad is free for pure positioning.
- Steering shifts world position directly (`cam->right`, never
  rotated) instead of turning the camera — the same model Hang-On,
  Out Run, and Space Harrier all use, not a raycaster's rotate-then-
  move. `MIN_SIDE`/`MAX_SIDE` fence the play area, the same way the
  real game does.
- A real altitude axis: climbing above `OBSTACLE_HEIGHT` clears every
  obstacle on the course, the actual dodge Space Harrier's own up/down
  stick is for. A direct positional step, not ramped like throttle.
- A visible player craft drawn at a fixed screen position, banking
  left/right around its own centre as it steers, and scattered
  background decoration reusing the same projection — same technique,
  applied to feel and readability instead of new math.
- A third way through the course: Ctrl or `d` fires a shot that
  destroys the first obstacle it reaches. A destroyed obstacle stops
  colliding and stops rendering; `map_reset()` restores every one of
  them for the next run, the same reason `camera_init()` resets
  position.
- Horizontal ground stripes receding toward the horizon, reusing
  `scaler_project()`'s own size growth applied downward instead of
  upward — a smaller, honest stand-in for the real cabinet's own
  moving-scanline floor, not a new rendering technique.
- Three sound effects — a shot, a hit, and a crash — the only audio
  this chapter adds. No score, no HUD, no music yet; those are still
  a later part of this curriculum.

Source is split by concern, one file per module:

| File | Contents |
| --- | --- |
| `main.c` | SDL2 + SDL2_mixer init, the game loop (event → update → render), cleanup |
| `vec2.c` / `vec2.h` | a small 2D vector type: add, subtract, scale, dot (unchanged from r01/r02) |
| `camera.c` / `camera.h` | position, facing angle, and the derived `forward`/`right` axes (unchanged from r01/r02) — no SDL2 anywhere |
| `scaler.c` / `scaler.h` | world position → screen projection (unchanged from r01/r02) — no SDL2 anywhere |
| `map.c` / `map.h` | load the map file, collision/finish/shot-hit checks — no SDL2 anywhere |
| `shot.c` / `shot.h` | a generic projectile pool — fire, advance, age out — knows nothing about obstacles at all, no SDL2 anywhere |
| `render.c` / `render.h` | the only file that calls actual SDL2 drawing functions |
| `audio.c` / `audio.h` | **new** — the only file that calls actual SDL2_mixer functions |
| `event.h` | **new** — the shared `t_event` enum: `EVENT_FIRED`, `EVENT_HIT`, `EVENT_DIED`, `EVENT_WON` |

`vec2.c`, `camera.c`, `scaler.c`, `map.c`, and `shot.c` never call an
SDL2 (or SDL2_mixer) function, so they link into a test binary with
neither library at all.

Build and test your own version first. Use `solution/` to compare
once you are done, not before.

---

## Building the solution

```bash
cd solution
make -C libtci re
bash gen_assets.sh
bash gen_audio.sh
make re
./getaway ../fixtures/map1.txt
```

Controls: Left/Right arrows or `h`/`l` to steer (a direct sideways
shift, not a turn — the camera always faces forward), Up/Down arrows
or `k`/`j` to climb/descend, Ctrl or `d` to fire (`d` matches the real
emulator's default), Escape or `q` to quit. Forward speed is constant
— there's no accelerate/brake key, matching the real game this chapter
is named after.

`gen_assets.sh` and `gen_audio.sh` need Python3 + Pillow (stdlib
`wave` covers the audio synthesis — no extra package):

```bash
sudo apt install python3-pil libsdl2-mixer-dev
```

All three sound effects are synthesized locally by `gen_audio.sh` —
own-work square/sweep waves, not sourced from anywhere — so there is
nothing to attribute and no license file for `assets/*.wav` beyond
this repository's own MIT license.

---

## What the tester checks

**Build** — the real game compiles and links with zero warnings.

**A standalone logic tester** — `vec2.o`, `camera.o`, `scaler.o`,
`map.o`, and `shot.o` compiled and linked with `libtci.a` alone, no
SDL2 or SDL2_mixer at all, asserting real outcomes against
`fixtures/map1.txt`:

- The map file parses to the right obstacle count and finish
  distance.
- Standing on an obstacle is a collision; a couple of world units off
  it is not.
- The open ground between obstacles never falsely reports a collision.
- Flying at or above `OBSTACLE_HEIGHT` clears an obstacle that would
  otherwise be a certain collision; just under that height still
  isn't enough.
- A shot that reaches an obstacle destroys it, deactivates itself, and
  counts as exactly one hit; a destroyed obstacle stops colliding;
  `map_reset()` restores it.
- A shot fired from above `OBSTACLE_HEIGHT` flies over an obstacle,
  same as the player would, and scores no hit.
- Reaching the finish distance wins the run; short of it does not.

**`getaway`** — runs its event loop for two seconds under a headless
(`SDL_VIDEODRIVER=dummy`, `SDL_AUDIODRIVER=dummy`) driver without
crashing. A smoke test, not a visual or audible check — actually
running the course, dodging obstacles by steering, altitude, or
shooting them down, is done by running it
yourself.

---

## License

MIT License. See [LICENSE](LICENSE).
