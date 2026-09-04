#!/bin/bash
# Synthesize every sound g03 needs -- own-work, generated from square
# waves with Python's stdlib `wave` module, the same procedurally-
# generated-art spirit as gen_assets.sh's PIL rectangles and g02b's
# gen_audio.sh. Three files, matching the three events audio.c
# actually handles:
#
# assets/sfx_shoot.wav  -- short descending zap, kept brief on purpose
#                          so a held-down burst layers instead of
#                          smearing into a wash of overlapping tails
# assets/sfx_hit.wav    -- a quick tick into a short falling crack,
#                          distinct from both the shot that caused it
#                          and the crash a collision plays
# assets/sfx_crash.wav  -- a low thud into a descending rumble

set -e

mkdir -p assets

python3 - <<'EOF'
import math
import struct
import wave

RATE = 44100

def square_wave(freq, duration, amplitude=0.3, fade=0.01):
    n = int(RATE * duration)
    fade_n = int(RATE * fade)
    samples = []
    for i in range(n):
        t = i / RATE
        s = amplitude if math.sin(2 * math.pi * freq * t) >= 0 else -amplitude
        if i < fade_n:
            s *= i / fade_n
        elif i > n - fade_n:
            s *= (n - i) / fade_n
        samples.append(s)
    return samples

def sweep_wave(freq_start, freq_end, duration, amplitude=0.3, fade=0.01):
    n = int(RATE * duration)
    fade_n = int(RATE * fade)
    samples = []
    phase = 0.0
    for i in range(n):
        frac = i / n
        freq = freq_start + (freq_end - freq_start) * frac
        phase += freq / RATE
        s = amplitude if math.sin(2 * math.pi * phase) >= 0 else -amplitude
        if i < fade_n:
            s *= i / fade_n
        elif i > n - fade_n:
            s *= (n - i) / fade_n
        samples.append(s)
    return samples

def concat(*parts):
    out = []
    for p in parts:
        out.extend(p)
    return out

def write_wav(path, samples):
    with wave.open(path, "w") as f:
        f.setnchannels(1)
        f.setsampwidth(2)
        f.setframerate(RATE)
        frames = b"".join(
            struct.pack("<h", max(-32767, min(32767, int(s * 32767))))
            for s in samples
        )
        f.writeframes(frames)
    print(f"  wrote {path}")

write_wav("assets/sfx_shoot.wav", sweep_wave(1400, 500, 0.06, amplitude=0.25))
write_wav("assets/sfx_hit.wav", concat(
    square_wave(500, 0.02, amplitude=0.35),
    sweep_wave(700, 200, 0.06, amplitude=0.3),
))
write_wav("assets/sfx_crash.wav", concat(
    square_wave(70, 0.05, amplitude=0.4),
    sweep_wave(200, 45, 0.2, amplitude=0.3),
))
EOF
