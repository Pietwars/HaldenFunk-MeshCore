#!/usr/bin/env python3
"""HaldenFunk: verkleinert die Offline-Karten (web/.../map/*.json.gz) fuer knappen Flash.
Douglas-Peucker-Vereinfachung, Raster-Rundung, Zuschnitt auf die Kartengrenzen, kleine Flaechen/Kurzstrecken weg.
Format bleibt kompatibel (Koordinaten in 1e-5 Grad, Delta-kodiert); Orte, Halden, Schaechte bleiben vollstaendig."""
import json, gzip, sys, math, zlib
from pathlib import Path

# Parameter je Karte: tol = DP-Toleranz (1e-5 Grad, ~1,1 m Breite), grid = Rundungsraster, minA = Mindestflaeche (m^2) Flaechen, minL = Mindestlaenge (m) Linien
PROFILE = {
 'mansfelder-land': dict(tol=22, grid=6, minA={'wd': 15000, 'wa': 2000, 'v': 0}, minL={'s': 300, 'h': 0, 'w': 0, 'rw': 0}, drop=()),
 '_detail':         dict(tol=5, grid=2, minA={'wd': 1500, 'wa': 300, 'v': 0}, minL={'f': 25, 'r': 0, 's': 60}, drop=()),
}
FILL = {'wd', 'v', 'wa', 'b'}

def dp(pts, tol, k):
    if len(pts) < 3: return pts
    keep = [False] * len(pts); keep[0] = keep[-1] = True; st = [(0, len(pts) - 1)]
    while st:
        a, b = st.pop(); ax, ay = pts[a][0] * k, pts[a][1]; bx, by = pts[b][0] * k, pts[b][1]
        dx, dy = bx - ax, by - ay; L = math.hypot(dx, dy); best, bi = -1, -1
        for i in range(a + 1, b):
            px, py = pts[i][0] * k - ax, pts[i][1] - ay
            d = abs(dx * py - dy * px) / L if L else math.hypot(px, py)
            if d > best: best, bi = d, i
        if best > tol: keep[bi] = True; st += [(a, bi), (bi, b)]
    return [p for p, f in zip(pts, keep) if f]

def clip_runs(pts, bb):  # Linie in Abschnitte innerhalb der (leicht erweiterten) bbox zerlegen
    runs, cur = [], []
    inside = lambda p: bb[0] <= p[0] <= bb[2] and bb[1] <= p[1] <= bb[3]
    for i, p in enumerate(pts):
        nb = (i > 0 and inside(pts[i - 1])) or (i + 1 < len(pts) and inside(pts[i + 1]))
        if inside(p) or nb: cur.append(p)
        else:
            if len(cur) > 1: runs.append(cur)
            cur = []
    if len(cur) > 1: runs.append(cur)
    return runs

def shrink(d, prof):
    q = d.get('q', 1)
    tol, g = prof['tol'], prof['grid']
    b = d['bbox']; m = 0.01; bb = [(b[0] - m) * 1e5, (b[1] - m) * 1e5, (b[2] + m) * 1e5, (b[3] + m) * 1e5]
    k = math.cos(math.radians((b[1] + b[3]) / 2)); mx = 1.113 * k; my = 1.113  # Meter je Einheit
    L = []
    for l in d['l']:
        t = l[0]
        if t in prof['drop']: continue
        x, y = l[1] * q, l[2] * q; pts = [(x, y)]
        for i in range(3, len(l), 2): x += l[i] * q; y += l[i + 1] * q; pts.append((x, y))
        if t in FILL:
            if not any(bb[0] <= p[0] <= bb[2] and bb[1] <= p[1] <= bb[3] for p in pts): continue
            a = abs(sum(pts[i][0] * pts[i - 1][1] - pts[i - 1][0] * pts[i][1] for i in range(len(pts)))) / 2 * mx * my
            if a < prof['minA'].get(t, 0): continue
            runs = [pts]
        else:
            runs = clip_runs(pts, bb)
        for r in runs:
            r = dp(r, tol / 4 if t == 'v' else tol, k)  # Halden feiner
            gg = 1 if t == 'v' else g
            r = [(round(p[0] / gg) * gg, round(p[1] / gg) * gg) for p in r]
            r = [p for i, p in enumerate(r) if i == 0 or p != r[i - 1]]
            if len(r) < 2 or (t in FILL and len(r) < 3): continue
            if t not in FILL:
                ln = sum(math.hypot((r[i][0] - r[i - 1][0]) * mx, (r[i][1] - r[i - 1][1]) * my) for i in range(1, len(r)))
                if ln < prof['minL'].get(t, 0): continue
            row = [t, r[0][0], r[0][1]]
            for i in range(1, len(r)): row += [r[i][0] - r[i - 1][0], r[i][1] - r[i - 1][1]]
            L.append(row)
    d = dict(d); d.pop('q', None); d['l'] = L
    return d

# Aufruf: tools/shrink_maps.py <Ordner mit Rohkarten> <Zielordner, z. B. web/map>
if __name__ == "__main__":
    src, dst = Path(sys.argv[1]), Path(sys.argv[2]); dst.mkdir(parents=True, exist_ok=True)
    for f in sorted(src.glob('*.json.gz')):
        key = f.name.split('.')[0]; d = json.load(gzip.open(f))
        out = shrink(d, PROFILE.get(key, PROFILE['_detail']))
        raw = json.dumps(out, ensure_ascii=False, separators=(',', ':')).encode()
        z = bytearray(gzip.compress(raw, 9, mtime=0)); z[9] = 255
        (dst / f.name).write_bytes(z)
        print('%-22s %7d -> %7d Bytes (%d -> %d Linien)' % (f.name, f.stat().st_size, len(z), len(d['l']), len(out['l'])))
