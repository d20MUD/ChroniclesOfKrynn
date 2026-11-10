#!/usr/bin/env python3
"""Run all native crafting description setters with visible-word validation."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'src/crafting_new.c').read_text()
def function(signature):
 a=s.index(signature);return s[a:s.index('\n}',a)+2]
code=r'''
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#define MAX_INPUT_LENGTH 2048
#define MAX_EXTRA_DESC 4096
#define COLOUR_CHAR '^'
struct craft_data {int crafting_item_type,crafting_specific,craft_variant,crafting_recipe;int materials[2][2];char *keywords,*short_description,*room_description,*ex_description;};
struct char_data{struct craft_data craft;};
#define GET_CRAFT(ch) ((ch)->craft)
static struct {const char *variant_descriptions[2];int materials[1][2][1];} crafting_recipes[2];
static const char *crafting_material_descriptions[]={"steel","cold iron"};
static char output[8192];
size_t send_to_char(struct char_data *ch,const char *format,...){
 va_list args;va_start(args,format);int n=vsnprintf(output,sizeof(output),format,args);va_end(args);return n;
}
'''
helpers=(root/'src/helpers.c').read_text()
a=helpers.index('void parse_at(');code+=helpers[a:helpers.index('\n}',a)+2]
code+=function('static void show_craft_description(')
for signature in ['static bool crafting_description_has_required_words(', 'void set_crafting_keywords(', 'void set_crafting_short_desc(', 'void set_crafting_room_desc(', 'void set_crafting_extra_desc(']:code+=function(signature)
code+=r'''
int main(void){
 struct char_data ch={0};ch.craft.crafting_item_type=1;ch.craft.crafting_specific=1;ch.craft.craft_variant=0;ch.craft.crafting_recipe=1;ch.craft.materials[0][0]=0;ch.craft.materials[0][1]=1;
 crafting_recipes[1].variant_descriptions[0]="long sword";
 void (*setters[])(struct char_data *,const char *)={set_crafting_keywords,set_crafting_short_desc,set_crafting_room_desc};
 char **fields[]={&ch.craft.keywords,&ch.craft.short_description,&ch.craft.room_description};
 const char *valid[]={"steel long sword", "@Rst@neel @Clong @nsword", "\tRst\tneel \tClong \tnsword", "^Rst^neel ^Clong ^nsword", "\033[31mst\033[0meel long \033[1;36msw\033[0mord", "\t[f500]st\t[b123]eel long sword\tn", "steel long sword@@", "steel long sword^^", "steel long sword@", "steel long sword\t"};
 const char *invalid[]={"@Rsteel@N dagger", "\tRiron\tn long sword", "^Rsteel^n sword", "\t[f500]long sword", "long sword \t[fsteel]", "long sword \t[fsteel", "\033[38;2;123;255;0msteel\033[0m"};
 for(int setter=0;setter<3;setter++) {
  for(unsigned i=0;i<sizeof(valid)/sizeof(*valid);i++){
   char input[256];snprintf(input,sizeof(input),"%s",valid[i]);setters[setter](&ch,input);
   char parsed[256];strcpy(parsed,valid[i]);parse_at(parsed);
   char expected[512];snprintf(expected,sizeof(expected),"-- %s\tn\r\n",parsed);assert(strstr(output,expected));
   assert(*fields[setter] && !strcmp(*fields[setter],valid[i]));assert(!strcmp(input,valid[i]));free(*fields[setter]);*fields[setter]=NULL;
  }
  for(unsigned i=0;i<sizeof(invalid)/sizeof(*invalid);i++){setters[setter](&ch,invalid[i]);assert(!*fields[setter]);}
  // The existing length/project/material guards still apply.
  char long_input[160];memset(long_input,'a',sizeof(long_input)-1);long_input[sizeof(long_input)-1]=0;
  setters[setter](&ch,long_input);assert(!*fields[setter]);
  ch.craft.materials[0][1]=0;setters[setter](&ch,"steel long sword");assert(!*fields[setter]);ch.craft.materials[0][1]=1;
 }
 // Extra descriptions render colors while preserving their stored markup.
 ch.craft.keywords=strdup("steel long sword");
 set_crafting_extra_desc(&ch,"@Wa steel @Clong sword@n, 100% handcrafted");
 assert(strstr(output,"-- \tWa steel \tClong sword\tn, 100% handcrafted\tn\r\n"));
 assert(!strcmp(ch.craft.ex_description,"@Wa steel @Clong sword@n, 100% handcrafted"));
 free(ch.craft.keywords);free(ch.craft.ex_description);
 // Multiword armor and material names can have colors in the middle too.
 crafting_recipes[1].variant_descriptions[0]="full plate";ch.craft.materials[0][0]=1;
 set_crafting_short_desc(&ch,"a ^Rcold ^niron full ^Cplate^n");assert(ch.craft.short_description);free(ch.craft.short_description);
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='craft-desc-colors-') as tmp:
 p=Path(tmp)/'test.c';binary=Path(tmp)/'test';p.write_text(code)
 subprocess.run(['cc','-fsanitize=address,undefined','-g','-std=gnu99',str(p),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)
print('Craft description validation: all three setters, @/tab/^/ANSI/RGB colors inside required words, unchanged stored colors, escaped/trailing markers, missing terms, multiword materials/types and existing guards passed.')
