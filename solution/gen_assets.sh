#!/bin/bash
# Generate placeholder obstacle sprites for g03.
#
# cone.png: an orange traffic cone, transparent background.
# barrier.png: a red-and-white striped barrier, transparent background.
#
# Both are drawn at a fixed 128x128 so render.c's SDL_RenderCopy(...,
# NULL, &dst) always samples the whole texture -- the scaling happens
# entirely in the destination rect, not the source, same as r01/r02.

set -e

mkdir -p assets

python3 - <<'EOF'
from PIL import Image, ImageDraw

SIZE = 128

cone = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
d = ImageDraw.Draw(cone)
d.rectangle([SIZE // 2 - 44, SIZE - 16, SIZE // 2 + 44, SIZE], fill=(0x3a, 0x3a, 0x3a, 255))
d.polygon(
    [(SIZE // 2, 8), (SIZE // 2 + 34, SIZE - 16), (SIZE // 2 - 34, SIZE - 16)],
    fill=(0xe8, 0x6a, 0x1a, 255),
)
d.polygon(
    [(SIZE // 2 - 20, SIZE - 46), (SIZE // 2 + 20, SIZE - 46),
     (SIZE // 2 + 26, SIZE - 32), (SIZE // 2 - 26, SIZE - 32)],
    fill=(0xf5, 0xf5, 0xf5, 255),
)
cone.save("assets/cone.png")
print("  wrote assets/cone.png")

barrier = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
d = ImageDraw.Draw(barrier)
d.rectangle([8, SIZE - 90, SIZE - 8, SIZE - 30], fill=(0xf5, 0xf5, 0xf5, 255))
stripe_w = 16
x = 8
flip = False
while x < SIZE - 8:
    w = min(stripe_w, SIZE - 8 - x)
    if flip:
        d.rectangle([x, SIZE - 90, x + w, SIZE - 30], fill=(0xc8, 0x1e, 0x1e, 255))
    x += w
    flip = not flip
d.rectangle([16, SIZE - 30, 26, SIZE], fill=(0x3a, 0x3a, 0x3a, 255))
d.rectangle([SIZE - 26, SIZE - 30, SIZE - 16, SIZE], fill=(0x3a, 0x3a, 0x3a, 255))
barrier.save("assets/barrier.png")
print("  wrote assets/barrier.png")
EOF
