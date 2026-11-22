"""Check the twin without the robot program: starts the open scene, saves the
overview, the robot camera and one LiDAR revolution as images, prints the
board state, and stops the simulation.

    python snapshot.py [output_dir]
"""
import math
import os
import struct
import sys
import time

import cv2
import numpy as np
import zmq
from coppeliasim_zmqremoteapi_client import RemoteAPIClient

out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(os.path.abspath(__file__)), 'snap')
os.makedirs(out, exist_ok=True)
client = RemoteAPIClient()
sim = client.require('sim')
if sim.getSimulationState() == sim.simulation_stopped:
    sim.startSimulation()
time.sleep(1.5)


def grab(path, name):
    h = sim.getObject(path)
    sim.handleVisionSensor(h)
    img, res = sim.getVisionSensorImg(h)
    a = np.frombuffer(img, dtype=np.uint8).reshape(res[1], res[0], 3)
    a = cv2.flip(cv2.cvtColor(a, cv2.COLOR_RGB2BGR), 0)
    cv2.imwrite(os.path.join(out, name), a)
    print('saved', name, res)


grab('/recOverview', 'overview.png')
ctx = zmq.Context()


def req(port, msg, timeout=2000):
    s = ctx.socket(zmq.REQ)
    s.setsockopt(zmq.LINGER, 0)
    s.connect(f'tcp://127.0.0.1:{port}')
    s.send(msg.encode())
    if s.poll(timeout):
        r = s.recv()
    else:
        r = None
    s.close()
    return r


print('board:', req(23101, 'st'))
r = req(23103, 'img')
w, h = struct.unpack('<II', r[:8])
a = np.frombuffer(r[8:], dtype=np.uint8).reshape(h, w, 3)
cv2.imwrite(os.path.join(out, 'robot_camera.png'), cv2.flip(cv2.cvtColor(a, cv2.COLOR_RGB2BGR), 0))
print('camera', w, h)
time.sleep(0.3)
r = req(23102, 'scan 0')
seq, n = struct.unpack('<II', r[:8])
pts = np.frombuffer(r[8:8 + 8 * n], dtype=np.float32).reshape(n, 2)
print('lidar seq', seq, 'points', n, 'valid', int((pts[:, 1] > 0).sum()))
canvas = np.full((800, 800, 3), 255, np.uint8)
cv2.circle(canvas, (400, 400), 4, (0, 0, 255), -1)
cv2.line(canvas, (400, 400), (400, 360), (0, 0, 255), 2)   # robot front = up
for raw, mm in pts:
    if mm <= 0:
        continue
    phi = math.radians(raw - 90.0)          # clockwise from the robot front
    x = 400 + mm * 0.12 * math.sin(phi)      # right = +x on screen
    y = 400 - mm * 0.12 * math.cos(phi)      # front = up
    cv2.circle(canvas, (int(x), int(y)), 2, (0, 0, 0), -1)
cv2.imwrite(os.path.join(out, 'lidar.png'), canvas)
for i in (0, 90, 180, 270):
    k = int(np.argmin(np.abs(((pts[:, 0] - 90.0 - i + 180) % 360) - 180)))
    print(f'robot angle {i:3d} (cw from front): {pts[k, 1]:.0f} mm')
if len(sys.argv) < 3:
    sim.stopSimulation()
