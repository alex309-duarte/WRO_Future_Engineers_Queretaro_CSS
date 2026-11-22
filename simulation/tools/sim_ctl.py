"""Small control of CoppeliaSim from the command line (Windows side).

    python sim_ctl.py load <scene.ttt>   open a scene
    python sim_ctl.py start | stop       start / stop the simulation
    python sim_ctl.py wait-stopped       block until the simulation is stopped
    python sim_ctl.py display on | off   draw the 3D view while simulating, or not
    python sim_ctl.py speed [seconds]    simulated time against real time
"""
import os, sys, time
from coppeliasim_zmqremoteapi_client import RemoteAPIClient
sim = RemoteAPIClient().require('sim')
cmd = sys.argv[1]
if cmd == 'load':
    if sim.getSimulationState() != sim.simulation_stopped:
        sim.stopSimulation()
        while sim.getSimulationState() != sim.simulation_stopped:
            time.sleep(0.1)
    sim.closeScene()
    sim.loadScene(os.path.abspath(sys.argv[2]).replace('\\', '/'))
elif cmd == 'start':
    sim.startSimulation()
elif cmd == 'stop':
    sim.stopSimulation()
    while sim.getSimulationState() != sim.simulation_stopped:
        time.sleep(0.1)
elif cmd == 'wait-stopped':
    while sim.getSimulationState() != sim.simulation_stopped:
        time.sleep(0.5)
elif cmd == 'display':
    sim.setBoolParam(sim.boolparam_display_enabled, sys.argv[2] == 'on')
elif cmd == 'speed':
    seconds = float(sys.argv[2]) if len(sys.argv) > 2 else 5.0
    t0, w0 = sim.getSimulationTime(), time.time()
    time.sleep(seconds)
    t1, w1 = sim.getSimulationTime(), time.time()
    print('simulated / real time = %.3f' % ((t1 - t0) / (w1 - w0)))
print('simulation state:', sim.getSimulationState())
