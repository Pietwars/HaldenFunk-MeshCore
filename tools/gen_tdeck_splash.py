#!/usr/bin/env python3
"""Encode the HaldenFunk artwork as native RGB565 (little endian)."""
from pathlib import Path
import struct
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / 'examples/companion_radio/ui-brand/assets'

def encode(rgb):
    # Python integers: uint8 shifts would discard the red and upper green bits.
    return b''.join(struct.pack('<H', ((int(r) >> 3) << 11) |
                               ((int(g) >> 2) << 5) | (int(b) >> 3)) for r, g, b in rgb)

def main():
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    assert encode([(255, 0, 0), (0, 255, 0), (0, 0, 255), (255,255,255)]) == bytes.fromhex('00f8e0071f00ffff')
    with Image.open(ROOT / 'logo/haldenfunk-display.png') as src:
        image = src.convert('RGB').resize((320,213), Image.Resampling.LANCZOS)
        data = encode(image.getpixel((x,y)) for y in range(213) for x in range(320))
    assert len(data) == 136320 and any(data[1::2])
    target = ASSETS / 'tdeck-splash.rgb565'
    if args.check:
        assert target.read_bytes() == data, 'Boot asset differs; run tools/gen_tdeck_splash.py'
    else:
        target.write_bytes(data)
    print('Bootmotiv: RGB565, 320 x 213, complete colour channels OK')

if __name__ == '__main__':
    main()
