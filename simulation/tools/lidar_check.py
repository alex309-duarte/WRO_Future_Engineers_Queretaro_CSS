"""Compares the computed LiDAR of the scene script with CoppeliaSim's own ray
sensor on the loaded scene (simulation running): worst and mean difference.

    python lidar_check.py
"""
import time

from coppeliasim_zmqremoteapi_client import RemoteAPIClient

sim = RemoteAPIClient().require('sim')
started = sim.getSimulationState() == sim.simulation_stopped
if started:
    sim.startSimulation()
    time.sleep(1.0)
script = sim.getScript(sim.scripttype_simulation, sim.getObject('/TheMaker'))
worst, mean = sim.callScriptFunction('lidar_selftest', script, 720)
print('720 rays: worst difference %.2f mm, mean %.3f mm' % (worst, mean))
if started:
    sim.stopSimulation()
