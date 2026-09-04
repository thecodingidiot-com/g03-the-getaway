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
- A road file (a finish distance, then a list of obstacles: `x y
  sprite_id`), loaded with `libtci`'s `tci_getline`/`tci_atoi` — this
  is g-tier, not r-tier, so it's `libtci` again, not raw `fopen`.
- Real collision: driving within `COLLISION_DIST` world units of an
  obstacle ends the run. Reaching `finish_dist` world units from the
  start wins it.
- Steering shifts world position directly (`cam->right`, never
  rotated) instead of turning the camera — the same model Hang-On,
  Out Run, and Space Harrier all use, not a raycaster's rotate-then-
  move. Acceleration/braking have their own keys, off the d-pad.
- A visible player craft drawn at a fixed screen position, and
  scattered background decoration reusing the same projection — same
  technique, applied to feel and readability instead of new math.
- No score, no HUD, no audio — outcomes print to the terminal and the
  run restarts. Art and sound are a later part of this curriculum, not
  this chapter's job.

Source is split by concern, one file per module:

| File | Contents |
| --- | --- |
| `main.c` | SDL2 init, the game loop (event → update → render), cleanup |
| `vec2.c` / `vec2.h` | a small 2D vector type: add, subtract, scale, dot (unchanged from r01/r02) |
| `camera.c` / `camera.h` | position, facing angle, and the derived `forward`/`right` axes (unchanged from r01/r02) — no SDL2 anywhere |
| `scaler.c` / `scaler.h` | world position → screen projection (unchanged from r01/r02) — no SDL2 anywhere |
| `road.c` / `road.h` | load the road file, collision and finish checks — no SDL2 anywhere |
| `render.c` / `render.h` | the only file that calls actual SDL2 drawing functions |

`vec2.c`, `camera.c`, `scaler.c`, and `road.c` never call an SDL2
function, so they link into a test binary with no SDL2 library at all.

Build and test your own version first. Use `solution/` to compare
once you are done, not before.

---

## Building the solution

```bash
cd solution
make -C libtci re
bash gen_assets.sh
make re
./getaway ../fixtures/road1.txt
```

Controls: Left/Right arrows or `h`/`l` to steer (a direct sideways
shift, not a turn — the camera always faces forward), Space to
accelerate, Shift to brake, Escape or `q` to quit.

`gen_assets.sh` needs Python3 + Pillow:

```bash
sudo apt install python3-pil
```

---

## What the tester checks

**Build** — the real game compiles and links with zero warnings.

**A standalone logic tester** — `vec2.o`, `camera.o`, `scaler.o`, and
`road.o` compiled and linked with `libtci.a` alone, no SDL2 at all,
asserting real outcomes against `fixtures/road1.txt`:

- The road file parses to the right obstacle count and finish
  distance.
- Standing on an obstacle is a collision; a couple of world units off
  it is not.
- The open road between obstacles never falsely reports a collision.
- Reaching the finish distance wins the run; short of it does not.

**`getaway`** — runs its event loop for two seconds under a headless
(`SDL_VIDEODRIVER=dummy`) video driver without crashing. A smoke test,
not a visual check — actually driving the road, dodging obstacles by
steering, is done by running it yourself.

---

## License

MIT License. See [LICENSE](LICENSE).
