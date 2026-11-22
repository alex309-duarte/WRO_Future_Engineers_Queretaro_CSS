"""Render the video of a twin run from its record (1920 x 1080).

The run itself is not filmed: the scene writes a record of it (car pose 25
times per second, signs, referee) into scene/results/run_*.json, so the
simulation keeps real time and the stopwatch is the robot's real time. This
tool puts the car back along that record and renders the views afterwards.

    python replay_video.py <run.json> <out.mp4> [--camera <dir>] [--title "text"] [--fps 30]

--camera: folder with the camera video saved by the robot program during the
run (processed_video.mp4, with the detection boxes) and frames.csv.
Runs on Windows next to CoppeliaSim (the scene of the run is loaded).
"""
import argparse
import bisect
import csv
import json
import math
import os
import time

import cv2
import numpy as np
from coppeliasim_zmqremoteapi_client import RemoteAPIClient

W, H = 1920, 1080
COLS = ['t', 'x', 'y', 'th', 'steer_deg', 'wheel_angle', 'speed', 't_run', 'laps', 'touched', 'walls',
        'surprise_ok', 'surprise_n', 'led']


def to_bgr(img, res):
    a = np.frombuffer(img, dtype=np.uint8).reshape(res[1], res[0], 3)
    return cv2.flip(cv2.cvtColor(a, cv2.COLOR_RGB2BGR), 0)


def put(img, text, org, scale=0.8, color=(255, 255, 255), thick=2):
    # dark copy of the text, shifted, as a shadow (same thickness: some OpenCV
    # builds space the letters by their weight, so a thicker outline would not
    # line up with the text)
    d = 2 if scale >= 0.6 else 1
    cv2.putText(img, text, (org[0] + d, org[1] + d), cv2.FONT_HERSHEY_SIMPLEX, scale, (0, 0, 0), thick, cv2.LINE_AA)
    cv2.putText(img, text, org, cv2.FONT_HERSHEY_SIMPLEX, scale, color, thick, cv2.LINE_AA)


def stopwatch(t):
    t = max(0.0, t)
    return '%d:%04.1f' % (int(t // 60), t % 60)


class MiniMap:
    """Top view of the field with the path driven so far (640 x 600)."""

    def __init__(self, sim, footprint):
        self.s = 0.17  # px per mm
        self.ox, self.oy = 65, 45
        self.base = np.full((600, 640, 3), 255, np.uint8)
        self.rect((-1500, 1500), (1500, -1500), (0, 0, 0), 3)
        self.rect((-500, 500), (500, -500), (60, 50, 40), -1)
        self.signs = {}
        for h in sim.getObjectsInTree(sim.getObject('/Signs'), sim.object_shape_type, 1):
            alias = sim.getObjectAlias(h)
            p = sim.getObjectPosition(h, sim.handle_world)
            self.signs[alias] = [h, p[0] * 1000, p[1] * 1000, p[2]]
        for h in sim.getObjectsInTree(sim.getObject('/Field'), sim.object_shape_type, 1):
            if sim.getObjectAlias(h).startswith('lotLimit'):
                p = sim.getObjectPosition(h, sim.handle_world)
                self.rect((p[0] * 1000 - 100, p[1] * 1000 + 10), (p[0] * 1000 + 100, p[1] * 1000 - 10),
                          (255, 0, 255), -1)
        self.path = []
        self.footprint = np.array(footprint) * 1000.0

    def px(self, x, y):
        return int(self.ox + (x + 1500) * self.s), int(self.oy + (1500 - y) * self.s)

    def rect(self, a, b, color, thick):
        cv2.rectangle(self.base, self.px(*a), self.px(*b), color, thick)

    def draw(self, x, y, th):
        self.path.append(self.px(x * 1000, y * 1000))
        img = self.base.copy()
        if len(self.path) > 1:
            cv2.polylines(img, [np.array(self.path, np.int32)], False, (210, 140, 30), 2, cv2.LINE_AA)
        for alias, (h, sx, sy, sz) in self.signs.items():
            col = (55, 39, 238) if 'red' in alias else (44, 214, 68)
            r = 25
            cv2.rectangle(img, self.px(sx - r, sy + r), self.px(sx + r, sy - r), col, -1)
            if alias.startswith('modified'):
                cv2.circle(img, self.px(sx, sy), 13, (0, 200, 255), 2, cv2.LINE_AA)
        c, s = math.cos(th), math.sin(th)
        pts = [self.px(x * 1000 + c * a - s * b, y * 1000 + s * a + c * b) for a, b in self.footprint]
        cv2.fillPoly(img, [np.array(pts, np.int32)], (90, 90, 90), cv2.LINE_AA)
        cv2.putText(img, 'Top view: path of the run (yellow ring = modified sign)', (12, 26),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 0), 1, cv2.LINE_AA)
        return img


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('run')
    ap.add_argument('out')
    ap.add_argument('--camera', default=None)
    ap.add_argument('--title', default=None)
    ap.add_argument('--fps', type=float, default=30.0)
    ap.add_argument('--lead', type=float, default=2.0, help='seconds shown before the start button')
    ap.add_argument('--tail', type=float, default=4.0, help='seconds shown after the last movement')
    ap.add_argument('--from', dest='t_from', type=float, default=None, help='first run second to render')
    ap.add_argument('--to', dest='t_to', type=float, default=None, help='last run second to render')
    ap.add_argument('--until-laps', type=float, default=None,
                    help='end the video (and stop the stopwatch) when this many laps are done, e.g. 2.95: '
                         'the car is back in the starting section after the third lap')
    args = ap.parse_args()

    run = json.load(open(args.run, encoding='utf-8'))
    rec = run['replay']
    times = [r[0] for r in rec]
    t0 = run['run_t0']
    # last movement
    last_move = times[-1]
    for r in reversed(rec):
        if abs(r[6]) > 0.01:
            last_move = r[0]
            break
    t_begin = max(times[0], t0 - args.lead)
    t_end = min(times[-1], last_move + args.tail)
    if args.t_from is not None:
        t_begin = max(times[0], t0 + args.t_from)
    if args.t_to is not None:
        t_end = min(times[-1], t0 + args.t_to)
    t_laps = None
    if args.until_laps is not None:
        for r in rec:
            if r[8] >= args.until_laps:
                t_laps = r[0]
                t_end = min(times[-1], t_laps + 2.5)
                break
    events = []
    for e in run.get('events', []):
        try:
            events.append((float(e[1:e.index('s]')]), e[e.index(']') + 2:]))
        except ValueError:
            pass
    sign_moves = run.get('replay_signs', [])

    cam_times, cap = [], None
    if args.camera and os.path.isfile(os.path.join(args.camera, 'processed_video.mp4')):
        cap = cv2.VideoCapture(os.path.join(args.camera, 'processed_video.mp4'))
        with open(os.path.join(args.camera, 'frames.csv')) as f:
            cam_times = [float(row[1]) for row in csv.reader(f) if len(row) == 2]
    cam_index, cam_frame = -1, np.zeros((480, 640, 3), np.uint8)

    client = RemoteAPIClient()
    sim = client.require('sim')
    if sim.getSimulationState() != sim.simulation_stopped:
        sim.stopSimulation()
        while sim.getSimulationState() != sim.simulation_stopped:
            time.sleep(0.1)
    sim.closeScene()
    sim.loadScene(run['scene'].replace('\\', '/'))
    sim.startSimulation()
    time.sleep(1.0)
    car = sim.getObject('/TheMaker')
    script = sim.getScript(sim.scripttype_simulation, car)
    with open(os.path.join(os.path.dirname(run['scene']), 'assets', 'footprint.json'), encoding='utf-8') as f:
        footprint = json.load(f)['points']
    sim.callScriptFunction('replay_pose', script, rec[0][1], rec[0][2], rec[0][3], 0, 0)
    mini = MiniMap(sim, footprint)

    writer = cv2.VideoWriter(args.out, cv2.VideoWriter_fourcc(*'mp4v'), args.fps, (W, H))
    title = args.title or run.get('layout', '')
    n_frames = int((t_end - t_begin) * args.fps)
    move_i = 0
    wall = time.time()
    for k in range(n_frames):
        t = t_begin + k / args.fps
        i = min(len(rec) - 1, max(1, bisect.bisect_left(times, t)))
        a, b = rec[i - 1], rec[i]
        u = 0.0 if b[0] == a[0] else min(1.0, max(0.0, (t - a[0]) / (b[0] - a[0])))
        lerp = lambda j: a[j] + (b[j] - a[j]) * u
        x, y, th, steer, wheel = lerp(1), lerp(2), lerp(3), lerp(4), lerp(5)
        moved = []
        while move_i < len(sign_moves) and sign_moves[move_i][0] <= t:
            _, alias, sx, sy = sign_moves[move_i]
            if alias in mini.signs:
                hs = mini.signs[alias]
                moved.append([hs[0], sx, sy, hs[3]])
                hs[1], hs[2] = sx * 1000, sy * 1000
            move_i += 1
        img_a, img_b = sim.callScriptFunction('replay_render', script, x, y, th, steer, wheel, moved)
        ov = to_bgr(img_a, (1280, 720))
        ch = to_bgr(img_b, (640, 360))
        if cap is not None and cam_times:
            want = bisect.bisect_right(cam_times, t) - 1
            while cam_index < want:
                ok, fr = cap.read()
                if not ok:
                    break
                cam_index += 1
                cam_frame = cv2.resize(fr, (640, 480))

        frame = np.full((H, W, 3), 30, np.uint8)
        frame[0:720, 0:1280] = ov
        frame[720:1080, 0:640] = ch
        frame[0:480, 1280:1920] = cam_frame
        frame[480:1080, 1280:1920] = mini.draw(x, y, th)
        panel = frame[720:1080, 640:1280]
        panel[:] = (28, 32, 40)
        t_run = t - t0
        t_stop = (run['result_time'] - t0) if run.get('result_time') else None
        if t_laps is not None:
            t_stop = t_laps - t0
        shown = t_run if (t_stop is None or t_run < t_stop) else t_stop
        put(panel, 'WRO 2026 Future Engineers - Obstacle Challenge', (16, 34), 0.62, (255, 255, 255), 1)
        put(panel, "Digital twin of 'The Maker' (team CSS, Mexico)", (16, 62), 0.58, (200, 220, 255), 1)
        put(panel, stopwatch(shown), (16, 150), 2.6, (80, 255, 255) if t_run >= 0 else (150, 150, 150), 5)
        put(panel, 'laps %.2f' % b[8], (360, 120), 0.9)
        put(panel, 'speed %.2f m/s' % abs(b[6]), (360, 152), 0.6, (220, 220, 220), 1)
        put(panel, 'Signs moved %d   wall contacts %d' % (b[9], b[10]), (16, 200), 0.7,
            (120, 255, 120) if b[9] == 0 else (80, 80, 255), 2)
        if b[12]:
            put(panel, 'Surprise rule: %d of %d passages correct' % (b[11], b[12]), (16, 236), 0.7,
                (120, 255, 120) if b[11] == b[12] else (80, 80, 255), 2)
        recent = [txt for (te, txt) in events if te <= min(t_run, shown + 0.01)][-3:]
        for j, txt in enumerate(recent):
            put(panel, txt[:66], (16, 272 + 24 * j), 0.45, (255, 230, 150), 1)
        if t_laps is not None and t_run >= t_stop:
            put(panel, 'THREE LAPS DONE', (16, 348), 0.8, (80, 255, 255), 2)
        elif t_stop is not None and t_run >= t_stop and run.get('result'):
            put(panel, run['result'], (16, 348), 0.8, (80, 255, 255), 2)
        put(frame, title[:95], (14, 30), 0.7, (255, 255, 255), 2)
        put(frame, 'Robot camera (frames the program receives, with its detections)', (1290, 24), 0.48,
            (255, 255, 255), 1)
        writer.write(frame)
        if k % 300 == 0:
            print('frame %d of %d (%.0f s)' % (k, n_frames, time.time() - wall), flush=True)
    writer.release()
    sim.stopSimulation()
    print('written', args.out, n_frames, 'frames in %.0f s' % (time.time() - wall))


if __name__ == '__main__':
    main()
