"""Prints where the scene script spends each simulation step, and the
simulation speed against real time, while a run is in progress.

    python profile_step.py [seconds]
"""
import sys
import time

from coppeliasim_zmqremoteapi_client import RemoteAPIClient

sim = RemoteAPIClient().require('sim')
script = sim.getScript(sim.scripttype_simulation, sim.getObject('/TheMaker'))
seconds = float(sys.argv[1]) if len(sys.argv) > 1 else 5.0
sim.callScriptFunction('profile_get', script, True)
t0, w0 = sim.getSimulationTime(), time.time()
time.sleep(seconds)
t1, w1 = sim.getSimulationTime(), time.time()
print('sim/wall = %.3f' % ((t1 - t0) / (w1 - w0)))
print(sim.callScriptFunction('profile_get', script, True))
