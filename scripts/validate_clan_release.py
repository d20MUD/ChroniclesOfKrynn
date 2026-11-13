#!/usr/bin/env python3
"""Run all clan suites and both builds locally; never deploy or enable services."""
import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
from datetime import datetime, timezone
import json
from pathlib import Path
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--web-root', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--jobs', type=int, default=3, choices=range(1, 5))
    args = p.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    web = args.web_root.resolve()
    results = []
    start = datetime.now(timezone.utc).isoformat()

    def execute(name, command, cwd):
        now = time.monotonic()
        with (args.output / (name + '.log')).open('w') as out:
            r = subprocess.run(command, cwd=cwd, stdout=out, stderr=subprocess.STDOUT)
        return dict(name=name, command=command, cwd=str(cwd), exit_code=r.returncode,
                    seconds=round(time.monotonic()-now, 3), log=name+'.log')

    suites = sorted((ROOT/'tests').glob('run_clan_*.py'))
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        pending = [pool.submit(execute, s.stem, [sys.executable, str(s)], ROOT) for s in suites]
        for task in as_completed(pending):
            r = task.result()
            results.append(r)
            print(f"{'PASS' if r['exit_code']==0 else 'FAIL'} {r['name']}", flush=True)
    # Dependencies, fixture generation and builds remain sequential.
    commands = [
        ('server-build', ['cmake', '--build', 'build', '-j'+str(args.jobs)], ROOT),
        ('server-make-build', ['make', '-j'+str(args.jobs)], ROOT),
        ('web-tests', ['node', '--test', *[str(s) for s in sorted((web/'tests').glob('*.test.mjs'))]], web),
        ('web-build', ['npm', 'run', 'build'], web),
        ('web-lint', ['npm', 'run', 'lint'], web),
    ]
    for name, cmd, cwd in commands:
        r = execute(name, cmd, cwd)
        results.append(r)
        print(f"{'PASS' if r['exit_code']==0 else 'FAIL'} {name}", flush=True)
    report = dict(version=1, started_at=start, finished_at=datetime.now(timezone.utc).isoformat(),
                  passed=all(r['exit_code']==0 for r in results), results=results)
    (args.output/'validation.json').write_text(json.dumps(report, indent=2)+'\n')
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
