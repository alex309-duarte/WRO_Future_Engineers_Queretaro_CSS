"""Print the referee state of the running scene once per second (Ctrl+C to stop)."""
import sys, time
from coppeliasim_zmqremoteapi_client import RemoteAPIClient
sim = RemoteAPIClient().require('sim')
script = sim.getScript(sim.scripttype_simulation, sim.getObject('/TheMaker'))
n = int(sys.argv[1]) if len(sys.argv) > 1 else 10 ** 9
for _ in range(n):
    if sim.getSimulationState() == sim.simulation_stopped:
        print('simulation stopped'); break
    h = sim.callScriptFunction('hud', script)
    print('t=%5.1f laps=%.2f %s x=%.3f y=%.3f th=%.0f steer=%+.1f v=%.2f touched=%d walls=%d led=%s %s' % (
        h['time'], h['laps'], h['direction'], h['x'], h['y'], h['th'] * 57.3, h['steer'], h['speed'],
        h['touched'], h['walls'], h['led'], h['result']), flush=True)
    time.sleep(1)
