#!/usr/bin/env python3
"""Verify production feat export against disposable MariaDB, never the game DB."""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile
import time

root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='krynn-feat-export-') as directory:
    temp = Path(directory)
    data = temp / 'data'
    socket = temp / 'mysql.sock'
    log = temp / 'mysql.log'
    executable = temp / 'feat-export-test'
    flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'mariadb'], text=True))
    subprocess.run([os.environ.get('CC', 'cc'), '-std=gnu99', '-Wall', '-Wextra',
                    '-Wno-unused-parameter', '-ffunction-sections', '-fdata-sections',
                    '-I'+str(root/'src'), str(root/'src/feats.c'), str(root/'src/constants.c'),
                    str(root/'tests/feat_info_export.c'), '-Wl,--gc-sections',
                    *flags, '-o', str(executable)], check=True)
    subprocess.run(['mariadb-install-db', '--no-defaults', '--datadir='+str(data),
                    '--auth-root-authentication-method=normal', '--skip-test-db'],
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)
    server = subprocess.Popen(['mariadbd', '--no-defaults', '--datadir='+str(data),
                               '--socket='+str(socket), '--pid-file='+str(temp/'mysql.pid'),
                               '--skip-networking', '--log-error='+str(log),
                               '--user='+os.environ.get('USER', 'krynn')],
                              stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    try:
        client = ['mariadb', '--no-defaults', '--socket='+str(socket), '-uroot']
        for attempt in range(100):
            result = subprocess.run([*client, '-e', 'SELECT 1'],
                                    stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            if result.returncode == 0:
                break
            if server.poll() is not None:
                raise RuntimeError(log.read_text())
            time.sleep(.1)
        else:
            raise RuntimeError('Temporary MariaDB did not become ready')
        subprocess.run([*client, '-e', 'CREATE DATABASE feat_export_test CHARACTER SET utf8mb4'], check=True)
        subprocess.run([str(executable), str(socket)], check=True)
        # Ensure the optional migration can also be used repeatedly.
        for _ in range(2):
            subprocess.run([*client, 'feat_export_test'],
                           input=(root/'sql/20261025_feat_info.sql').read_text(), text=True, check=True)
    finally:
        server.terminate()
        server.wait(timeout=20)
