#!/usr/bin/env python3
"""Run score text/modal descriptions with the real appearance selection functions."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
score=(root/'src/act.informative.c').read_text()
descriptions=(root/'src/char_descs.c').read_text()
def function(text,signature):
 a=text.index(signature);return text[a:text.index('\n}',a)+2]
assert 'score_short_descriptions(ch, FALSE);' in function(score,'ACMD(do_score_text)')
assert 'score_short_descriptions(ch, TRUE);' in function(score,'ACMD(do_score)')
assert 'do_score_text(ch, argument, cmd, subcmd);' in function(score,'ACMD(do_score)')
code=r'''
#include <assert.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#define NUM_EXTENDED_RACES 4
#define NUM_RACE_TYPES 3
#define MAX_PC_SUBRACES 3
#define RACE_TYPE_ANIMAL 1
#define MAX_INPUT_LENGTH 2048
struct disguise_appearance {int race,age,sex,descriptor1,adjective1,descriptor2,adjective2;};
struct form {const char *short_desc;};
struct char_data {
 bool npc,disguised,alter_self,irda,sivak,wild;
 int race,real_race,age,sex,descriptor1,adjective1,descriptor2,adjective2,altered_race,morphed,subrace;
 struct disguise_appearance costume,irda_form;
 struct form sivak_form;
};
struct {const char *name;} race_list[NUM_EXTENDED_RACES]={{"human"},{"wolf"},{"elf"},{"ogre"}};
const char *shape_types[MAX_PC_SUBRACES]={"unknown","bear","cat"};
const char *race_family_types[NUM_RACE_TYPES]={"unknown","animal","dragon"};
#define IS_NPC(ch) ((ch)->npc)
#define HAS_DISGUISE(ch) ((ch)->disguised)
#define IS_ALTER_SELF(ch) ((ch)->alter_self)
#define IS_IRDA_FORM(ch) ((ch)->irda)
#define IS_SIVAK_FORM(ch) ((ch)->sivak)
#define IS_MORPHED(ch) ((ch)->morphed)
#define GET_DISGUISE_RACE(ch) ((ch)->altered_race)
#define AFF_FLAGGED(ch,flag) ((ch)->wild)
#define GET_NAME(ch) "a guard"
#define GET_RACE(ch) ((ch)->race)
#define GET_REAL_RACE(ch) ((ch)->real_race)
#define GET_CH_AGE(ch) ((ch)->age)
#define GET_SEX(ch) ((ch)->sex)
#define GET_PC_DESCRIPTOR_1(ch) ((ch)->descriptor1)
#define GET_PC_ADJECTIVE_1(ch) ((ch)->adjective1)
#define GET_PC_DESCRIPTOR_2(ch) ((ch)->descriptor2)
#define GET_PC_ADJECTIVE_2(ch) ((ch)->adjective2)
#define SUBRACE(ch) ((ch)->subrace)
#define IRDA_APPEARANCE(ch) ((ch)->irda_form)
#define COSMETIC_DISGUISE(ch) ((ch)->costume)
#define active_sivak_form(ch) (&(ch)->sivak_form)
#define AN(name) (strchr("aeiouAEIOU",*(name)) ? "an" : "a")
static char output[8192],labels[2][64],values[2][2048];
static int fields;
size_t send_to_char(struct char_data *ch,const char *format,...) {
 va_list args;va_start(args,format);int n=vsnprintf(output+strlen(output),sizeof(output)-strlen(output),format,args);va_end(args);return n;
}
void web_info_field(struct char_data *ch,const char *section,const char *label,const char *format,...) {
 assert(!strcmp(section,"Identity") && fields<2);
 strcpy(labels[fields],label);
 va_list args;va_start(args,format);vsnprintf(values[fields],sizeof(values[fields]),format,args);va_end(args);fields++;
}
char *format_short_desc(int race,int age,int sex,int d1,int a1,int d2,int a2) {
 char buf[512];snprintf(buf,sizeof(buf),"%s/%d/%d/%d/%d/%d/%d",race_list[race].name,age,sex,d1,a1,d2,a2);return strdup(buf);
}
'''
for sig in ['char *current_short_desc(', 'char *current_true_desc(', 'bool has_altered_form(', 'char *current_altered_desc(', 'char *current_disguise_desc(']:
 code+=function(descriptions,sig)+'\n'
code+=function((root/'src/helpers.c').read_text(),'void parse_at(')+'\n'
code+=function(score,'static void score_short_descriptions(')+'\n'
code+=r'''
static void check(struct char_data *ch,const char *base,const char *current) {
 output[0]=0;fields=0;score_short_descriptions(ch,false);
 assert(fields==0);char expected[4096];snprintf(expected,sizeof(expected),"\tcShort description: \tn%s\tn\r\n",base);
 if(current) {strcat(expected,"\tcCurrent short description: \tn");strcat(expected,current);strcat(expected,"\tn\r\n");}
 assert(!strcmp(output,expected));
 output[0]=0;score_short_descriptions(ch,true);
 assert(!*output && fields==(current?2:1));
 assert(!strcmp(labels[0],"Short description") && !strcmp(values[0],base));
 if(current) assert(!strcmp(labels[1],"Current short description") && !strcmp(values[1],current));
}
int main(void) {
 struct char_data ch={0};ch.age=20;ch.sex=1;ch.descriptor1=1;ch.adjective1=2;ch.descriptor2=3;ch.adjective2=4;
 const char *base="human/20/1/1/2/3/4";
 check(&ch,base,NULL);
 ch.wild=true;ch.altered_race=1;ch.race=1;check(&ch,base,"a wolf");
 ch.wild=false;ch.altered_race=0;ch.morphed=2;check(&ch,base,"a dragon");
 ch.morphed=1;ch.subrace=1;check(&ch,base,"a bear");ch.morphed=0;
 ch.disguised=true;ch.costume=(struct disguise_appearance){2,40,2,5,6,7,8};
 check(&ch,base,"elf/40/2/5/6/7/8");
 ch.wild=true;ch.altered_race=1;check(&ch,base,"elf/40/2/5/6/7/8"); // disguise over transformed body
 ch.wild=false;ch.altered_race=0;ch.disguised=false;ch.alter_self=true;ch.race=2;
 check(&ch,base,"elf/20/1/1/2/3/4");ch.alter_self=false;
 ch.irda=true;ch.irda_form=(struct disguise_appearance){2,50,2,5,6,7,8};check(&ch,base,"elf/50/2/5/6/7/8");ch.irda=false;
 ch.sivak=true;ch.sivak_form.short_desc="@Ra silver ogre@n, 100% convincing";
 check(&ch,base,"\tRa silver ogre\tn, 100% convincing");
 assert(!strcmp(ch.sivak_form.short_desc,"@Ra silver ogre@n, 100% convincing"));ch.sivak=false;
 ch.disguised=true;ch.costume=(struct disguise_appearance){0,20,1,1,2,3,4};check(&ch,base,NULL); // suppress equal descriptions
 ch.npc=true;check(&ch,"a guard",NULL);
 puts("PASS: score text/modal parity, normal and differing descriptions, wild shape, polymorph, disguise precedence, alter self, Irda/Sivak forms, colors, percent signs, equal-description suppression and NPC safety.");
}
'''
with tempfile.TemporaryDirectory(prefix='score-descriptions-') as directory:
 p=Path(directory);(p/'test.c').write_text(code)
 subprocess.run(['cc','-std=gnu99','-g','-fsanitize=address,undefined','-fno-pie','-no-pie',str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
