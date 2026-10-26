#!/usr/bin/env python3
"""Phase 12 rank/modifier contracts and live smith/effect integration."""
from pathlib import Path
import os
import runpy
runpy.run_path(str(Path(__file__).with_name('run_legendary_rules.py')),run_name='__main__')
os.environ['LEGENDARY_RANKS_TEST']='1'
runpy.run_path(str(Path(__file__).with_name('run_legendary_signatures.py')),run_name='__main__')
