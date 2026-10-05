#!/usr/bin/env python3
"""Generates the PS Vita LiveArea images for the tech demo.

The artwork is drawn here from scratch (gradients, geometric shapes and the
Pillow default font); it contains no original game art or logos. The Vita
installer expects 8-bit palettized PNGs at fixed sizes.
"""
import os
from PIL import Image, ImageDraw, ImageFont

OUT = os.path.join(os.path.dirname(__file__), "..", "assets", "vita", "sce_sys")


def font(size):
    try:
        return ImageFont.load_default(size=size)
    except TypeError:  # Pillow < 10.1
        return ImageFont.load_default()


def backdrop(w, h):
    img = Image.new("RGB", (w, h))
    px = img.load()
    for y in range(h):
        t = y / max(1, h - 1)
        # Dusk sky over sand: deep blue to warm ochre.
        r = int(24 + t * 170)
        g = int(36 + t * 120)
        b = int(84 - t * 40)
        for x in range(w):
            px[x, y] = (r, g, b)
    d = ImageDraw.Draw(img)
    # A stepped ziggurat silhouette and a sun disc.
    base = int(h * 0.92)
    cx = int(w * 0.72)
    step_w, step_h = w * 0.28, h * 0.11
    for i in range(4):
        sw = step_w * (1 - i * 0.22)
        d.rectangle([cx - sw / 2, base - step_h * (i + 1), cx + sw / 2,
                     base - step_h * i], fill=(46, 30, 22))
    d.ellipse([w * 0.12, h * 0.18, w * 0.12 + h * 0.22, h * 0.40],
              fill=(250, 214, 120))
    d.rectangle([0, base, w, h], fill=(70, 46, 28))
    return img, d


def text(d, xy, s, size, fill):
    f = font(size)
    x, y = xy
    d.text((x + 2, y + 2), s, font=f, fill=(0, 0, 0))
    d.text((x, y), s, font=f, fill=fill)


def save(img, path):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    img.convert("P", palette=Image.Palette.ADAPTIVE, colors=255).save(path)


def main():
    icon, d = backdrop(128, 128)
    text(d, (10, 12), "AoE", 40, (255, 222, 120))
    text(d, (10, 58), "RoR", 28, (255, 255, 255))
    text(d, (10, 96), "VITA", 18, (180, 220, 255))
    save(icon, os.path.join(OUT, "icon0.png"))

    bg, d = backdrop(840, 500)
    text(d, (40, 60), "Age of Empires: Rise of Rome", 44, (255, 222, 120))
    text(d, (40, 120), "Clean-room reconstruction - PS Vita tech demo", 26,
         (255, 255, 255))
    text(d, (40, 170), "Requires your own game data in ux0:data/VitaAoE", 22,
         (200, 230, 255))
    save(bg, os.path.join(OUT, "livearea", "contents", "bg.png"))

    start, d = backdrop(280, 158)
    text(d, (14, 40), "AoE RoR", 34, (255, 222, 120))
    text(d, (14, 92), "Vita tech demo", 20, (255, 255, 255))
    save(start, os.path.join(OUT, "livearea", "contents", "startup.png"))


if __name__ == "__main__":
    main()
