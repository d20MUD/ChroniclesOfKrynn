#!/usr/bin/env python3
"""Convert old personal-hunt files to SQL; never modify files or connect to a DB.

Stop the relevant game first. Apply sql/20261114_personal_hunts.sql, then pipe
this script's output to the MariaDB batch client WITHOUT --force. Duplicate
character IDs abort the import instead of overwriting existing SQL progress.
"""
import argparse
from pathlib import Path
import sys


def convert(directory: Path) -> str:
    statements = []
    for path in sorted(directory.iterdir()):
        if not path.is_file() or not path.name.isdecimal():
            continue
        values = path.read_text().split()
        if len(values) != 38 or values[0] != 'HUNTS1':
            raise ValueError(f'Invalid hunt record: {path}')
        owner, expires, *targets = map(int, values[1:])
        if owner != int(path.name) or not 0 < owner < 2**63 or not 0 <= expires < 2**63:
            raise ValueError(f'Invalid identity or expiry: {path}')
        if any(not -(2**31) <= n < 2**31 for n in targets) or any(not 0 <= targets[i] < 35 for i in range(0, 35, 7)):
            raise ValueError(f'Invalid target data: {path}')
        payload = ' '.join(map(str, targets)) + ' '
        statements.append(f"INSERT INTO personal_hunt_progress (character_id,expires_at,targets,revision) VALUES ({owner},{expires},'{payload}',1);")
    return 'START TRANSACTION;\n' + '\n'.join(statements) + '\nCOMMIT;\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, required=True)
    args = parser.parse_args()
    try:
        sql = convert(args.directory)
    except (OSError, ValueError) as error:
        parser.exit(1, f'STOP: {error}\n')
    sys.stdout.write(sql)


if __name__ == '__main__':
    main()
