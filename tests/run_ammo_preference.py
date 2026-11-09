#!/usr/bin/env python3
"""Exercise the real preference command and its player-file field round trip."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'src/act.offensive.c').read_text();a=s.index('ACMD(do_ammopref)');command=s[a:s.index('\n}',a)+2]
p=(root/'src/players.c').read_text();save=next(l for l in p.splitlines() if 'BUFFER_WRITE("AmPr:' in l);load=next(l for l in p.splitlines() if 'strcmp(tag, "AmPr")' in l)
code=r'''
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "handler.h"
#include "comm.h"
#include "db.h"
#include <assert.h>
#define MAX(a,b) ((a)>(b)?(a):(b))
#undef ACMD
#define ACMD(name) void name(struct char_data *ch,const char *argument,int cmd,int subcmd)
#define PREREQ_NOT_NPC() do { if(IS_NPC(ch))return; } while(0)
static struct obj_data selected,pouch;
struct index_data *obj_index;
struct obj_data *obj_proto;
obj_rnum real_object(obj_vnum vnum){return vnum==1234?0:NOTHING;}
obj_rnum top_of_objt;
static int saves,last_saved;
static char message[8192];
void save_char(struct char_data *ch,int mode){saves++;last_saved=GET_PREFERRED_AMMO(ch);}
size_t send_to_char(struct char_data *ch,const char *format,...){va_list ap;va_start(ap,format);vsnprintf(message+strlen(message),sizeof(message)-strlen(message),format,ap);va_end(ap);return strlen(message);}
void skip_spaces_c(const char **str){while(**str==' ')(*str)++;}
size_t strlcpy(char *dest,const char *src,size_t size){size_t n=strlen(src);if(size)snprintf(dest,size,"%s",src);return n;}
struct obj_data *get_obj_in_list_vis(struct char_data *ch,char *name,int *number,struct obj_data *list){return !strcmp(name,"flaming arrows")?list:NULL;}
''' + command + r'''
static void roundtrip(struct char_data *ch) {
 char buffer[128],tag[5]="AmPr",line[32];
 #define BUFFER_WRITE(...) snprintf(buffer,sizeof(buffer),__VA_ARGS__)
''' + save + r'''
 GET_PREFERRED_AMMO(ch)=0;
 assert(sscanf(buffer,"AmPr: %31s",line)==1);
''' + load + r'''
}
int main(void){
 static struct char_data ch;static struct player_special_data special;struct index_data prototypes[1]={{0}};
 ch.player_specials=&special;obj_index=prototypes;top_of_objt=0;prototypes[0].vnum=1234;
 GET_OBJ_TYPE(&selected)=ITEM_MISSILE;GET_OBJ_RNUM(&selected)=0;selected.short_description="flaming arrows";obj_proto=&selected;
 GET_EQ(&ch,WEAR_AMMO_POUCH)=&pouch;pouch.contains=&selected;
 do_ammopref(&ch,"flaming arrows",0,0);assert(saves==1 && last_saved==1235 && GET_PREFERRED_AMMO(&ch)==1235);
 assert(strstr(message,"flaming arrows"));do_ammopref(&ch,"",0,0);assert(strstr(message,"flaming arrows"));roundtrip(&ch);assert(GET_PREFERRED_AMMO(&ch)==1235);
 do_ammopref(&ch,"missing",0,0);assert(saves==1 && GET_PREFERRED_AMMO(&ch)==1235);
 do_ammopref(&ch,"none",0,0);assert(saves==2 && last_saved==0);
 do_ammopref(&ch,"",0,0);assert(strstr(message,"Usage: ammopref") && saves==2);
 GET_EQ(&ch,WEAR_AMMO_POUCH)=NULL;do_ammopref(&ch,"flaming arrows",0,0);assert(saves==2);
 puts("Ammo preference: exact pouch choice, errors, clearing, saved value and player-file round trip passed");
}
'''
with tempfile.TemporaryDirectory(prefix='ammo-preference-') as directory:
 c=Path(directory)/'pref.c';binary=Path(directory)/'pref';c.write_text(code)
 subprocess.run(['cc','-std=gnu99','-I'+str(root/'src'),'-fsanitize=address,undefined',str(c),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)
