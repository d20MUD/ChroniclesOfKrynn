#!/usr/bin/env python3
"""Verify zone environment editing, header round trips, and legacy defaults."""
from pathlib import Path
import os
import re
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
def function(source, name):
    start=re.search(r'^(?:static )?(?:void|int|bool|const char \*)\s*'+name+r'\(',source,re.M).start()
    brace=source.index('\n{',start)
    return source[start:source.index('\n}',brace)+2]+'\n'
constants=(root/'src/constants.c').read_text()
a=constants.index('const char *zone_climates[]'); b=constants.index('/** Current LuminariMUD version',a)
code=constants[a:b].replace('CHECK_TABLE_SIZE(zone_climates, NUM_ZONE_CLIMATES);','').replace('CHECK_TABLE_SIZE(zone_terrains, NUM_ZONE_TERRAINS);','')
code+=function((root/'src/db.c').read_text(),'parse_zone_environment')
zedit=(root/'src/zedit.c').read_text()
code+=function(zedit,'zedit_disp_environment')+function(zedit,'zedit_set_environment')
writer=(root/'src/genzon.c').read_text()
a=writer.index('  for (i = 0; i < ZN_ARRAY_MAX; i++)',writer.index('int save_zone('))
b=writer.index('  /* Handy Quick Reference',a)
code+='static void write_header(FILE *zfile, zone_rnum zone_num) {\nint i, flag_tot=0;\nchar zbuf1[128],zbuf2[128],zbuf3[128],zbuf4[128];\n'+writer[a:b]+'}\n'
headers='\n'.join('#include "'+h+'"' for h in
    ['conf.h','sysdep.h','structs.h','utils.h','db.h','comm.h','constants.h','genolc.h','oasis.h'])
stubs=r'''
#include <assert.h>
struct zone_data *zone_table;
static int menus;
static char output[8192];
static void zedit_disp_menu(struct descriptor_data *d) { menus++; OLC_MODE(d)=ZEDIT_MAIN_MENU; }
size_t write_to_output(struct descriptor_data *d,const char *fmt,...) {
  va_list ap; va_start(ap,fmt); int n=vsnprintf(output+strlen(output),sizeof(output)-strlen(output),fmt,ap); va_end(ap); return n;
}
int sprintascii(char *out,bitvector_t bits) { return sprintf(out,"%llu",(unsigned long long)bits); }
char *convert_from_tabs(char *s) { return s; }
room_vnum genolc_zone_bottom(zone_rnum zone) { return zone_table[zone].bot; }
'''
checks=r'''
int main(void) {
  struct zone_data zone={0}, loaded={0}; struct descriptor_data d={0}; struct oasis_olc_data olc={0};
  d.olc=&olc; olc.zone=&zone; zone_table=&zone;
  zone.number=200; zone.bot=20000; zone.top=20099; zone.lifespan=30; zone.reset_mode=2;
  zone.min_level=zone.max_level=-1; zone.show_weather=1; zone.name="Test zone"; zone.builders="Builder";
  const char *legacy[]={"20000 20099 30 2", "20000 20099 30 2 0 0 0 0 -1 -1",
    "20000 20099 30 2 0 0 0 0 -1 -1 1", "20000 20099 30 2 0 0 0 0 -1 -1 1 2 3 4"};
  for (int i=0;i<4;i++) {
    loaded.climate=ZONE_CLIMATE_TROPICAL; loaded.terrain=ZONE_TERRAIN_FOREST;
    parse_zone_environment(&loaded,legacy[i]);
    assert(loaded.climate==ZONE_CLIMATE_UNSPECIFIED && loaded.terrain==ZONE_TERRAIN_UNSPECIFIED);
  }
  for (int climate=0;climate<NUM_ZONE_CLIMATES;climate++)
    for (int terrain=0;terrain<NUM_ZONE_TERRAINS;terrain++) {
      zone.climate=climate; zone.terrain=terrain;
      FILE *fp=tmpfile(); assert(fp); write_header(fp,0); rewind(fp);
      char line[1024]; for(int i=0;i<4;i++) assert(fgets(line,sizeof(line),fp));
      parse_zone_environment(&loaded,line);
      assert(loaded.climate==climate && loaded.terrain==terrain);
      if (climate || terrain) {
        int bot,top,life,reset,min,max,weather,region,faction,city,c,t; char a[32],b[32],e[32],f[32];
        assert(sscanf(line,"%d %d %d %d %s %s %s %s %d %d %d %d %d %d %d %d",
          &bot,&top,&life,&reset,a,b,e,f,&min,&max,&weather,&region,&faction,&city,&c,&t)==16);
        assert(bot==20000 && top==20099 && min==-1 && max==-1 && weather==1);
      }
      fclose(fp);
    }
  parse_zone_environment(&loaded,"20000 20099 30 2 0 0 0 0 -1 -1 1 0 0 0 -1 999");
  assert(!loaded.climate && !loaded.terrain);
  parse_zone_environment(&loaded,"20000 20099 30 2 0 0 0 0 -1 -1 1 0 0 0 5");
  assert(loaded.climate==ZONE_CLIMATE_TROPICAL && !loaded.terrain);
  assert(!strcmp(zone_climate_name(-1),"Unspecified") && !strcmp(zone_terrain_name(999),"Unspecified"));
  zone.number=0; zone.climate=ZONE_CLIMATE_TEMPERATE; zone.terrain=ZONE_TERRAIN_FOREST;
  zedit_disp_environment(&d,TRUE); assert(OLC_MODE(&d)==ZEDIT_CLIMATE && strstr(output,"Tropical"));
  zedit_set_environment(&d,"5",TRUE); assert(zone.climate==ZONE_CLIMATE_TROPICAL && zone.number==1 && menus==1);
  zone.number=0; zedit_set_environment(&d,"3",FALSE);
  assert(zone.terrain==ZONE_TERRAIN_DESERT && zone.climate==ZONE_CLIMATE_TROPICAL && zone.number==1 && menus==2);
  const char *invalid[]={"", "   ", "-1", "999", "3junk", "999999999999999999999999"};
  for(int i=0;i<5;i++) {
    zone.number=0; zedit_set_environment(&d,invalid[i],TRUE); zedit_set_environment(&d,invalid[i],FALSE);
    assert(zone.climate==ZONE_CLIMATE_TROPICAL && zone.terrain==ZONE_TERRAIN_DESERT && !zone.number && menus==2);
  }
  zedit_set_environment(&d,"0",TRUE); zedit_set_environment(&d,"0",FALSE);
  assert(!zone.climate && !zone.terrain);
  return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='zone-environment-') as tmp:
    path=Path(tmp)/'check.c'; path.write_text(headers+stubs+code+checks)
    binary=Path(tmp)/'check'
    subprocess.run([os.environ.get('CC','cc'),'-std=gnu99','-Wall','-Wextra','-Wno-unused-parameter',
                    '-I'+str(root/'src'),str(path),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('Zone environment: all climate/terrain header round trips, 4 legacy formats, invalid defaults, editor choices/dirty state, and name safety passed.')
