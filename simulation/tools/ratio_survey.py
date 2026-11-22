"""Surprise rule: measure the height-to-base ratio of the sign boxes in the
twin camera, for the three sign heights (50, 100, 150 mm), both colours, at
several distances and side offsets. The limits of the classifier in
bridge/surprise_rule.cpp come from these numbers (the plan in README 6.10:
"the limits between the three classes will be set from frames taken at
different distances and angles").

Run with any twin scene open (the simulation is started and stopped here):
    python ratio_survey.py [out.csv]
"""
import csv
import math
import struct
import sys
import time

import cv2
import numpy as np
import zmq
from coppeliasim_zmqremoteapi_client import RemoteAPIClient

out = sys.argv[1] if len(sys.argv) > 1 else 'ratio_survey.csv'
sim = RemoteAPIClient().require('sim')
if sim.getSimulationState() == sim.simulation_stopped:
    sim.startSimulation()
time.sleep(1.0)
ctx = zmq.Context()
board = ctx.socket(zmq.REQ); board.connect('tcp://127.0.0.1:23101')
cam = ctx.socket(zmq.REQ); cam.connect('tcp://127.0.0.1:23103')


def cmd(m):
    board.send(m.encode()); return board.recv().decode()


def grab():
    cam.send(b'img'); r = cam.recv()
    w, h = struct.unpack('<II', r[:8])
    a = np.frombuffer(r[8:], np.uint8).reshape(h, w, 3)
    return cv2.flip(cv2.cvtColor(a, cv2.COLOR_RGB2BGR), 0)


def boxes(bgr):
    # same thresholds as bridge/sim_hailo.cpp
    hsv = cv2.cvtColor(bgr, cv2.COLOR_BGR2HSV)
    red = cv2.inRange(hsv, (0, 120, 60), (6, 255, 255)) | cv2.inRange(hsv, (168, 120, 60), (180, 255, 255))
    green = cv2.inRange(hsv, (40, 100, 50), (80, 255, 255))
    out = []
    for name, m in (('red', red), ('green', green)):
        m = cv2.morphologyEx(m, cv2.MORPH_OPEN, np.ones((3, 3), np.uint8))
        n, _, st, _ = cv2.connectedComponentsWithStats(m, 8)
        for i in range(1, n):
            x, y, w, h, a = st[i]
            if a >= 0.00025 * bgr.shape[0] * bgr.shape[1]:
                out.append((name, x, y, w, h, a))
    return out


def sign_height(y_top, y_bottom, cam_h=164.0, tilt_deg=10.0, hfov_deg=86.0, w=800, h=600):
    """Same estimate as bridge/surprise_rule.cpp (Sign_Height_Mm)."""
    f = (w / 2) / math.tan(math.radians(hfov_deg) / 2)
    bb = math.radians(tilt_deg) + math.atan((y_bottom - h / 2) / f)
    bt = math.radians(tilt_deg) + math.atan((y_top - h / 2) / f)
    if bb <= 0.02:
        return 0.0
    d = cam_h / math.tan(bb)
    return cam_h - (d + 60.0) * math.tan(bt)


RED = [238 / 255, 39 / 255, 55 / 255]
GREEN = [68 / 255, 214 / 255, 44 / 255]
SX, SY = 0.6, 1.0          # sign seat used for the survey (north corridor, free in every layout)
signs = {}
for colour in ('red', 'green'):
    for hmm in (50, 100, 150):
        h = sim.createPrimitiveShape(sim.primitiveshape_cuboid, [0.05, 0.05, hmm / 1000.0], 0)
        sim.setShapeColor(h, None, sim.colorcomponent_ambient_diffuse, RED if colour == 'red' else GREEN)
        sim.setObjectPosition(h, [SX, SY, -1.0], sim.handle_world)
        signs[(colour, hmm)] = h

rows = []
try:
    for (colour, hmm), h in signs.items():
        sim.setObjectPosition(h, [SX, SY, hmm / 2000.0], sim.handle_world)
        for d in (0.25, 0.35, 0.45, 0.6, 0.75, 0.9, 1.1, 1.3):
            for lat in (-0.25, -0.12, 0.0, 0.12, 0.25):
                for yaw in (-15, 0, 15):
                    # camera lens is 19.5 mm ahead of the rear axle
                    th = math.radians(yaw)
                    cmd('pose %.4f %.4f %.1f' % (SX - d - 0.0195, SY + lat, yaw))
                    time.sleep(0.03)
                    img = grab()
                    bs = [b for b in boxes(img) if b[0] == colour]
                    if not bs:
                        continue
                    name, x, y, w, hh, a = max(bs, key=lambda b: b[5])
                    cut = x <= 1 or y <= 1 or x + w >= img.shape[1] - 1 or y + hh >= img.shape[0] - 1
                    rows.append([colour, hmm, d, lat, yaw, w, hh, round(hh / w, 3), int(cut), x, y,
                                 round(sign_height(y, y + hh), 1)])
        sim.setObjectPosition(h, [SX, SY, -1.0], sim.handle_world)
finally:
    for h in signs.values():
        sim.removeObjects([h])
    sim.stopSimulation()

with open(out, 'w', newline='') as f:
    wr = csv.writer(f)
    wr.writerow(['colour', 'height_mm', 'distance_m', 'lateral_m', 'yaw_deg', 'box_w_px', 'box_h_px', 'ratio', 'cut',
                 'box_x_px', 'box_y_px', 'est_height_mm'])
    wr.writerows(rows)
print('rows', len(rows), '->', out)
def voting(row):   # the frames that vote in surprise_rule.cpp
    return not row[8] and 14 <= row[5] <= 62 and (row[10] + row[6]) / 600.0 <= 0.76


for label, col, div in (('box height / box width', 7, 1.0), ('sign height / 50 mm base', 11, 50.0)):
    print(label)
    for hmm in (50, 100, 150):
        r = [row[col] / div for row in rows if row[1] == hmm and voting(row)]
        if r:
            print('  %3d mm (n=%d): min %.2f  p5 %.2f  median %.2f  p95 %.2f  max %.2f' % (
                hmm, len(r), min(r), np.percentile(r, 5), np.median(r), np.percentile(r, 95), max(r)))
v = [row for row in rows if voting(row)]
ok = sum((50 if row[11] / 50 < 1.5 else (100 if row[11] / 50 < 2.5 else 150)) == row[1] for row in v)
print('limits 1.5 / 2.5 on sign height / base: %d of %d voting frames correct (%.1f %%)' % (ok, len(v), 100.0 * ok / len(v)))
