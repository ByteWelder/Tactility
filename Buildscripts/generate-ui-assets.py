#!/usr/bin/env python3
"""
Generates the spinner and cursor images in Data/system/ui/ from Data/system_sources/:
- spinner_<size>.png from TactilitySpinner.svg
- cursor_<size>.png from cursor.svg

The sizes are in pixels. They must match the ones in Modules/lvgl-module/source/ui_assets.c.

Requires the cairosvg package: pip install cairosvg
"""
import os
import re

import cairosvg

SPINNER_SIZES = [12, 16, 20, 24, 32, 36, 40]
CURSOR_SIZES = [12, 16, 20, 24, 32, 40]

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SOURCES = os.path.join(ROOT, "Data", "system_sources")
OUTPUT = os.path.join(ROOT, "Data", "system", "ui")


def load_svg(svg_name: str) -> bytes:
    """
    Inkscape leaves x, y, width and height attributes on paths that came from rectangles.
    cairosvg applies them to the path, which displaces and distorts it, so they're removed.
    """
    with open(os.path.join(SOURCES, svg_name), "r", encoding="utf-8") as file:
        svg = file.read()
    svg = re.sub(
        r"<path\b.*?>",
        lambda tag: re.sub(r'\s+(?:x|y|width|height)="[^"]*"', "", tag[0]),
        svg,
        flags=re.S,
    )
    return svg.encode("utf-8")


def render(svg_name: str, output_name: str, size: int):
    cairosvg.svg2png(
        bytestring=load_svg(svg_name),
        write_to=os.path.join(OUTPUT, output_name),
        output_width=size,
        output_height=size,
    )
    print(output_name)


def main():
    os.makedirs(OUTPUT, exist_ok=True)
    for size in SPINNER_SIZES:
        render("TactilitySpinner.svg", f"spinner_{size}.png", size)
    for size in CURSOR_SIZES:
        render("cursor.svg", f"cursor_{size}.png", size)


if __name__ == "__main__":
    main()
