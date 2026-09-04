#!/bin/bash
# Generate placeholder sprites for g03.
#
# rock.png / pillar.png: obstacles to dodge, transparent background.
# marker.png: small background decoration, never checked for collision.
# player.png: the player's own craft, drawn at a fixed screen position.
# shot.png: a fired projectile, drawn through the same scaler as
# obstacles and markers.
#
# All drawn at a fixed 128x128 (player.png at 128x80) so render.c's
# SDL_RenderCopy(..., NULL, &dst) always samples the whole texture --
# the scaling happens entirely in the destination rect, same as r01/r02.
#
# Colours are hestia's own palette -- the same violet accent
# (#7c3aed / #5b21b6 / #9e77f7) thecodingidiot.com's own homepage
# tunnel already uses, not a new scheme invented for this chapter.

set -e

mkdir -p assets

python3 - <<'EOF'
from PIL import Image, ImageDraw

SIZE = 128

rock = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
d = ImageDraw.Draw(rock)
d.polygon(
    [
        (18, SIZE - 10), (8, SIZE - 55), (34, SIZE - 100),
        (SIZE - 30, SIZE - 104), (SIZE - 10, SIZE - 58), (SIZE - 20, SIZE - 10),
    ],
    fill=(0x5b, 0x21, 0xb6, 255),
)
rock.save("assets/rock.png")
print("  wrote assets/rock.png")

pillar = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
d = ImageDraw.Draw(pillar)
d.rectangle([SIZE // 2 - 22, 10, SIZE // 2 + 22, SIZE - 6], fill=(0x9e, 0x77, 0xf7, 255))
d.rectangle([SIZE // 2 - 30, SIZE - 22, SIZE // 2 + 30, SIZE - 6], fill=(0x7c, 0x3a, 0xed, 255))
d.rectangle([SIZE // 2 - 26, 4, SIZE // 2 + 26, 18], fill=(0x7c, 0x3a, 0xed, 255))
pillar.save("assets/pillar.png")
print("  wrote assets/pillar.png")

marker = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
d = ImageDraw.Draw(marker)
d.ellipse([SIZE // 2 - 30, SIZE - 40, SIZE // 2 + 30, SIZE - 4], fill=(0x5b, 0x21, 0xb6, 255))
d.ellipse([SIZE // 2 - 14, SIZE - 60, SIZE // 2 + 14, SIZE - 24], fill=(0x9e, 0x77, 0xf7, 255))
marker.save("assets/marker.png")
print("  wrote assets/marker.png")

PLAYER_W, PLAYER_H = 128, 80
player = Image.new("RGBA", (PLAYER_W, PLAYER_H), (0, 0, 0, 0))
d = ImageDraw.Draw(player)
d.polygon(
    [(4, PLAYER_H // 2), (PLAYER_W // 2 - 6, PLAYER_H // 2 - 8),
     (PLAYER_W // 2 - 6, PLAYER_H // 2 + 8)],
    fill=(0xe0, 0xe0, 0xe0, 255),
)
d.polygon(
    [(PLAYER_W - 4, PLAYER_H // 2), (PLAYER_W // 2 + 6, PLAYER_H // 2 - 8),
     (PLAYER_W // 2 + 6, PLAYER_H // 2 + 8)],
    fill=(0xe0, 0xe0, 0xe0, 255),
)
d.ellipse(
    [PLAYER_W // 2 - 16, PLAYER_H // 2 - 14, PLAYER_W // 2 + 16, PLAYER_H // 2 + 14],
    fill=(0x7c, 0x3a, 0xed, 255),
)
d.ellipse(
    [PLAYER_W // 2 - 7, PLAYER_H // 2 - 7, PLAYER_W // 2 + 7, PLAYER_H // 2 + 7],
    fill=(0xff, 0xff, 0xff, 255),
)
player.save("assets/player.png")
print("  wrote assets/player.png")

shot = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
d = ImageDraw.Draw(shot)
d.ellipse(
    [SIZE // 2 - 26, SIZE // 2 - 26, SIZE // 2 + 26, SIZE // 2 + 26],
    fill=(0x9e, 0x77, 0xf7, 255),
)
d.ellipse(
    [SIZE // 2 - 12, SIZE // 2 - 12, SIZE // 2 + 12, SIZE // 2 + 12],
    fill=(0xff, 0xff, 0xff, 255),
)
shot.save("assets/shot.png")
print("  wrote assets/shot.png")
EOF
