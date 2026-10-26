#!/usr/bin/env python3
"""Live techniques against disposable SQL and the production effect engine."""
from pathlib import Path
import os
import runpy
os.environ['LEGENDARY_TECHNIQUES_TEST']='1'
os.environ['LEGENDARY_EFFECTS_TEST']='1'
runpy.run_path(str(Path(__file__).with_name('run_legendary_foundations.py')),run_name='__main__')
