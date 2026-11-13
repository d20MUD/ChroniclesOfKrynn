#!/usr/bin/env python3
"""Exercise the actual compatible inventory header writer and delivery restore."""
from pathlib import Path
import subprocess,tempfile,re
root=Path(__file__).resolve().parents[1]
source=(root/'src/objsave.c').read_text()
def function(signature):
    match=re.search(re.escape(signature)+r'[^;{]*\n\{',source);assert match,signature
    start=match.start();brace=source.index('{',start);depth=1;end=brace+1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[start:end]
code='\n'.join('#include "'+h+'"' for h in ['conf.h','sysdep.h','structs.h','utils.h','legendary_identity.h'])+'\n#include <assert.h>\n'
code+=function('static int objsave_write_rentcode(')+'\n'+function('static void delivery_header_restore(')+'\n'
code+=r'''
int main(void) {
 struct char_data ch={0};struct player_special_data ps={0};char line[512];int a,b,c,d,e,f;FILE *fp=tmpfile();ch.player_specials=&ps;
 strcpy(ps.clan_delivery_inventory,"dddddddddddddddddddddddddddddddd");ps.clan_delivery_inventory_stage=1;
 strcpy(ps.clan_qm_inventory,"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
 assert(objsave_write_rentcode(fp,1,0,&ch));rewind(fp);assert(fgets(line,sizeof(line),fp));fclose(fp);
 assert(sscanf(line,"%d %d %d %d %d %d",&a,&b,&c,&d,&e,&f)==6);
 *ps.clan_delivery_inventory=0;delivery_header_restore(&ch,line);
 assert(!strcmp(ps.clan_qm_inventory,"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
 assert(!strcmp(ps.clan_delivery_inventory,"dddddddddddddddddddddddddddddddd")&&ps.clan_delivery_inventory_stage==1);
 delivery_header_restore(&ch,"1 12345 0 0 0 0");assert(!*ps.clan_qm_inventory);assert(!*ps.clan_delivery_inventory&&ps.clan_delivery_inventory_stage==0);
 delivery_header_restore(&ch,"1 12345 0 0 0 0 - 0");assert(!*ps.clan_delivery_inventory);
 delivery_header_restore(&ch,"1 12345 0 0 0 0 invalid 1");assert(!strcmp(ps.clan_delivery_inventory,"invalid-operation"));
 delivery_header_restore(&ch,"1 12345 0 0 0 0 - 0 invalid");assert(!strcmp(ps.clan_qm_inventory,"invalid-operation"));
 puts("Delivery inventory header: paid marker round-trip, legacy six-field reader/writer compatibility, malformed marker rejection passed.");return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='delivery-header-') as directory:
    path=Path(directory);(path/'test.c').write_text(code)
    subprocess.run(['cc','-std=gnu99','-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-Isrc',str(path/'test.c'),'src/legendary_identity.c','-lcrypto','-o',str(path/'test')],cwd=root,check=True)
    subprocess.run([str(path/'test')],check=True)
