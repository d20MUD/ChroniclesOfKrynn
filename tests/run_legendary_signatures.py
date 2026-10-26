#!/usr/bin/env python3
"""Signatures and technique regressions against disposable MariaDB."""
from pathlib import Path
import os
import runpy
os.environ['LEGENDARY_SIGNATURES_TEST']='1'
runpy.run_path(str(Path(__file__).with_name('run_legendary_techniques.py')),run_name='__main__')
