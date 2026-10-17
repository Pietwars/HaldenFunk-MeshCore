#!/usr/bin/env python3
"""Einzige Versionsquelle: [hf] version in platformio.local.ini. Aufruf: tools/version.py -> 0.1.1-alpha"""
import re
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]


def version():
    m = re.search(r'^\[hf\]\s*\n(?:[#;].*\n)*version\s*=\s*(\S+)', (ROOT / 'platformio.local.ini').read_text(encoding='utf-8'), re.M)
    if not m:
        raise SystemExit('Version nicht gefunden: [hf] version in platformio.local.ini')
    return m.group(1)


if __name__ == '__main__':
    print(version())
