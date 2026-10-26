#!/usr/bin/env python3
"""Run Phase 5 earning rules and persistence against disposable MariaDB."""
from pathlib import Path
import os
import runpy
os.environ['LEGENDARY_PERSISTENCE_TEST']='1'
os.environ['LEGENDARY_TRANSACTION_TEST']='1'
os.environ['LEGENDARY_EARNINGS_TEST']='1'
runpy.run_path(str(Path(__file__).with_name('run_estate_integration.py')),run_name='__main__')
