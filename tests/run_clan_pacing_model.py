#!/usr/bin/env python3
"""Compare modeled XP with production C and verify rank effort after rounding."""
import ast
from fractions import Fraction
import importlib.util
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('pacing', ROOT/'scripts/model_clan_progression_pacing.py')
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)
a = m.parser().parse_args(['--output', '/unused'])
data = m.build(a)
assert data['assumptions']['kills_per_logged_hour'] == 30
# Reuse the isolated XP adapter, replacing its example constants and XP table
# with actual production definitions. Native XP functions themselves are unmodified.
fixture = ast.parse((ROOT/'tests/run_clan_scholar.py').read_text())
header = next(n.value.value for n in fixture.body if isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'header' for t in n.targets))
def function(source, signature):
    start = source.index(signature)
    start = source.rfind('\n', 0, start)+1
    return source[start:source.index('\n}', start)+2]
levels = function((ROOT/'src/class.c').read_text(), 'long int level_exp(')
tokens = sorted(set(re.findall(r'case ([A-Z_0-9]+):', levels)))
definitions = '\n'.join(f'#define {token} {i+100}' for i,token in enumerate(tokens))
header = re.sub(r'static long level_exp\(.*?\n', '', header)
header += '\n'+'\n'.join(l for l in (ROOT/'src/mudlim.h').read_text().splitlines() if l.startswith('#define GAIN_EXP_MODE_'))+'\n'+definitions+'\n#define GET_CLASS(ch) ((void)(ch), CLASS_WARRIOR)\n#define GET_REAL_RACE(ch) (-1)\nstatic int requirement_percent=100;\n#define CONFIG_EXPERIENCE_MULTIPLIER requirement_percent\n#define LVL_IMPL 34\n#define EXP_MAX 2147483647\n#define CAMPAIGN_DL 1\n#define log(...) ((void)0)\n'+levels+'\n'
for name, value in [('NEWBIE_LEVEL', m.read_define(ROOT/'src/structs.h', 'NEWBIE_LEVEL')), ('NEWBIE_EXP', m.read_define(ROOT/'src/limits.c', 'NEWBIE_EXP'))]+[(f'MIN_NUM_MOBS_TO_KILL_{n}',m.read_define(ROOT/'src/limits.c',f'MIN_NUM_MOBS_TO_KILL_{n}')) for n in [5,10,15,20,25]]:
    header += f'\n#undef {name}\n#define {name} {value}\n'
limits = (ROOT/'src/limits.c').read_text()
functions = '\n'.join(function(limits, n) for n in ['static int scholar_adventuring_xp(', 'int gain_exp(', 'int gain_exp_with_source('])
main = r'''
int main(void) {
 for(int percent=100;percent<=250;percent+=150) {
 requirement_percent=percent;
 for(int l=1;l<30;l++) {
  struct char_data ch={.level=l};ch.exp=level_exp(&ch,l);
  int gained=gain_exp_with_source(&ch,25*l*l,GAIN_EXP_MODE_SOLO,NULL);
  printf("%d %d %ld %d\n",percent,l,level_exp(&ch,l),gained);
 }
 }
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='clan-pacing-') as directory:
    path = Path(directory)
    (path/'native.c').write_text(header+functions+main)
    subprocess.run(['cc','-std=gnu99','-Wall','-Wextra','-Werror','-Wno-unused-function','-fsanitize=address,undefined','-fno-pie','-no-pie',str(path/'native.c'),'-o',str(path/'native')],check=True)
    config=path/'config';config.write_text('max_exp_gain = 999999\nexperience_multiplier = 250\n')
    live_args=type(a)(**vars(a));live_args.game_config=config
    cases={100:data,250:m.build(live_args)}
    assert cases[250]['favor_ranks'][-1]['threshold']==3036
    assert cases[250]['influence_ranks'][-1]['threshold']==22764
    for line in subprocess.check_output([str(path/'native')], text=True).splitlines():
        percent,level,xp,award = map(int,line.split())
        row=cases[percent]['comparison'][level-1]
        assert row['cumulative_character_xp']==xp and row['same_level_kill_award']==award,(level,row,award)
assert len(data['favor_ranks'])==6 and len(data['influence_ranks'])==20
for key,rate,factor in [('favor_ranks',Fraction(1,4),2),('influence_ranks',Fraction(1,8),30)]:
    previous_kills=Fraction(0)
    for rank in data[key]:
        row=data['comparison'][rank['reference_character_level']-1]
        kills=sum((Fraction(x['xp_to_next_level'],x['same_level_kill_award']) for x in data['comparison'][:rank['reference_character_level']-1]),Fraction(0))
        assert Fraction(rank['increment'])/rate >= (kills-previous_kills)*factor
        previous_kills=kills
assert data['favor_ranks'][-1]['threshold']==1454
assert data['influence_ranks'][-1]['threshold']==10898
print('Pacing model: production C level XP/kill caps/newbie awards, 30 fights/hour, sixth/twentieth-rank mapping and conservative 2x/10x effort rounding passed.')

# Later easing is a parameter change, with no need to change point units.
faster=type(a)(**vars(a));faster.favor_time='1';faster.clan_time='5'
eased=m.build(faster)
assert eased['favor_ranks'][-1]['threshold'] < data['favor_ranks'][-1]['threshold']
assert eased['influence_ranks'][-1]['threshold'] < data['influence_ranks'][-1]['threshold']
