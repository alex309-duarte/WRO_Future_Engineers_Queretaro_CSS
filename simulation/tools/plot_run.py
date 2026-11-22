"""Draw the path of a twin run over the field.

    python plot_run.py scene/results/run_<date>.json scene/layouts/layout_A.json [out.png]

The result file holds the referee's record of the run (rear-axle position
every 0.1 s, contacts, laps, parking); the layout file gives the signs.
"""
import json
import sys

import cv2
import numpy as np

S = 0.25  # px per mm
PAD = 40


def to_px(x_mm_sim, y_mm_sim):
    # scene mm (origin at the centre, Y north) -> image px (north up)
    return int(PAD + (x_mm_sim + 1500) * S), int(PAD + (1500 - y_mm_sim) * S)


def main():
    run = json.load(open(sys.argv[1], encoding='utf-8'))
    layout = json.load(open(sys.argv[2], encoding='utf-8'))
    out = sys.argv[3] if len(sys.argv) > 3 else sys.argv[1].replace('.json', '.png')
    size = int(3000 * S + 2 * PAD)
    img = np.full((size, size + 260, 3), 255, np.uint8)
    cv2.rectangle(img, to_px(-1500, 1500), to_px(1500, -1500), (0, 0, 0), 3)
    cv2.rectangle(img, to_px(-500, 500), to_px(500, -500), (0, 0, 0), -1)
    p = layout['parking']
    y0, y1 = 1500 - p['from_mm'], 1500 - p['from_mm'] - p['length_mm']
    cv2.rectangle(img, to_px(1300, y0), to_px(1500, y1), (255, 0, 255), 2)
    for s in layout['pillars']:
        x, y = s['x_mm'] - 1500, 1500 - s['y_mm']
        col = (55, 39, 238) if s['color'] == 'red' else (44, 214, 68)
        cv2.rectangle(img, to_px(x - 25, y + 25), to_px(x + 25, y - 25), col, -1)
    tr = run.get('trace_t_x_y_mm', [])
    pts = [to_px(x, y) for _, x, y in tr]
    for a, b in zip(pts, pts[1:]):
        cv2.line(img, a, b, (200, 120, 0), 2)
    if pts:
        cv2.circle(img, pts[0], 6, (0, 160, 0), -1)
        cv2.circle(img, pts[-1], 6, (0, 0, 200), -1)
    x0 = size + 10
    lines = [run.get('layout', ''), run.get('direction', ''), 'laps %s' % run.get('laps'),
             'time %s s' % run.get('time_s', '-'), run.get('result', '') or '(no result)',
             'signs touched %d' % len(run.get('signs_touched', [])),
             'wall contacts %s' % run.get('wall_contacts')]
    for i, t in enumerate(lines):
        cv2.putText(img, str(t), (x0, 40 + 28 * i), cv2.FONT_HERSHEY_SIMPLEX, 0.55, (0, 0, 0), 1, cv2.LINE_AA)
    for i, e in enumerate(run.get('events', [])[:20]):
        cv2.putText(img, e, (x0, 260 + 20 * i), cv2.FONT_HERSHEY_SIMPLEX, 0.4, (60, 60, 60), 1, cv2.LINE_AA)
    cv2.imwrite(out, img)
    print(out)


if __name__ == '__main__':
    main()
