"""Build the CoppeliaSim scene of the WRO 2026 Obstacle Challenge digital twin.

Runs on the PC where CoppeliaSim 4.10 is open (ZeroMQ remote API, port 23000):

    python build_scene.py --layout A --direction cw
    python build_scene.py --layout random --seed 7 --direction ccw

It builds, from nothing, the 3 x 3 m field of the WRO 2026 Future Engineers
Obstacle Challenge (mat, 100 mm walls, traffic signs, magenta parking limits)
and the car "The Maker" (design v7) from its SolidWorks assembly, with the
script that emulates its board, LiDAR, camera and start button
(twin_car.lua). The car starts inside the parking lot, as in the rules; the
direction (cw / ccw) is the way it faces, which the program detects by itself.
The scene is saved next to this file as wro_twin_<layout>_<direction>.ttt.

Field coordinates in the layout files are millimetres on the 3000 mm track,
origin at the north-west inner corner of the outer wall, x to the east, y to
the south (as in the team's drawings in other/testing/). In the scene the
origin is the centre of the mat, X east, Y north.
"""
import argparse
import json
import math
import os
import random
import time

from coppeliasim_zmqremoteapi_client import RemoteAPIClient

HERE = os.path.dirname(os.path.abspath(__file__))
ASSETS = os.path.join(HERE, 'assets')

RED = [238 / 255, 39 / 255, 55 / 255]
GREEN = [68 / 255, 214 / 255, 44 / 255]
MAGENTA = [1.0, 0.0, 1.0]
BLACK = [0.06, 0.06, 0.06]

# Seats of the traffic signs (mm): 3 positions along each straight section,
# on two lanes 400 and 600 mm from the outer wall.
SEATS = {
    'north': [(x, y) for x in (1000, 1500, 2000) for y in (400, 600)],
    'south': [(x, y) for x in (1000, 1500, 2000) for y in (2400, 2600)],
    'west': [(x, y) for y in (1000, 1500, 2000) for x in (400, 600)],
    'east': [(x, y) for y in (1000, 1500, 2000) for x in (2400, 2600)],
}

# Car geometry (robot frame: origin on the floor under the rear axle, X forward,
# Y left, Z up; metres). From models/cad/v7_assembly.
WHEEL_REAR = (0.0, 0.0637, 0.0313)
WHEEL_FRONT = (0.1638, 0.0637, 0.0313)
LIDAR_FRAME = (0.0664, 0.0, 0.072)      # RPLidar S2L optical centre, ~75 mm above the floor
CAMERA_POS = (0.0195, 0.0, 0.164)        # Camera Module 3 lens, top of the mast
CAMERA_TILT_DEG = 10.0                   # pitched down (CAD)
CAMERA_HFOV_DEG = 86.0                   # Wide module, 800 x 600 (4:3) crop of the 102 deg sensor
BODY_X = (-0.040, 0.2035)                # body length 243.5 mm
BODY_HALF_WIDTH = 0.0789                 # 157.7 mm


def mm_to_sim(x, y):
    return (x - 1500.0) / 1000.0, (1500.0 - y) / 1000.0


def random_layout(seed):
    rng = random.Random(seed)
    pillars = []
    single = rng.choice(list(SEATS))
    for section, seats in SEATS.items():
        n = 1 if section == single else rng.choice([1, 2, 2])
        if section == 'east':
            # starting section: signs moved to the inner lane (rules)
            seats = [s for s in seats if s[0] == 2400]
        if n == 1:
            chosen = [rng.choice(seats)]
        else:
            # two signs at the two ends of the section, any lane
            ends = sorted({s[0] if section in ('north', 'south') else s[1] for s in seats})
            key = (lambda s: s[0]) if section in ('north', 'south') else (lambda s: s[1])
            chosen = [rng.choice([s for s in seats if key(s) == ends[0]]),
                      rng.choice([s for s in seats if key(s) == ends[-1]])]
        for s in chosen:
            pillars.append({'color': rng.choice(['red', 'green']), 'x_mm': s[0], 'y_mm': s[1]})
    return {'name': f'Random layout (seed {seed})', 'parking': {'section': 'east', 'from_mm': 1000,
            'length_mm': 358}, 'pillars': pillars}


def load_layout(name, seed):
    if name == 'random':
        return random_layout(seed)
    path = name if os.path.isfile(name) else os.path.join(HERE, 'layouts', f'layout_{name.upper()}.json')
    with open(path, encoding='utf-8') as f:
        return json.load(f)


class Builder:
    def __init__(self):
        self.client = RemoteAPIClient()
        self.sim = self.client.require('sim')
        self.assimp = self.client.require('simAssimp')

    # ------------------------------------------------------------ helpers
    def cuboid(self, alias, size, pos, color, parent):
        sim = self.sim
        h = sim.createPrimitiveShape(sim.primitiveshape_cuboid, list(size), 0)
        sim.setObjectAlias(h, alias)
        sim.setObjectPosition(h, list(pos), sim.handle_world)
        sim.setShapeColor(h, None, sim.colorcomponent_ambient_diffuse, color)
        self.make_static(h)
        sim.setObjectParent(h, parent, True)
        return h

    def make_static(self, h, collidable=True):
        sim = self.sim
        for name, value in (('shapeintparam_static', 1), ('shapeintparam_respondable', 0)):
            try:
                sim.setObjectInt32Param(h, getattr(sim, name), value)
            except Exception:
                pass
        props = sim.objectspecialproperty_detectable | sim.objectspecialproperty_measurable
        if collidable:
            props |= sim.objectspecialproperty_collidable
        try:
            sim.setObjectSpecialProperty(h, props)
        except Exception:
            pass

    @staticmethod
    def footprint():
        with open(os.path.join(ASSETS, 'footprint.json'), encoding='utf-8') as f:
            return json.load(f)['points']

    def dummy(self, alias, pos, parent=None, size=0.01):
        sim = self.sim
        h = sim.createDummy(size)
        sim.setObjectAlias(h, alias)
        sim.setObjectPosition(h, list(pos), parent if parent is not None else sim.handle_world)
        if parent is not None:
            sim.setObjectParent(h, parent, True)
        return h

    def import_mesh(self, filename, alias, parent, collidable=False):
        sim = self.sim
        path = os.path.join(ASSETS, filename).replace('\\', '/')
        handles = self.assimp.importShapes(path, 512, 1.0, self.assimp.upVector.z, 64)
        h = handles[0] if len(handles) == 1 else sim.groupShapes(handles, False)
        # vertices are in the robot frame: put the shape frame on that frame
        try:
            sim.relocateShapeFrame(h, [0, 0, 0, 0, 0, 0, 1])
        except Exception:
            pass
        sim.setObjectAlias(h, alias)
        self.make_static(h, collidable)
        sim.setObjectParent(h, parent, False)
        return h

    def set_lookat(self, h, eye, target, up=(0, 0, 1)):
        """Vision sensors and cameras look along their +Z axis with +Y up."""
        def norm(v):
            n = math.sqrt(sum(c * c for c in v))
            return [c / n for c in v]

        def cross(a, b):
            return [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]]

        z = norm([target[i] - eye[i] for i in range(3)])
        d = sum(up[i] * z[i] for i in range(3))
        y = norm([up[i] - d * z[i] for i in range(3)])
        x = cross(y, z)
        m = [x[0], y[0], z[0], eye[0], x[1], y[1], z[1], eye[1], x[2], y[2], z[2], eye[2]]
        self.sim.setObjectMatrix(h, m, self.sim.handle_world)

    # ------------------------------------------------------------ scene
    def new_scene(self):
        sim = self.sim
        try:
            sim.stopSimulation()
            while sim.getSimulationState() != sim.simulation_stopped:
                time.sleep(0.1)
        except Exception:
            pass
        sim.closeScene()
        for path in ('/Floor', '/ResizableFloor_5_25'):
            try:
                sim.removeModel(sim.getObject(path))
            except Exception:
                pass
        # 10 ms steps in real time: the program runs on the wall clock, as on the robot
        for setter in (lambda: sim.setFloatParam(sim.floatparam_simulation_time_step, 0.01),
                       lambda: sim.setFloatProperty(sim.handle_scene, 'timeStep', 0.01)):
            try:
                setter()
                break
            except Exception:
                pass
        for setter in (lambda: sim.setBoolParam(sim.boolparam_realtime_simulation, True),
                       lambda: sim.setBoolProperty(sim.handle_scene, 'realtimeSimulation', True)):
            try:
                setter()
                break
            except Exception:
                pass

    def build_field(self, layout):
        sim = self.sim
        field = self.dummy('Field', (0, 0, 0))
        tex = os.path.join(ASSETS, 'mat_obstacle.png').replace('\\', '/')
        mat = sim.createTexture(tex, 0, [3.2, 3.2])[0]
        sim.setObjectAlias(mat, 'mat')
        sim.setObjectPosition(mat, [0, 0, 0.0005], sim.handle_world)
        self.make_static(mat, collidable=False)
        sim.setObjectParent(mat, field, True)
        t, h = 0.02, 0.1
        # outer walls, inner faces at +-1.5 m
        for alias, size, pos in (('wallN', (3.04, t, h), (0, 1.5 + t / 2, h / 2)),
                                 ('wallS', (3.04, t, h), (0, -1.5 - t / 2, h / 2)),
                                 ('wallE', (t, 3.0, h), (1.5 + t / 2, 0, h / 2)),
                                 ('wallW', (t, 3.0, h), (-1.5 - t / 2, 0, h / 2)),
                                 # inner walls (1000 mm corridor): outer faces at +-0.5 m
                                 ('wallInN', (1.0, t, h), (0, 0.5 - t / 2, h / 2)),
                                 ('wallInS', (1.0, t, h), (0, -0.5 + t / 2, h / 2)),
                                 ('wallInE', (t, 1.0 - 2 * t, h), (0.5 - t / 2, 0, h / 2)),
                                 ('wallInW', (t, 1.0 - 2 * t, h), (-0.5 + t / 2, 0, h / 2))):
            self.cuboid(alias, size, pos, BLACK, field)
        # parking lot (east section, against the outer wall): two magenta
        # limits 200 x 20 x 100 mm, lot length 1.5 x the robot length
        p = layout['parking']
        y0_mm, y1_mm = p['from_mm'], p['from_mm'] + p['length_mm']
        xs, _ = mm_to_sim(2900, 0)
        _, ya = mm_to_sim(0, y0_mm - 10)
        _, yb = mm_to_sim(0, y1_mm + 10)
        self.cuboid('lotLimit1', (0.2, 0.02, 0.1), (xs, ya, 0.05), MAGENTA, field)
        self.cuboid('lotLimit2', (0.2, 0.02, 0.1), (xs, yb, 0.05), MAGENTA, field)
        lot = {'x0': mm_to_sim(2800, 0)[0], 'x1': mm_to_sim(3000, 0)[0],
               'y0': mm_to_sim(0, y1_mm)[1], 'y1': mm_to_sim(0, y0_mm)[1]}
        signs = self.dummy('Signs', (0, 0, 0))
        count = {'red': 0, 'green': 0}
        self.sign_info = []
        for s in layout['pillars']:
            count[s['color']] += 1
            x, y = mm_to_sim(s['x_mm'], s['y_mm'])
            hgt = s.get('height_mm', 100) / 1000.0
            original = s.get('original', s['color'])
            alias = f"sign_{s['color']}_{count[s['color']]}"
            if hgt != 0.1:
                alias = f"modified_{s['color']}_{int(round(hgt * 1000))}mm"
            self.cuboid(alias, (0.05, 0.05, hgt), (x, y, hgt / 2),
                        RED if s['color'] == 'red' else GREEN, signs)
            self.sign_info.append((alias, x, y, original, int(round(hgt * 1000))))
        return lot

    def build_car(self, start):
        sim = self.sim
        car = self.dummy('TheMaker', (0, 0, 0), size=0.02)
        body = self.import_mesh('car_body.obj', 'body', car, collidable=False)
        lidar_frame = self.dummy('lidarFrame', LIDAR_FRAME, car, 0.005)
        lidar_body = self.import_mesh('car_lidar.obj', 'lidarBody', car)
        ray = sim.createProximitySensor(sim.proximitysensor_ray, 16, 1 | 4, [0] * 8,
                                        [0.02, 12.0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.001, 0, 0])
        sim.setObjectAlias(ray, 'lidarRay')
        sim.setObjectParent(ray, lidar_frame, False)
        sim.setObjectPosition(ray, [0, 0, 0], lidar_frame)
        # wheels: front ones turn about a steering dummy
        for side, sgn, mesh in (('L', 1, 'wheel_left.obj'), ('R', -1, 'wheel_right.obj')):
            steer = self.dummy(f'steerF{side}', (WHEEL_FRONT[0], sgn * WHEEL_FRONT[1], WHEEL_FRONT[2]), car, 0.004)
            wf = self.import_mesh(mesh, f'wheelF{side}', steer)
            sim.setObjectPosition(wf, [0, 0, 0], steer)
            wr = self.import_mesh(mesh, f'wheelR{side}', car)
            sim.setObjectPosition(wr, [WHEEL_REAR[0], sgn * WHEEL_REAR[1], WHEEL_REAR[2]], car)
        # collision hull (invisible): the car's outline seen from above (convex
        # hull of the CAD, assets/footprint.json) as a prism above the floor
        foot = self.footprint()
        n = len(foot)
        verts, idx = [], []
        for zz in (0.01, 0.16):
            for px, py in foot:
                verts += [px, py, zz]
        for i in range(n):
            j = (i + 1) % n
            idx += [i, j, n + j, i, n + j, n + i]
        for i in range(1, n - 1):
            idx += [0, i + 1, i, n, n + i, n + i + 1]
        hull = sim.createMeshShape(0, 0.0, verts, idx)
        sim.setObjectAlias(hull, 'hull')
        self.make_static(hull, collidable=True)
        sim.setObjectParent(hull, car, False)
        try:
            sim.setObjectInt32Param(hull, sim.objintparam_visibility_layer, 0)
        except Exception:
            pass
        # what the LiDAR sees of the car itself: at its scanning height only the
        # 90 mm camera-mast plate behind it crosses the plane (CAD section at
        # 72 mm), so a thin invisible plate stands in for the whole body mesh
        occ = sim.createPrimitiveShape(sim.primitiveshape_cuboid, [0.007, 0.090, 0.012], 0)
        sim.setObjectAlias(occ, 'lidarOccluder')
        self.make_static(occ, collidable=False)
        sim.setObjectParent(occ, car, False)
        sim.setObjectPosition(occ, [0.0135, 0, LIDAR_FRAME[2]], car)
        try:
            sim.setObjectInt32Param(occ, sim.objintparam_visibility_layer, 0)
        except Exception:
            pass
        # LED of the Grove start button
        led = sim.createPrimitiveShape(sim.primitiveshape_spheroid, [0.012, 0.012, 0.008], 0)
        sim.setObjectAlias(led, 'startLed')
        sim.setShapeColor(led, None, sim.colorcomponent_ambient_diffuse, [0.2, 0.25, 0.2])
        self.make_static(led, collidable=False)
        sim.setObjectParent(led, car, False)
        sim.setObjectPosition(led, [0.056, -0.0508, 0.032], car)
        # camera: Camera Module 3 Wide, 800 x 600 like the "rpi" input
        cam = sim.createVisionSensor(1 | 2 | 4, [800, 600, 0, 0],
                                     [0.01, 6.0, math.radians(CAMERA_HFOV_DEG), 0.01, 0, 0, 0.5, 0.5, 0.5, 0, 0])
        sim.setObjectAlias(cam, 'camera')
        sim.setObjectParent(cam, car, False)
        tilt = math.radians(CAMERA_TILT_DEG)
        eye = CAMERA_POS
        target = (eye[0] + math.cos(tilt), eye[1], eye[2] - math.sin(tilt))
        self.set_lookat(cam, eye, target)
        # start pose
        sim.setObjectPosition(car, [start['x'], start['y'], 0], sim.handle_world)
        sim.setObjectOrientation(car, [0, 0, start['th']], sim.handle_world)
        return car

    def add_views(self):
        sim = self.sim
        try:
            cam = sim.getObject('/DefaultCamera')
            self.set_lookat(cam, (0.0, -3.4, 3.4), (0.0, -0.1, 0.0))
        except Exception:
            pass
        # recorder views for the run video (rendered only on request)
        top = sim.createVisionSensor(1 | 2 | 4, [1280, 720, 0, 0],
                                     [0.05, 20.0, math.radians(60), 0.01, 0, 0, 0.85, 0.87, 0.9, 0, 0])
        sim.setObjectAlias(top, 'recOverview')
        self.set_lookat(top, (0.0, -3.3, 3.6), (0.0, -0.15, 0.0))
        chase = sim.createVisionSensor(1 | 2 | 4, [640, 360, 0, 0],
                                       [0.02, 10.0, math.radians(60), 0.01, 0, 0, 0.85, 0.87, 0.9, 0, 0])
        sim.setObjectAlias(chase, 'recChase')

    def attach_script(self, car, field_lua):
        sim = self.sim
        with open(os.path.join(HERE, 'twin_car.lua'), encoding='utf-8') as f:
            code = f.read()
        start = code.index('FIELD = {--@FIELD@')
        end = code.index('}--@END@') + len('}--@END@')
        code = code[:start] + 'FIELD = ' + field_lua + code[end:]
        try:
            script = sim.createScript(sim.scripttype_simulation, code, 0, 'lua')
            sim.setObjectParent(script, car, True)
        except Exception:
            script = sim.addScript(sim.scripttype_childscript)
            sim.setScriptStringParam(script, sim.scriptstringparam_text, code)
            sim.associateScriptWithObject(script, car)
        return script


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--layout', default='A', help='A..E (team test layouts), random, or a JSON file')
    ap.add_argument('--direction', default='cw', choices=['cw', 'ccw'])
    ap.add_argument('--seed', type=int, default=1)
    ap.add_argument('--start-gap', type=float, default=5.0,
                    help='mm between the front bumper and the front parking limit at the start')
    ap.add_argument('--start-side', type=float, default=120.0,
                    help='mm between the outer wall and the centre line of the car at the start')
    ap.add_argument('--modify', default=None,
                    help='surprise rule: X_MM,Y_MM,HEIGHT_MM replaces the sign at that seat by one of '
                         'the opposite colour, 50 or 150 mm tall')
    ap.add_argument('--out', default=None, help='scene file (.ttt)')
    ap.add_argument('--no-save', action='store_true')
    args = ap.parse_args()

    layout = load_layout(args.layout, args.seed)
    tag_extra = ''
    if args.modify:
        mx, my, mh = (int(v) for v in args.modify.split(','))
        for sgn in layout['pillars']:
            if sgn['x_mm'] == mx and sgn['y_mm'] == my:
                sgn['original'] = sgn['color']
                sgn['color'] = 'green' if sgn['color'] == 'red' else 'red'
                sgn['height_mm'] = mh
                layout['name'] += f" + surprise: {sgn['original']} sign at ({mx}, {my}) -> {sgn['color']} {mh} mm"
                tag_extra = f'_S{mx}x{my}h{mh}'
                break
        else:
            raise SystemExit(f'no sign at ({mx}, {my}) in the layout')
    b = Builder()
    b.new_scene()
    lot = b.build_field(layout)
    # start: inside the lot, front wheels towards the next corner of the run
    # (rules), the front bumper start_gap mm from the front limit, so the
    # reverse that begins the exit manoeuvre has room behind the car
    lot_x, _ = mm_to_sim(3000 - args.start_side, 0)
    gap = args.start_gap / 1000.0
    if args.direction == 'cw':      # facing south: the open lane is on the right
        th = -math.pi / 2
        front_limit = lot['y0']
        y = front_limit + BODY_X[1] + gap
    else:                           # facing north: the open lane is on the left
        th = math.pi / 2
        front_limit = lot['y1']
        y = front_limit - BODY_X[1] - gap
    start = {'x': lot_x, 'y': y, 'th': th}
    car = b.build_car(start)
    b.add_views()
    foot = ', '.join('{%.4f, %.4f}' % (p[0], p[1]) for p in Builder.footprint())
    signs_lua = ', '.join("['%s'] = {original = '%s', height = %d}" % (a, o, h) for a, _, _, o, h in b.sign_info)
    field_lua = ('{name = %s, direction = %s, lot = {x0 = %.4f, x1 = %.4f, y0 = %.4f, y1 = %.4f, '
                 'wall_dir = math.pi / 2}, footprint = {%s}, signs = {%s}}' % (
                     json.dumps(layout['name']), json.dumps(args.direction),
                     lot['x0'], lot['x1'], lot['y0'], lot['y1'], foot, signs_lua))
    b.attach_script(car, field_lua)
    os.makedirs(os.path.join(HERE, 'results'), exist_ok=True)
    if not args.no_save:
        tag = args.layout.upper() if len(args.layout) == 1 else (
            f'R{args.seed}' if args.layout == 'random' else os.path.splitext(os.path.basename(args.layout))[0])
        out = args.out or os.path.join(HERE, f'wro_twin_{tag}{tag_extra}_{args.direction}.ttt')
        b.sim.saveScene(out.replace('\\', '/'))
        print('Scene saved:', out)
    print('Layout:', layout['name'], '| direction:', args.direction, '| signs:', len(layout['pillars']))


if __name__ == '__main__':
    main()
