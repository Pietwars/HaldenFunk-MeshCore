#!/usr/bin/env python3
"""Vorschau-PNGs der Marken-Anzeige (examples/companion_radio/ui-brand), am PC nachgestellt.

Bildet UITask::render() mit denselben Maßen nach (glcd-5x7-Schrift aus Adafruit GFX, Logo wie
tools/gen_brand_logo.py), inklusive Akku-Zeile (UITask::drawBattery) mit Beispielwerten im Format von
ui-brand/BattTrend.h (keine echten Messwerte). HaldenFunk-Farben (Anthrazit/Kupfer); keine PIN-Anzeige
(Bluetooth ohne PIN).
Aufruf:  brand_preview.py <Gerät> <Ausgabeordner> [<PlatformIO-Env>]   (braucht Pillow)
         brand_preview.py --has <Gerät>   -> Exit 0, wenn das Gerät die Marken-Anzeige hat
"""
import glob, re, sys
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
DEVICES = {'LilyGo_TDeck': (320, 240, 320, 180, False), 'LilyGo_TWatchS3': (240, 240, 240, 140, True)}
if sys.argv[1] == '--has':
    sys.exit(0 if sys.argv[2] in DEVICES else 1)
from PIL import Image, ImageDraw
dev, out = sys.argv[1], Path(sys.argv[2])
env = sys.argv[3] if len(sys.argv) > 3 else '*'
fonts = sorted(glob.glob(str(ROOT / '.pio/libdeps' / env / 'Adafruit GFX Library/glcdfont.c'))) or \
        sorted(glob.glob(str(ROOT / '.pio/libdeps/*/Adafruit GFX Library/glcdfont.c')))
if not fonts: sys.exit('glcdfont.c nicht gefunden (erst bauen)')
src = open(fonts[0]).read()
F = [int(x, 16) for x in re.findall(r'0x[0-9A-Fa-f]{2}', src[src.index('{'):src.index('};')])]
LOGO = ROOT / 'logo/haldenfunk-display.png'
LGFX = sorted(glob.glob(str(ROOT / '.pio/libdeps/*/LovyanGFX/src/lgfx/Fonts')))

def _nocomment(t): return re.sub(r'//[^\n]*|/\*.*?\*/', '', t, flags=re.S)
def _nums(txt): return [int(x, 0) for x in re.findall(r'0x[0-9A-Fa-f]+|\b\d+\b', txt)]

class GFXFont:
    """Adafruit-GFX-Schrift aus LovyanGFX (Fonts/GFXFF/<name>.h), wie LGFX mit Datum top_left gezeichnet"""
    def __init__(self, name):
        t = _nocomment(open(f'{LGFX[0]}/GFXFF/{name}.h').read())
        self.bm = _nums(t[t.index('Bitmaps[]'):].split('{', 1)[1].split('};', 1)[0])
        gl = t[t.index('Glyphs[]'):].split('{', 1)[1].split('};', 1)[0]
        self.gl = [tuple(int(v) for v in re.findall(r'-?\d+', g)) for g in re.findall(r'\{([^}]*)\}', gl)]
        tail = t[t.index(f'GFXfont {name}'):]
        self.first, self.last, self.yadv = [int(v, 0) for v in re.findall(r'0x[0-9A-Fa-f]+|\b\d+\b', tail.split('Glyphs,', 1)[1])[:3]]
        self.ab = max(-g[5] for g in self.gl); self.bb = max(g[2] + g[5] for g in self.gl)
    def height(self): return self.ab + self.bb
    def width(self, s):
        w = 0
        for ch in s:
            g = self.gl[ord(ch) - self.first]; w += g[3]
        return w
    def draw(self, d, x, y, c, s):
        for ch in s:
            off, w, h, adv, xo, yo = self.gl[ord(ch) - self.first]
            bit = 0
            for yy in range(h):
                for xx in range(w):
                    b = self.bm[off + (bit >> 3)]
                    if b & (0x80 >> (bit & 7)): d.px_set(x + xo + xx, y + self.ab + yo + yy, c)
                    bit += 1
            x += adv

class BMPFont:
    """Font2 (TFT_eSPI Font16, 16 px) aus LovyanGFX"""
    def __init__(self):
        t = _nocomment(open(f'{LGFX[0]}/Font16.h').read())
        self.wid = _nums(t[t.index('widtbl_f16[96]'):].split('{', 1)[1].split('}', 1)[0])
        self.chr = {}
        for m in re.finditer(r'chr_f16_([0-9A-Fa-f]{2})\[\d+\]\s*=\s*\{([^}]*)\}', t):
            self.chr[int(m.group(1), 16)] = _nums(m.group(2))
    def height(self): return 16
    def width(self, s): return sum(self.wid[ord(ch) - 32] for ch in s)
    def draw(self, d, x, y, c, s):
        for ch in s:
            w = self.wid[ord(ch) - 32]; data = self.chr.get(ord(ch), []); bpr = (w + 6) >> 3
            for yy in range(16):
                for xx in range(w):
                    i = yy * bpr + (xx >> 3)
                    if i < len(data) and data[i] & (0x80 >> (xx & 7)): d.px_set(x + xx, y + yy, c)
            x += w

LGFX_FONTS = {}
def lgfx_font(size):
    if size not in LGFX_FONTS:
        LGFX_FONTS[size] = GFXFont({1: 'FreeSans9pt7b', 2: 'FreeSansBold12pt7b', 3: 'FreeSansBold18pt7b'}.get(size, 'FreeSansBold24pt7b'))
    return LGFX_FONTS[size]
def c565(v): r, g, b = (v >> 11) & 31, (v >> 5) & 63, v & 31; return (r * 255 // 31, g * 255 // 63, b * 255 // 31)
def q(p): r, g, b = p; return c565(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3))
BG, COPPER, GREEN, WHITE, GREY, ORANGE, RED = [c565(x) for x in (0x1082, 0xECAB, 0x4EF0, 0xFFFF, 0x8410, 0xFD20, 0xF800)]

class D:
    def __init__(s, W, H, lw, lh, touch):
        s.W, s.H, s.lw, s.lh, s.touch = W, H, lw, lh, touch
        s.im = Image.new('RGB', (W, H), BG); s.px = s.im.load(); s.dr = ImageDraw.Draw(s.im)
    def fill(s, x, y, w, h, c): s.dr.rectangle([x, y, x + w - 1, y + h - 1], fill=c)
    def rect(s, x, y, w, h, c): s.dr.rectangle([x, y, x + w - 1, y + h - 1], outline=c)
    def px_set(s, x, y, c):
        if 0 <= x < s.W and 0 <= y < s.H: s.px[x, y] = c
    def tw(s, t, sz): return lgfx_font(sz).width(t) if s.touch else 6 * sz * len(t)
    def th(s, sz): return lgfx_font(sz).height() if s.touch else 8 * sz
    def text(s, x, y, sz, c, t):
        if s.touch: return lgfx_font(sz).draw(s, x, y, c, t)   # T-Watch: LovyanGFX-Schriften

        for ch in t:
            for i in range(5):
                l = F[ord(ch) * 5 + i]
                for j in range(8):
                    if l & (1 << j):
                        s.dr.rectangle([x + i * sz, y + j * sz, x + i * sz + sz - 1, y + j * sz + sz - 1], fill=c)
            x += 6 * sz
    def ctext(s, cx, y, sz, c, t): s.text(cx - s.tw(t, sz) // 2, y, sz, c, t)
    def logo(s):
        sys.path.insert(0, str(ROOT / 'tools')); from gen_brand_logo import fit
        im = fit(Image.open(LOGO).convert('RGB'), s.lw, s.lh)
        b = im.tobytes(); px = bytes(v for i in range(0, len(b), 3) for v in q((b[i], b[i + 1], b[i + 2])))
        im = Image.frombytes('RGB', im.size, px)
        s.im.paste(im, ((s.W - s.lw) // 2, 2))

# Beispiel-Akkuwerte (Texte wie batt::texts in ui-brand/BattTrend.h, Varianten lang/mittel/kurz):
# (Prozent, links, [Verbrauch lang, mittel, kurz]) - keine echten Messwerte
EST = {'normal': ["Verbr. ca. 5,0 %/h, Rest ca. 13 h", "Verbr. ca. 5,0 %/h, Rest 13 h", "ca. 5,0 %/h, Rest 13 h"],
       'messung': ["Verbrauch: Messung, noch 12 min", "Verbrauch: noch 12 min", "Verbrauch: noch 12 min"],
       'leer': ["Verbr. ca. 6,3 %/h, Rest ca. 1,9 h", "Verbr. ca. 6,3 %/h, Rest 1,9 h", "ca. 6,3 %/h, Rest 1,9 h"]}
AKKU = {
  'LilyGo_TDeck': {'normal': (67, "ca. 67 % 3,93 V", EST['normal']), 'messung': (81, "ca. 81 % 4,03 V", EST['messung']),
                   'laden': (98, "ca. 98 % 4,18 V", ["Spannung steigt (Laden?)"] * 3), 'leer': (12, "ca. 12 % 3,70 V", EST['leer'])},
  'LilyGo_TWatchS3': {'normal': (71, "71 % 3,93 V", EST['normal']), 'messung': (84, "84 % 4,03 V", EST['messung']),
                      'laden': (88, "88 % 4,11 V", ["USB: Laden"] * 3), 'leer': (12, "12 % 3,70 V", EST['leer'])}}

def strip_h(d):
    lines = 2 if d.W <= 240 else 1
    return lines * d.th(1) + (lines - 1) + 2

def battery(d, akku):
    """wie UITask::drawBattery()"""
    pct, left, ests = akku
    W = d.W; small = W <= 240; m = 4
    y, h, th = 2 + d.lh + 2, strip_h(d), d.th(1)
    d.fill(0, y, W, h, BG)
    iw, ih = (22, 12) if small else (18, 8); iy = y + 1 + (th - ih) // 2
    d.rect(m, iy, iw, ih, WHITE); d.fill(m + iw, iy + ih // 4, 2, ih - 2 * (ih // 4), WHITE)
    c = GREEN if pct >= 40 else (ORANGE if pct >= 15 else RED)
    fw = max(1, (iw - 4) * pct // 100); d.fill(m + 2, iy + 2, fw, ih - 4, c)
    tx = m + iw + 2 + (6 if small else 4)
    d.text(tx, y + 1, 1, WHITE, left)
    avail = W - 2 * m if small else W - m - (tx + d.tw(left, 1) + 8)
    est = next((e for e in ests if d.tw(e, 1) <= avail), '')
    if est:
        ew = d.tw(est, 1)
        if small: d.text((W - ew) // 2, y + 1 + th + 1, 1, GREY, est)
        else: d.text(W - m - ew, y + 1, 1, GREY, est)

def render(d, mode, akku):
    """wie UITask::render() in examples/companion_radio/ui-brand/UITask.cpp"""
    W, H = d.W, d.H; small = W <= 240
    d.logo()
    battery(d, akku)
    bx = W // 20; bw = W - 2 * bx; by = 2 + d.lh + 2 + strip_h(d) + 2; bh = H - by - 5
    big = 2 if small else 3
    hs, hb = d.th(1), d.th(big)
    if mode in ('gesendet', 'warten'):
        c, t = (GREEN, "Advert gesendet") if mode == 'gesendet' else (ORANGE, "Bitte 7 s warten")
        d.fill(bx, by, bw, bh, c); d.ctext(W // 2, by + (bh - hb) // 2, big, BG, t)
    else:
        d.fill(bx, by, bw, bh, COPPER); d.rect(bx, by, bw, bh, WHITE)
        gap = max(1, (bh - hb - hs) // 3)
        d.ctext(W // 2, by + gap, big, BG, "Advert senden")
        d.ctext(W // 2, by + 2 * gap + hb, 1, BG, "antippen" if d.touch else "Trackball 2,5 s halten")

out.mkdir(parents=True, exist_ok=True)
A = AKKU[dev]
SHOTS = [('start', 'normal'), ('gesendet', 'normal'), ('warten', 'normal'),
         ('akku-messung', 'messung'), ('akku-laden', 'laden'), ('akku-fast-leer', 'leer')]
for i, (name, a) in enumerate(SHOTS, 1):
    m = name if not name.startswith('akku') else 'start'
    d = D(*DEVICES[dev]); render(d, m, A[a])
    d.im.resize((d.W * 2, d.H * 2), Image.NEAREST).save(out / f'{i}-{name}.png')
print('Vorschau:', out)
