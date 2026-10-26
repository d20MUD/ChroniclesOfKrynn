#!/usr/bin/env python3
"""Run full legendary persistence/codec tests using disposable MariaDB only."""
from pathlib import Path
import os
import runpy
os.environ['LEGENDARY_PERSISTENCE_TEST'] = '1'
runpy.run_path(str(Path(__file__).with_name('run_estate_integration.py')), run_name='__main__')
