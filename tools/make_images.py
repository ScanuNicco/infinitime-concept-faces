#!/usr/bin/env python3
"""Generate the Horizon watch face images and the sun-path table used by its C++ code.

    python3 tools/make_images.py

Writes images/horizon_*.png and prints the C++ table for horizon/WatchFaceHorizon.cpp.
Everything is drawn at 4x and scaled down for smooth edges. Requires Pillow.
"""
import math
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parent.parent
SS = 4  # supersampling

# Geometry (screen coordinates; the watch is 240x240). The dome image covers the bottom of the
# screen from DOME_TOP down.
W = 240
DOME_TOP = 156
DOME_H = W - DOME_TOP  # 84
CENTER = (120, 150)  # ellipse centre, relative to the image top
DOME_R = (150, 146)  # dome outline
ARC_R = (124, 118)  # dotted sun path
ARC_FROM, ARC_TO = 146, 34  # degrees: sunrise end (left) to sunset end (right)

TOP, BOTTOM = (0x2F, 0x6F, 0xE0), (0x16, 0x45, 0xAE)


def ellipse_point(r, deg, scale=1):
    t = math.radians(deg)
    return ((CENTER[0] + r[0] * math.cos(t)) * scale, (CENTER[1] - r[1] * math.sin(t)) * scale)


def dome():
    w, h = W * SS, DOME_H * SS
    grad = Image.new("RGBA", (w, h))
    px = grad.load()
    for y in range(h):
        f = y / (h - 1)
        c = tuple(round(a + (b - a) * f) for a, b in zip(TOP, BOTTOM))
        for x in range(w):
            px[x, y] = (*c, 255)
    mask = Image.new("L", (w, h), 0)
    cx, cy = CENTER[0] * SS, CENTER[1] * SS
    ImageDraw.Draw(mask).ellipse([cx - DOME_R[0] * SS, cy - DOME_R[1] * SS, cx + DOME_R[0] * SS, cy + DOME_R[1] * SS],
                                 fill=255)
    img = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    img.paste(grad, (0, 0), mask)
    # dotted sun path: evenly spaced by arc length
    d = ImageDraw.Draw(img)
    pts = [ellipse_point(ARC_R, ARC_FROM + (ARC_TO - ARC_FROM) * i / 2000, SS) for i in range(2001)]
    step, acc, last = 8.5 * SS, 0.0, pts[0]
    dots = [pts[0]]
    for p in pts[1:]:
        acc += math.dist(p, last)
        last = p
        if acc >= step:
            dots.append(p)
            acc = 0
    for x, y in dots:
        r = 1.5 * SS
        d.ellipse([x - r, y - r, x + r, y + r], fill=(255, 255, 255, 215))
    return img.resize((W, DOME_H), Image.LANCZOS)


def sun():
    s = 18 * SS
    img = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    c = s / 2
    d.ellipse([c - 8.5 * SS, c - 8.5 * SS, c + 8.5 * SS, c + 8.5 * SS], fill=(255, 210, 63, 80))  # glow
    d.ellipse([c - 6 * SS, c - 6 * SS, c + 6 * SS, c + 6 * SS], fill=(255, 214, 72, 255))
    return img.resize((18, 18), Image.LANCZOS)


def moon():
    s = 16 * SS
    img = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    d.ellipse([1 * SS, 1 * SS, 15 * SS, 15 * SS], fill=(232, 238, 252, 255))
    d.ellipse([5 * SS, -1 * SS, 18 * SS, 12 * SS], fill=(0, 0, 0, 0))  # bite out of it: a crescent
    return img.resize((16, 16), Image.LANCZOS)


def stars(seed=7, count=70):
    """1-bit starfield for Constellation: white stars on black (black becomes transparent on the watch).

    InfiniTime's converter keeps only the lowest bit of each grey value, so pixels must be exactly
    0 or 255 (no anti-aliasing).
    """
    import random

    rnd = random.Random(seed)
    img = Image.new("L", (W, W), 0)
    px = img.load()
    for _ in range(count):
        x, y = rnd.randrange(2, W - 2), rnd.randrange(2, W - 2)
        px[x, y] = 255
        size = rnd.random()
        if size > 0.75:  # a few brighter stars: small crosses
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                px[x + dx, y + dy] = 255
        elif size > 0.5:
            px[x + 1, y] = 255
    return img


# Weather icons for Outlook, drawn as images (no RAM needed, unlike a font with every glyph).
# Font Awesome 5 Free Solid code points, the same glyphs InfiniTime's WeatherSymbols.cpp uses.
WEATHER_ICONS = {
    "sun": 0xF185, "moon": 0xF186, "cloudsun": 0xF6C4, "cloudmoon": 0xF6C3, "cloud": 0xF0C2,
    "brokenclouds": 0xF73B, "showers": 0xF740, "sunrain": 0xF743, "moonrain": 0xF73C,
    "bolt": 0xF0E7, "snow": 0xF2DC, "smog": 0xF75F, "unknown": 0xF05E,
}
WEATHER_SIZES = {"outlook_wx_": 44, "outlook_wxs_": 20}  # image name prefix -> box size in px
YELLOW = (0xDD, 0xDD, 0x00)


def glyph(codepoint, box):
    from PIL import ImageFont

    font = ImageFont.truetype(str(ROOT / "fonts" / "FontAwesome5-Free-Solid.ttf"), box * SS)
    img = Image.new("RGBA", (box * SS * 2, box * SS * 2), (0, 0, 0, 0))
    ImageDraw.Draw(img).text((box * SS // 2, box * SS // 2), chr(codepoint), font=font, fill=(*YELLOW, 255))
    img = img.crop(img.getbbox())
    scale = box * SS / max(img.size)  # fit the glyph in the box, keeping its shape
    img = img.resize((max(1, round(img.width * scale)), max(1, round(img.height * scale))), Image.LANCZOS)
    out = Image.new("RGBA", (box * SS, box * SS), (0, 0, 0, 0))
    out.paste(img, ((out.width - img.width) // 2, (out.height - img.height) // 2), img)
    return out.resize((box, box), Image.LANCZOS)


# Rally geometry (keep in sync with rally/WatchFaceRally.cpp)
RALLY_TICKS_HALF = 110  # outer ticks end on a square of this half-size (inside the edge band)
RALLY_DIAL = 58         # inner dial half-size


def square_ray(cx, cy, half, deg):
    """Where a ray from (cx, cy) at deg (clockwise from 12) meets a square of half-size half."""
    t = math.radians(deg)
    dx, dy = math.sin(t), -math.cos(t)
    m = max(abs(dx), abs(dy))
    return dx / m, dy / m  # direction scaled so the larger component is 1


def one_bit(draw_fn, size):
    """InfiniTime's 1-bit images are white (255) on transparent (0). Draw at 4x (draw_fn gets the
    scale) and scale down before thresholding, so thin angled lines come out even, not jagged."""
    img = Image.new("L", (size[0] * SS, size[1] * SS), 0)
    draw_fn(ImageDraw.Draw(img), SS)
    img = img.resize(size, Image.LANCZOS)
    return img.point(lambda v: 255 if v >= 110 else 0)


def rally_ticks(major: bool):
    """Outer bezel ticks along the square edge: every minute (minor) or every five (major)."""
    def draw(d, k):
        for m in range(60):
            if (m % 5 == 0) != major or m == 0:  # 0: the red mark is drawn by the face
                continue
            ux, uy = square_ray(120, 120, 1, m * 6)
            length, width = (14, 3) if major else (8, 2)
            r1 = RALLY_TICKS_HALF
            d.line([((120 + ux * (r1 - length)) * k, (120 + uy * (r1 - length)) * k),
                    ((120 + ux * r1) * k, (120 + uy * r1) * k)], fill=255, width=width * k)
    return one_bit(draw, (W, W))


def rally_dial_ticks():
    """Short marks across the inner dial's outline at each hour (image covers the dial only)."""
    size = 2 * RALLY_DIAL + 8
    c = size / 2

    def draw(d, k):
        for h in range(12):
            ux, uy = square_ray(c, c, 1, h * 30)
            r = RALLY_DIAL - 1
            d.line([((c + ux * (r - 4)) * k, (c + uy * (r - 4)) * k), ((c + ux * (r + 3)) * k, (c + uy * (r + 3)) * k)],
                   fill=255, width=2 * k)
    return one_bit(draw, (size, size))


def path_table(n=24):
    """Screen positions along the dotted arc, from sunrise (index 0) to sunset (index n)."""
    out = []
    for i in range(n + 1):
        x, y = ellipse_point(ARC_R, ARC_FROM + (ARC_TO - ARC_FROM) * i / n)
        out.append((round(x), round(y) + DOME_TOP))
    return out


if __name__ == "__main__":
    (ROOT / "images").mkdir(exist_ok=True)
    dome().save(ROOT / "images" / "horizon_dome.png")
    sun().save(ROOT / "images" / "horizon_sun.png")
    moon().save(ROOT / "images" / "horizon_moon.png")
    stars().save(ROOT / "images" / "constellation_stars.png")
    rally_ticks(False).save(ROOT / "images" / "rally_ticks_minor.png")
    rally_ticks(True).save(ROOT / "images" / "rally_ticks_major.png")
    rally_dial_ticks().save(ROOT / "images" / "rally_dial_ticks.png")
    for prefix, box in WEATHER_SIZES.items():
        for name, cp in WEATHER_ICONS.items():
            glyph(cp, box).save(ROOT / "images" / f"{prefix}{name}.png")
    table = path_table()
    print(f"// generated by tools/make_images.py: dome top y = {DOME_TOP}")
    print(f"constexpr lv_point_t sunPath[{len(table)}] = {{" + ", ".join(f"{{{x}, {y}}}" for x, y in table) + "};")
