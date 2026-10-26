#!/usr/bin/env python3
"""Phase 8 foundations with real ledger/codec/cache plus all previous regressions."""
from pathlib import Path
import os
import runpy
os.environ['LEGENDARY_FOUNDATIONS_TEST']='1'
os.environ['LEGENDARY_EQUIPMENT_TEST']='1'
os.environ['LEGENDARY_COMMANDS_TEST']='1'
os.environ['LEGENDARY_EARNINGS_TEST']='1'
runpy.run_path(str(Path(__file__).with_name('run_estate_integration.py')),run_name='__main__')
