#!/usr/bin/env python3
"""Phase 6 player commands plus Phases 3-5 regressions, disposable database only."""
from pathlib import Path
import os
import runpy
os.environ['LEGENDARY_COMMANDS_TEST']='1'
os.environ['LEGENDARY_EARNINGS_TEST']='1'
runpy.run_path(str(Path(__file__).with_name('run_estate_integration.py')),run_name='__main__')
