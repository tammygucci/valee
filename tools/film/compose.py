#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Composes the filmed screen (240x240 a frame) with the captions into 1920x1080 frames: raw RGB to stdout,
or with --still N1,N2,... PNGs of those frames to build/film/still_N.png. Needs Poppins (Google Fonts)."""
import json, sys, textwrap
from PIL import Image, ImageDraw, ImageFont

import os
OUT = os.environ.get("FILM_OUT", "build/film")
W, H, S = 1920, 1080, 4
SX, SY = 1920 - 960 - 60, 60
BG = (13, 14, 27)
GREEN = (90, 255, 110)
FONTS = "/usr/share/fonts/truetype/google-fonts"
FONT_B = f"{FONTS}/Poppins-Bold.ttf"
FONT_R = f"{FONTS}/Poppins-Regular.ttf"
FONT_M = f"{FONTS}/Poppins-Medium.ttf"
FADE = 9
END_FADE = 20

meta = json.load(open(f"{OUT}/captions.json"))
N, caps = meta["frames"], meta["caps"]
fb = open(f"{OUT}/screen.rgb", "rb")


def font(p, s):
    return ImageFont.truetype(p, s)


f_label, f_title, f_sub = font(FONT_M, 30), font(FONT_B, 76), font(FONT_R, 38)


def base():
    im = Image.new("RGB", (W, H), BG)
    d = ImageDraw.Draw(im)
    d.rounded_rectangle([SX - 14, SY - 14, SX + 960 + 13, SY + 960 + 13], 18, outline=(42, 46, 72), width=3)
    return im


def caption_layer(title, sub):
    lay = Image.new("RGBA", (SX - 40, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(lay)
    x = 110
    d.text((x, 96), "VALLÉE", font=f_label, fill=GREEN + (255,))
    d.text((x, 136), "for the M-VAVE FM-1", font=font(FONT_R, 24), fill=(110, 118, 140, 255))
    if not title:
        return lay
    tl = textwrap.wrap(title, 18)
    sl = textwrap.wrap(sub, 32)
    th = len(tl) * 92 + (24 + len(sl) * 54 if sl else 0)
    y = (H - th) // 2
    for line in tl:
        d.text((x, y), line, font=f_title, fill=(240, 242, 250, 255))
        y += 92
    y += 24
    for line in sl:
        d.text((x, y), line, font=f_sub, fill=(150, 158, 182, 255))
        y += 54
    return lay


def end_card():
    im = Image.new("RGB", (W, H), (0, 0, 0))
    logo = Image.open("web/brand/logo.png").convert("RGB")
    logo = logo.resize((logo.width * 3 // 2, logo.height * 3 // 2), Image.NEAREST)
    # the logo's own black ground, as the page's
    im.paste(logo, ((W - logo.width) // 2, 120))
    d = ImageDraw.Draw(im)
    def ctr(y, s, f, c):
        w = d.textlength(s, font=f)
        d.text(((W - w) / 2, y), s, font=f, fill=c)
    y = 120 + logo.height + 40
    ctr(y, "VALLÉE", font(FONT_B, 84), (240, 242, 250))
    ctr(y + 110, "tammygucci.github.io/valee", font(FONT_M, 40), GREEN)
    ctr(y + 175, "free firmware for the M-VAVE FM-1 · built on Skiff by illuminus5 · GPL-3.0", font(FONT_R, 28),
        (110, 118, 140))
    return im


BASE = base()
layers = [(f, caption_layer(t, s) if t != "END" else None) for f, t, s in caps]
END = end_card()


def frame(i):
    fb.seek(i * 172800)
    scr = Image.frombytes("RGB", (240, 240), fb.read(172800)).resize((960, 960), Image.NEAREST)
    im = BASE.copy()
    im.paste(scr, (SX, SY))
    k = max(j for j, (f, _) in enumerate(layers) if f <= i)
    f0, lay = layers[k]
    if lay is None:                                    # the end card, faded in
        a = min(1.0, (i - f0) / END_FADE)
        prev = layers[k - 1][1]
        im.paste(prev, (0, 0), prev)
        return Image.blend(im, END, a)
    a = min(1.0, (i - f0) / FADE) if k else 1.0
    if a < 1.0 and k:
        prev = layers[k - 1][1]
        if prev is not None:
            pa = prev.copy()
            pa.putalpha(pa.getchannel("A").point(lambda v: int(v * (1 - a))))
            # the label stays: draw it fully from the new layer below
            im.paste(pa, (0, 0), pa)
        la = lay.copy()
        la.putalpha(la.getchannel("A").point(lambda v: int(v * a)))
        im.paste(la, (0, 0), la)
    else:
        im.paste(lay, (0, 0), lay)
    return im


if len(sys.argv) > 2 and sys.argv[1] == "--still":
    for n in map(int, sys.argv[2].split(",")):
        frame(n).save(f"{OUT}/still_{n}.png")
else:
    a, b = (int(x) for x in sys.argv[1:3]) if len(sys.argv) > 2 else (0, N)
    out = sys.stdout.buffer
    for i in range(a, b):
        out.write(frame(i).tobytes())
