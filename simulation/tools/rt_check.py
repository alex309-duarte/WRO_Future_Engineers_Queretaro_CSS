"""Measures how fast the open scene runs compared with real time."""
import time
from coppeliasim_zmqremoteapi_client import RemoteAPIClient
sim = RemoteAPIClient().require('sim')
print('time step', sim.getSimulationTimeStep())
for name in ('boolparam_realtime_simulation',):
    try:
        print(name, sim.getBoolParam(getattr(sim, name)))
    except Exception as e:
        print(name, 'n/a', e)
started = False
if sim.getSimulationState() == sim.simulation_stopped:
    sim.startSimulation(); started = True
time.sleep(1.0)
t0, w0 = sim.getSimulationTime(), time.time()
time.sleep(5.0)
t1, w1 = sim.getSimulationTime(), time.time()
print('sim/wall = %.3f  (%.2f s sim in %.2f s)' % ((t1 - t0) / (w1 - w0), t1 - t0, w1 - w0))
if started:
    sim.stopSimulation()
