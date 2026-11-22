"""Short text summary of run results (logs/run_*.json): laps, contacts, referee events.

    python run_summary.py logs/run_A_cw_*.json
"""
import json
import sys

for f in sys.argv[1:]:
    d = json.load(open(f, encoding='utf-8'))
    print(f)
    print('  laps %.2f | signs touched %s | wall contacts %s | surprise %s/%s | result %s | simulation speed %s' % (
        d.get('laps', 0), [(s['sign'], s['moved_mm']) for s in d.get('signs_touched', [])],
        d.get('wall_contacts'), d.get('surprise_correct'), d.get('surprise_passages'), d.get('result'),
        d.get('sim_speed')))
    for e in d.get('events', []):
        print('   ', e)
