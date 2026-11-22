"""Quick check of the emulated board in the open scene (no robot program):
drives 1000 mm with the position loop, as Spike_Advance_For_distance(…, 1000, …)
does, and reports the distance measured in the scene."""
import math, time, zmq
from coppeliasim_zmqremoteapi_client import RemoteAPIClient
sim = RemoteAPIClient().require('sim')
started = sim.getSimulationState() == sim.simulation_stopped
if started:
    sim.startSimulation()
time.sleep(1.0)
ctx = zmq.Context(); s = ctx.socket(zmq.REQ); s.connect('tcp://127.0.0.1:23101')
def cmd(m):
    s.send(m.encode()); return s.recv().decode()
car = sim.getObject('/TheMaker')
print(cmd('pose -1.2 0.72 0'))  # north corridor, facing east
time.sleep(0.2)
p0 = sim.getObjectPosition(car, sim.handle_world)
print(cmd('zero'))
ticks = round(1000 / (math.pi * 65) * 1320)
print(cmd(f'mv {ticks} 2.85 20 8000'))
t0 = time.time()
while True:
    st = cmd('st').split()
    if st[4] == '0':
        break
    time.sleep(0.05)
p1 = sim.getObjectPosition(car, sim.handle_world)
print('state', st, 'took %.2f s' % (time.time() - t0))
print('moved %.1f mm' % (1000 * math.hypot(p1[0] - p0[0], p1[1] - p0[1])))
cmd('stop')
if started:
    sim.stopSimulation()
