#!/usr/bin/env python3
"""Generate infinitime-app.json.

    python3 tools/make_manifest.py

Font symbol sets are computed from the exact strings each face can show, so every font
contains only the glyphs it needs (fonts are loaded into the watch's RAM).
"""
import json
from pathlib import Path

from make_images import WEATHER_ICONS, WEATHER_SIZES

ROOT = Path(__file__).resolve().parent.parent

MONTHS_UP = "JAN FEB MAR APR MAY JUN JUL AUG SEP OCT NOV DEC"
MONTHS_LOW = "Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec"
DAYS_UP = "MONDAY TUESDAY WEDNESDAY THURSDAY FRIDAY SATURDAY SUNDAY"
DAYS_LOW = "Monday Tuesday Wednesday Thursday Friday Saturday Sunday"
DAYS_SHORT_UP = "MON TUE WED THU FRI SAT SUN"
DAYS_SHORT_LOW = "Mon Tue Wed Thu Fri Sat Sun"
DIGITS = "0123456789"
DEGREE = "°"

LIGHT = "fonts/BarlowCondensed-Light.ttf"
MEDIUM = "fonts/BarlowCondensed-Medium.ttf"
SEMIBOLD = "fonts/BarlowCondensed-SemiBold.ttf"



def syms(*parts: str) -> str:
    out = "".join(sorted(set("".join(parts))))
    # lv_font_conv would read a value starting with "-" as an option
    return out[1:] + "-" if out.startswith("-") else out


def font(file: str, size: int, *, symbols: str | None = None, range_: str | None = None, bpp: int = 2) -> dict:
    src = {"file": file}
    if symbols:
        src["symbols"] = symbols
    if range_:
        src["range"] = range_
    return {"sources": [src], "size": size, "bpp": bpp}


def image(path: str, color_format: str = "CF_TRUE_COLOR_ALPHA") -> dict:
    return {"sources": path, "color_format": color_format}


def face(enum: str, name: str, folder: str, description: str, fonts=None, images=None) -> dict:
    item = {
        "type": "watchface",
        "enum": enum,
        "name": name,
        "description": description,
        "header": f"{folder}/WatchFace{enum}.h",
        "sources": [f"{folder}/WatchFace{enum}.cpp"],
        "screenshot": f"screenshots/{folder}.png",  # shown by InfiniTime Builder
    }
    resources = {}
    if fonts:
        resources["fonts"] = fonts
    if images:
        resources["images"] = images
    if resources:
        item["resources"] = resources
    return item


# Conditions as Constellation writes them (see constellation/WatchFaceConstellation.cpp).
CONDITIONS = "Clear Cloudy Showers Raining Storm Snowing Misty"

FACES = [
    face("Tiles", "Tiles", "tiles", "Big light digits on dark red tiles, like a flip clock.", fonts={
        "tiles_digits": font(LIGHT, 72, symbols=DIGITS),
        "tiles_text": font(SEMIBOLD, 22, symbols=syms(DIGITS, " ", MONTHS_UP, DAYS_UP, "AMPM")),
    }),
    face("Horizon", "Horizon", "horizon",
         "Time and date over a dome where the sun travels from today's sunrise to sunset, with the weather.",
         fonts={
             "horizon_time": font(SEMIBOLD, 54, symbols="0123456789: AMP"),
             "horizon_day": font(LIGHT, 34, symbols=syms(DAYS_LOW)),
             "horizon_small": font(MEDIUM, 16, range_="0x20-0x7E,0xB0"),
         }),
    face("Words", "Words", "words", "A word clock: the time spelled out among dim letters, with the date in yellow."),
    face("Rally", "Rally", "rally",
         "A small analog clock in a ticked, numbered bezel; the band round the edge shows your steps and battery.",
         fonts={"rally_digits": font(SEMIBOLD, 16, symbols=DIGITS)}),
    face("Outlook", "Outlook", "outlook", "Large time and date, the weather and battery, and the coming days' forecast.",
         fonts={
             "outlook_time": font(LIGHT, 88, symbols=DIGITS + ":"),
             "outlook_temp": font(SEMIBOLD, 48, symbols=syms(DIGITS, "-", DEGREE)),
             "outlook_text": font(SEMIBOLD, 19, symbols=syms(DIGITS, " %", MONTHS_UP, DAYS_UP)),
         },
         # weather icons as images: drawn from flash, so they cost no RAM (a font would hold every icon)
         images={f"{prefix}{name}": image(f"images/{prefix}{name}.png")
                 for prefix in WEATHER_SIZES for name in WEATHER_ICONS}),
    face("Glow", "Glow", "glow", "Minimal analog: cyan hands, a red second hand, the date, battery and Bluetooth.",
         fonts={"glow_text": font(SEMIBOLD, 15, symbols=syms(DIGITS, " ", DAYS_SHORT_UP, MONTHS_UP))}),
    face("Noir", "Noir", "noir", "Long white hands and a red second hand on black, with the date at three.",
         fonts={"noir_text": font(MEDIUM, 16, symbols=syms(DIGITS, " ", DAYS_SHORT_UP))}),
    face("Subdial", "Subdial", "subdial", "A dress-watch face with a small-seconds sub-dial, date window and weekday.",
         fonts={"subdial_text": font(MEDIUM, 18, symbols=syms(DIGITS, DAYS_LOW))}),
    face("Constellation", "Constellation", "constellation",
         "The hour and minute as stars joined like a constellation, on a starry sky, with the weather and date.",
         fonts={"constellation_text": font(MEDIUM, 22, symbols=syms(DIGITS, " -", DEGREE, CONDITIONS, MONTHS_UP, DAYS_SHORT_UP))}),
    face("Almanac", "Almanac", "almanac",
         "The time as two rulers, hours above and minutes below, with the weather and date.",
         fonts={
             "almanac_hour": font(LIGHT, 46, symbols=DIGITS),
             "almanac_minute": font(LIGHT, 38, symbols=DIGITS),
             "almanac_mid": font(LIGHT, 26, symbols=DIGITS + " "),
             "almanac_small": font(MEDIUM, 19, symbols=syms(DIGITS, " -", DEGREE, "CF", DAYS_SHORT_LOW, MONTHS_LOW)),
         }),
]

MANIFEST = {
    "manifestVersion": 1,
    "name": "Concept Faces",
    "author": "InfiniTime Builder examples",
    "description": "Watch faces adapted from Pebble concept designs. Most use custom fonts and images from the resource pack.",
    "homepage": "https://github.com/ScanuNicco/infinitime-concept-faces",
    "minInfiniTime": "1.16.0",
    "apps": FACES,
}

if __name__ == "__main__":
    (ROOT / "infinitime-app.json").write_text(json.dumps(MANIFEST, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"wrote {len(FACES)} faces")
