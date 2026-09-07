#!/bin/bash
# g03 — The Getaway / test.sh
#
# Builds the game, then checks the map/collision/finish/shot logic
# deterministically -- compiled and linked WITHOUT SDL2 at all (vec2.c,
# camera.c, scaler.c, map.c, and shot.c never call an actual SDL
# function, so the logic layer needs no display and no SDL2 library at
# link time).
#
# Copy this file and fixtures/map1.txt into your working directory,
# build with 'make re', then run:
#
#   bash test.sh

set -o pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FIXTURES="${SCRIPT_DIR}/fixtures"

# ── colour ────────────────────────────────────────────────────────────────────

if [[ ! -t 1 ]]; then
    C_GREEN=""
    C_RED=""
    C_BOLD=""
    C_RESET=""
else
    C_GREEN="\033[0;32m"
    C_RED="\033[0;31m"
    C_BOLD="\033[1m"
    C_RESET="\033[0m"
fi

pass_count=0
fail_count=0
WORK_DIR=$(mktemp -d)

cleanup() {
    rm -rf "$WORK_DIR"
}
trap cleanup EXIT

hr() {
    echo "────────────────────────────────────────────────────────────────"
}

banner() {
    hr
    echo "  g03 — The Getaway / test.sh"
    hr
}

pass() {
    local label="$1"
    printf "  ${C_GREEN}PASS${C_RESET}  %s\n" "$label"
    pass_count=$((pass_count + 1))
}

fail() {
    local label="$1"
    local detail="${2:-}"
    printf "  ${C_RED}FAIL${C_RESET}  %s\n" "$label"
    if [[ -n "$detail" ]]; then
        echo "        $detail"
    fi
    fail_count=$((fail_count + 1))
}

banner

# ── build the real game ───────────────────────────────────────────────────────

echo "Building..."
build_log=$(make re 2>&1)
build_status=$?
if [[ "$build_status" -ne 0 ]]; then
    fail "build succeeds" "make re failed:"
    echo "$build_log"
    exit 1
fi
pass "build succeeds"

if echo "$build_log" | grep -qi "warning"; then
    fail "build produces no warnings" "$(echo "$build_log" | grep -i warning)"
else
    pass "build produces no warnings"
fi

if [[ -x ./getaway ]]; then
    pass "getaway binary exists"
else
    fail "getaway binary exists"
fi

# ── build the SDL2-free logic tester ─────────────────────────────────────────

if [[ ! -f "${FIXTURES}/map1.txt" ]]; then
    fail "fixtures/map1.txt found" "keep the g03-the-getaway clone alongside your working directory"
    exit 1
fi
cp "${FIXTURES}/map1.txt" "$WORK_DIR/map1.txt"

cat > "$WORK_DIR/test_logic.c" <<'TESTC'
#include <stdio.h>
#include "libtci.h"
#include "vec2.h"
#include "camera.h"
#include "shot.h"
#include "map.h"
#include "scaler.h"
#include "ground.h"

static int  g_pass = 0;
static int  g_fail = 0;

static void check_int(char const *label, int got, int want)
{
    if (got == want)
    {
        tci_printf("PASS  %s (got %d)\n", label, got);
        g_pass++;
    }
    else
    {
        tci_printf("FAIL  %s (got %d, want %d)\n", label, got, want);
        g_fail++;
    }
}

int main(void)
{
    t_map       map;
    t_camera    cam;

    if (!map_load(&map, "map1.txt"))
    {
        tci_printf("FAIL  map_load\n");
        return (1);
    }
    check_int("map1.txt has twenty obstacles", map.count, 20);
    check_int("map1.txt finish_dist", (int)map.finish_dist, 740);

    /* driving straight at the first obstacle (50, 0) with no steering
     * runs right into it -- the case that forces the first dodge. */
    camera_init(&cam, 50.0f, 0.0f, 0.0f);
    check_int("standing on an obstacle at ground height is a collision",
        map_check_collision(&map, &cam, 0.0f), EVENT_DIED);

    /* a little short of COLLISION_DIST away is still a miss -- the
     * radius is a real boundary, not a generous fudge factor. */
    camera_init(&cam, 50.0f, 2.0f, 0.0f);
    check_int("2.0 world units off an obstacle is not a collision",
        map_check_collision(&map, &cam, 0.0f), EVENT_NONE);

    /* the open map between obstacles is always clear. */
    camera_init(&cam, 67.0f, 0.0f, 0.0f);
    check_int("mid-map between obstacles is not a collision",
        map_check_collision(&map, &cam, 0.0f), EVENT_NONE);

    /* flying at or above OBSTACLE_HEIGHT clears the same obstacle
     * that would otherwise be a certain crash -- the actual dodge
     * Space Harrier's own up/down axis is for. */
    camera_init(&cam, 50.0f, 0.0f, 0.0f);
    check_int("flying above OBSTACLE_HEIGHT clears an obstacle",
        map_check_collision(&map, &cam, OBSTACLE_HEIGHT), EVENT_NONE);

    /* just under the threshold is still a real crash -- climbing
     * has to actually clear the obstacle, not just approach it. */
    camera_init(&cam, 50.0f, 0.0f, 0.0f);
    check_int("just under OBSTACLE_HEIGHT is still a collision",
        map_check_collision(&map, &cam, OBSTACLE_HEIGHT - 0.1f),
        EVENT_DIED);

    /* firing a shot and stepping it forward destroys the obstacle it
     * reaches -- the same distance check collision already uses,
     * against a moving point instead of the camera. */
    {
        t_shot  shots[MAX_SHOTS];
        int     j;
        int     total_hits;

        j = 0;
        while (j < MAX_SHOTS)
        {
            shots[j].active = 0;
            j++;
        }
        shot_fire(shots, (t_vec2){0.0f, 0.0f}, 0.0f);
        j = 0;
        total_hits = 0;
        while (j < 30)
        {
            shot_update(shots);
            total_hits += map_check_shots(&map, shots);
            j++;
        }
        check_int("a shot that reaches an obstacle destroys it",
            map.obstacles[0].destroyed, 1);
        check_int("the shot that hit is no longer active",
            shots[0].active, 0);
        check_int("map_check_shots() returns exactly one hit",
            total_hits, 1);
        check_int("a hit starts its obstacle's flash timer",
            map.obstacles[0].flash_timer, FLASH_DURATION);
        j = 0;
        while (j < FLASH_DURATION)
        {
            map_tick_flashes(&map);
            j++;
        }
        check_int("map_tick_flashes() counts the flash timer down to zero",
            map.obstacles[0].flash_timer, 0);
    }

    /* a destroyed obstacle stops colliding with the camera too --
     * shooting one down is a real alternative to steering or flying
     * over it, not just a visual effect. */
    camera_init(&cam, 50.0f, 0.0f, 0.0f);
    check_int("a destroyed obstacle no longer collides",
        map_check_collision(&map, &cam, 0.0f), EVENT_NONE);

    /* map_reset() puts every obstacle back for the next run -- the
     * same reason camera_init() puts the camera back at the start. */
    map_reset(&map);
    check_int("map_reset() restores a destroyed obstacle",
        map.obstacles[0].destroyed, 0);

    /* a shot fired from above OBSTACLE_HEIGHT flies over the same
     * obstacle it would otherwise destroy -- shots respect the same
     * height gate collision does. */
    {
        t_shot  shots[MAX_SHOTS];
        int     j;
        int     total_hits;

        j = 0;
        while (j < MAX_SHOTS)
        {
            shots[j].active = 0;
            j++;
        }
        shot_fire(shots, (t_vec2){0.0f, 0.0f}, OBSTACLE_HEIGHT);
        j = 0;
        total_hits = 0;
        while (j < 30)
        {
            shot_update(shots);
            total_hits += map_check_shots(&map, shots);
            j++;
        }
        check_int("a shot fired above OBSTACLE_HEIGHT flies over an obstacle",
            map.obstacles[0].destroyed, 0);
        check_int("a shot fired above OBSTACLE_HEIGHT scores no hit",
            total_hits, 0);
    }

    /* short of the finish line, still racing. */
    camera_init(&cam, 720.0f, 0.0f, 0.0f);
    check_int("short of the finish line is not a win",
        map_check_finish(&map, &cam), EVENT_NONE);

    /* at or past the finish line, the run is won. */
    camera_init(&cam, 740.0f, 0.0f, 0.0f);
    check_int("at the finish line is a win",
        map_check_finish(&map, &cam), EVENT_WON);
    camera_init(&cam, 800.0f, 0.0f, 0.0f);
    check_int("past the finish line is still a win",
        map_check_finish(&map, &cam), EVENT_WON);


    /* ── where is the ground? ─────────────────────────────────────────────
    **
    ** Two places in this program answer that, and they do not agree.
    **
    ** render_ground_stripes() puts a stripe at depth d at
    **     HORIZON_Y + size * STRIPE_Y_SCALE
    ** so the ground descends from the horizon as things get closer.
    **
    ** scaler_project() puts a billboard's TOP at HORIZON_Y - size, which
    ** puts its BOTTOM at HORIZON_Y exactly, whatever the depth. Objects
    ** are nailed to the horizon line while the ground slides down past
    ** them -- which is why every obstacle looks like it stands at the
    ** same height as every other one.
    **
    ** The stripe side now comes from ground.h, which is SDL2-free on
    ** purpose: the numbers deciding where the ground is were previously
    ** unreachable from the only harness that runs without a display,
    ** which is how the two answers drifted apart unnoticed. */

    camera_init(&cam, 0.0f, 0.0f, 0.0f);
    {
        t_vec2          probe;
        t_projection    proj;
        int             stripe_row;
        int             billboard_base;
        int             d;

        d = 6;
        while (d <= 24) {
            probe.x = (float)d;
            probe.y = 0.0f;
            proj = scaler_project(&cam, probe);
            stripe_row = ground_row(&proj);
            billboard_base = proj.screen_y + proj.size;
            check_int("ground agrees with itself at depth 6/12/18/24",
                billboard_base, stripe_row);
            d += 6;
        }
    }

    tci_printf("\n%d passed, %d failed\n", g_pass, g_fail);
    return (g_fail > 0);
}
TESTC

logic_build_log=$(gcc -Wall -Wextra -I libtci -I . -c "$WORK_DIR/test_logic.c" -o "$WORK_DIR/test_logic.o" 2>&1 \
    && gcc "$WORK_DIR/test_logic.o" vec2.o camera.o scaler.o ground.o map.o shot.o libtci/libtci.a -lm -o "$WORK_DIR/test_logic" 2>&1)
logic_build_status=$?

if [[ "$logic_build_status" -ne 0 ]]; then
    fail "logic tester builds without SDL2" "$logic_build_log"
    exit 1
fi
pass "logic tester builds without SDL2 (vec2.o/camera.o/scaler.o/ground.o/map.o/shot.o only)"

echo
echo "Running the logic tester..."
cd "$WORK_DIR"
logic_out=$(./test_logic)
logic_status=$?
cd - > /dev/null

echo "$logic_out" | grep "^PASS\|^FAIL" | while read -r line; do
    echo "  $line"
done

logic_pass_count=$(echo "$logic_out" | grep -c "^PASS")
logic_fail_count=$(echo "$logic_out" | grep -c "^FAIL")
pass_count=$((pass_count + logic_pass_count))
fail_count=$((fail_count + logic_fail_count))

if [[ "$logic_status" -ne 0 ]]; then
    fail "all logic assertions pass" "see failures above"
fi

# ── headless smoke test of the real binary ───────────────────────────────────

# assets/ ships only font.ttf; the sprites and sounds are generated. Without
# them the binary exits 1 before it ever reaches its event loop, and the smoke
# test below reports "exit code: 1", which says nothing about the real cause.
missing=""
for a in rock.png pillar.png marker.png player.png shot.png \
         sfx_crash.wav sfx_hit.wav sfx_shoot.wav; do
    [[ -f "assets/$a" ]] || missing="$missing $a"
done
if [[ -n "$missing" ]]; then
    fail "generated assets are present" \
        "missing:$missing — run 'bash gen_assets.sh' and 'bash gen_audio.sh' first"
else
    pass "generated assets are present"
fi

echo
echo "Running getaway headless (2s)..."
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout 2 ./getaway "$FIXTURES/map1.txt"
getaway_status=$?
if [[ "$getaway_status" -eq 124 ]]; then
    pass "getaway runs its event loop for 2s without crashing"
else
    fail "getaway runs its event loop for 2s without crashing" "exit code: $getaway_status"
fi

# ── summary ───────────────────────────────────────────────────────────────────

# ── leak report ─────────────────────────────────────────────────────────────
#
# Runs one representative invocation under valgrind and REPORTS what it finds.
# It never changes the pass/fail count. A leak is something to look at, not a
# reason to refuse your work — but you should see it, because a program that
# leaks is a program that will eventually be killed by the machine it runs on.
#
# Leaks are split by whose code lost the memory. A loss record whose stack
# names one of your own .c files is yours. One that lives entirely inside
# SDL, Mesa or glibc is not, and there is nothing for you to fix there.

leak_report() {
    local label="$1"; shift
    local log="${WORK_DIR:-/tmp}/leaks.$$.log"
    local mine=0 theirs=0 rec frames

    if ! command -v valgrind >/dev/null 2>&1; then
        printf "  ${C_BOLD}NOTE${C_RESET}  %s: valgrind is not installed, skipping\n" "$label"
        return 0
    fi

    valgrind --leak-check=full --show-leak-kinds=definite,indirect \
             --error-exitcode=0 --log-file="$log" "$@" >/dev/null 2>&1

    if [[ ! -s "$log" ]]; then
        printf "  ${C_BOLD}NOTE${C_RESET}  %s: valgrind produced no output\n" "$label"
        return 0
    fi

    # Split the log into loss records and ask, of each, whether any frame
    # points at a source file sitting in this directory.
    while IFS= read -r rec; do
        frames=$(sed -n "${rec}"',/^==[0-9]*== *$/p' "$log")
        # Every record carries valgrind's own malloc frame; that is not yours.
        # A frame is yours only if it names a source file sitting right here.
        local f owned=0
        for f in $(grep -oE '\(([A-Za-z0-9_-]+\.c):[0-9]+\)' <<<"$frames" \
                   | tr -d '()' | cut -d: -f1 | sort -u); do
            [[ "$f" == vg_replace_malloc.c ]] && continue
            [[ -f "$f" ]] && owned=1
        done
        if (( owned )); then
            mine=$((mine + 1))
            if (( mine == 1 )); then
                printf "  ${C_RED}LEAK${C_RESET}  %s — memory lost by your code:\n" "$label"
            fi
            grep -E 'bytes in [0-9,]+ blocks are (definitely|indirectly)' <<<"$frames" \
                | sed 's/^==[0-9]*== /        /'
            grep -oE '\(([A-Za-z0-9_-]+\.c:[0-9]+)\)' <<<"$frames" \
                | grep -v vg_replace_malloc | head -3 | tr -d '()' \
                | sed 's/^/          at /'
        else
            theirs=$((theirs + 1))
        fi
    done < <(grep -nE 'bytes in [0-9,]+ blocks are (definitely|indirectly) lost' "$log" | cut -d: -f1)

    if (( mine == 0 )); then
        printf "  ${C_GREEN}OK${C_RESET}    %s — no memory lost by your code" "$label"
        if (( theirs > 0 )); then
            printf ' (%d leak(s) inside libraries you did not write)' "$theirs"
        fi
        printf '\n'
    else
        printf '        this does not fail the tester — fix it anyway\n'
    fi
    rm -f "$log"
    return 0
}

# The graphical chapters run until you quit them, and a program killed
# mid-loop reports everything it has not freed yet as "lost" -- which would be
# a lie. So this starts a virtual display, lets the program run, sends it a
# 'q', and measures the clean exit.
leak_report_gui() {
    local label="$1"; shift
    if ! command -v valgrind >/dev/null 2>&1; then
        printf "  ${C_BOLD}NOTE${C_RESET}  %s: valgrind is not installed, skipping\n" "$label"
        return 0
    fi
    if ! command -v xvfb-run >/dev/null 2>&1 || ! command -v xte >/dev/null 2>&1; then
        printf "  ${C_BOLD}NOTE${C_RESET}  %s: needs xvfb-run and xte for a clean exit, skipping\n" "$label"
        return 0
    fi
    printf "  ${C_BOLD}....${C_RESET}  %s: running under valgrind, this takes a minute\n" "$label"
    local inner="${WORK_DIR:-/tmp}/leak_gui.$$.sh"
    {
        echo "C_GREEN=\"${C_GREEN}\"; C_RED=\"${C_RED}\"; C_BOLD=\"${C_BOLD}\"; C_RESET=\"${C_RESET}\""
        echo "WORK_DIR=\"${WORK_DIR:-/tmp}\""
        declare -f leak_report
        echo '( sleep 12; xte "key q" 2>/dev/null; sleep 5; xte "key q" 2>/dev/null ) &'
        printf 'leak_report %q' "$label"
        printf ' %q' "$@"
        printf '\n'
    } > "$inner"
    timeout 240 xvfb-run -a bash "$inner"
    rm -f "$inner"
    return 0
}

echo
leak_report_gui "getaway" ./getaway "$FIXTURES/map1.txt"

echo
hr
printf "  ${C_BOLD}%d passed, %d failed${C_RESET}\n" "$pass_count" "$fail_count"
hr

if [[ "$fail_count" -gt 0 ]]; then
    exit 1
fi
exit 0
