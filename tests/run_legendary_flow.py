#!/usr/bin/env python3
from pathlib import Path
import os,re,runpy
root=Path(__file__).resolve().parents[1]
path=root/'docs/systems/legendary_balance_results/player_flow.txt'
path.parent.mkdir(parents=True,exist_ok=True)
os.environ['LEGENDARY_FLOW_TEST']='1'
os.environ['LEGENDARY_FLOW_CAPTURE']=str(path)
runpy.run_path(str(Path(__file__).with_name('run_legendary_ranks.py')),run_name='__main__')
path.write_text(re.sub(r'\b[0-9a-f]{32}\b','<id>',path.read_text()))
