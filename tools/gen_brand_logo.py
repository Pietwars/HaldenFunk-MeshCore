#!/usr/bin/env python3
"""Erzeugt das Display-Bild der HaldenFunk-Anzeige aus dem vorhandenen HaldenFunk-Titelbild (Pillow nötig).

Quelle: docs/flyer/haldenfunk-header.jpg (eigene Grafik: Halden, Förderturm, Funkwellen; gleiches Motiv wie
web/bg.webp). Das Titelbild hat keinen Schriftzug; deshalb wird ein 16:9-Ausschnitt genommen und der
Schriftzug „⚒ HaldenFunk“ mit der Unterzeile „Glück Auf · MeshCore“ in den HaldenFunk-Farben der
Weboberfläche (Kupfer #e8955a, Sand #e3c39d) links oben in den leeren Nachthimmel gesetzt.

- logo/haldenfunk-display.png                    Display-Bild (1083 x 609)
- examples/companion_radio/ui-brand/BrandLogo.h  RGB565: 320x180 (T-Deck Plus), 240x140 (T-Watch S3, BRAND_SMALL)
Aufruf: python3 tools/gen_brand_logo.py   (Schrift: DejaVu Sans Bold, z. B. Paket fonts-dejavu-core)
"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFilter, ImageFont
ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / 'docs/flyer/haldenfunk-header.jpg'
LOGO = ROOT / 'logo/haldenfunk-display.png'
OUT = ROOT / 'examples/companion_radio/ui-brand/BrandLogo.h'
SIZES = {'': (320, 180), 'BRAND_SMALL': (240, 140)}
COPPER, SAND = (232, 149, 90), (227, 195, 157)
FONTS = ['/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf', '/usr/share/fonts/dejavu/DejaVuSans-Bold.ttf',
         '/Library/Fonts/DejaVuSans-Bold.ttf', 'DejaVuSans-Bold.ttf']

def font(size):
    for f in FONTS:
        try: return ImageFont.truetype(f, size)
        except OSError: pass
    raise SystemExit('Schrift DejaVu Sans Bold nicht gefunden')

def display_image(src):
    """16:9-Ausschnitt (unten: Halden + Förderturm bleiben ganz) + Schriftzug links oben im Himmel."""
    W, H = src.size; h = round(W * 9 / 16)
    img = src.crop((0, H - h, W, H)).convert('RGB')
    d = ImageDraw.Draw(img)
    big, small = font(118), font(54)
    x, y = 70, 22
    t1, t2 = '⚒ HaldenFunk', 'Glück Auf · MeshCore'
    b1 = d.textbbox((x, y), t1, font=big); y2 = b1[3] + 18
    b2 = d.textbbox((x + 6, y2), t2, font=small)
    # weicher dunkler Schatten, damit die Schrift auch verkleinert auf 240 px lesbar bleibt
    sh = Image.new('L', img.size, 0); sd = ImageDraw.Draw(sh)
    sd.text((x, y), t1, font=big, fill=200); sd.text((x + 6, y2), t2, font=small, fill=200)
    img = Image.composite(Image.new('RGB', img.size, (8, 6, 8)), img, sh.filter(ImageFilter.MaxFilter(9)).filter(ImageFilter.GaussianBlur(10)))
    d = ImageDraw.Draw(img)
    d.text((x, y), t1, font=big, fill=COPPER)
    d.text((x + 6, y2), t2, font=small, fill=SAND)
    return img

def fit(img, w, h):
    """auf Seitenverhältnis w:h zuschneiden (rechts bzw. oben weg, Schriftzug links oben bleibt), dann skalieren"""
    r = w / h
    if img.width / img.height > r:
        cw = round(img.height * r); img = img.crop((0, 0, cw, img.height))
    else:
        ch = round(img.width / r); img = img.crop((0, img.height - ch, img.width, img.height))
    return img.resize((w, h), Image.LANCZOS)

def arr(img, w, h):
    im = fit(img, w, h).convert('RGB'); b = im.tobytes()
    vals = ['0x%04X' % (((b[i] >> 3) << 11) | ((b[i + 1] >> 2) << 5) | (b[i + 2] >> 3)) for i in range(0, len(b), 3)]
    lines = [', '.join(vals[i:i + 16]) for i in range(0, len(vals), 16)]
    return f'#define BRAND_LOGO_W {w}\n#define BRAND_LOGO_H {h}\nstatic const uint16_t BRAND_LOGO[{w * h}] PROGMEM = {{\n' + ',\n'.join(lines) + '\n};\n'

if __name__ == '__main__':
    src = Image.open(HEADER).convert('RGB')
    logo = display_image(src); logo.save(LOGO, optimize=True); print('geschrieben:', LOGO, logo.size)
    out = ['// Erzeugt von tools/gen_brand_logo.py - nicht von Hand aendern.',
           '// Display-Bild aus dem HaldenFunk-Titelbild (docs/flyer/haldenfunk-header.jpg) + Schriftzug HaldenFunk.',
           '#pragma once', '#include <Arduino.h>',
           '#if defined(BRAND_SMALL)', arr(logo, *SIZES['BRAND_SMALL']), '#else', arr(logo, *SIZES['']), '#endif', '']
    OUT.write_text('\n'.join(out), encoding='utf-8'); print('geschrieben:', OUT)
