#!/usr/bin/env python3
"""Full legendary integration, actual process deaths, durable budget/performance audit."""
from pathlib import Path
import os
import runpy
os.environ['LEGENDARY_SECURITY_TEST']='1'
runpy.run_path(str(Path(__file__).with_name('run_legendary_ranks.py')),run_name='__main__')
