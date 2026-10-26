#!/usr/bin/env python3
"""Phase 9 shared activation/effects with disposable SQL and prior regressions."""
from pathlib import Path
import os
import runpy
os.environ['LEGENDARY_EFFECTS_TEST']='1'
runpy.run_path(str(Path(__file__).with_name('run_legendary_foundations.py')),run_name='__main__')
