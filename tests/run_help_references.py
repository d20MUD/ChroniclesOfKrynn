#!/usr/bin/env python3
"""Execute See Also parsing and access-aware reference resolution from help.c."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'src/help.c').read_text();a=s.index('/* Resolve references through');b=s.index('/* REPORT HELP_INFO',a)
code=r'''
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "help.h"
#include <json-c/json.h>
#include <assert.h>
size_t strlcpy(char *dst,const char *src,size_t size){size_t n=strlen(src);if(size){size_t copy=n<size-1?n:size-1;memcpy(dst,src,copy);dst[copy]=0;}return n;}
void space_to_minus(char *text){for(;*text;text++)if(*text==' ')*text='-';}
static void free_help_entry_list(struct help_entry_list *entry){if(!entry)return;free(entry->tag);free(entry->keywords);free(entry->last_updated);free(entry);}
struct help_entry_list *search_help(const char *topic,int level){
 struct help_entry_list *entry;
 if(strcmp(topic,"CRAFTING") && strcmp(topic,"LITANY-OF-RIGHTEOUSNESS") && strcmp(topic,"FILEALIAS") && strcmp(topic,"STAFF") && strcmp(topic,"PARTIAL"))return NULL;
 if(!strcmp(topic,"STAFF") && level<LVL_IMMORT)return NULL;
 entry=calloc(1,sizeof(*entry));
 entry->tag=strdup(!strcmp(topic,"PARTIAL")?"PARTIAL-LONGER":topic);
 if(!strcmp(topic,"FILEALIAS")){free(entry->tag);entry->tag=strdup("FILEPRIMARY FILEALIAS");entry->last_updated=strdup("File-based");entry->keywords=strdup("FILEPRIMARY FILEALIAS");}
 return entry;
}
static int native(struct char_data *ch,const char *arg,const char *raw,struct help_context *ctx){assert(ch->desc->output_capture);return !strcmp(raw,"manyshot");}
static int forbidden(struct char_data *ch,const char *arg,const char *raw,struct help_context *ctx){assert(0);return 0;}
static struct help_handler suggestions={"soundex",forbidden,NULL}, feat={"feat",native,&suggestions},database={"database",forbidden,&feat};
static struct help_handler *help_handler_chain=&database;
void init_help_handlers(void){assert(0);}
int spellhelp_try_show(struct char_data *ch,const char *topic){assert(ch->desc->output_capture);return !strcmp(topic,"fireball");}
'''+s[a:b]+r'''
int main(void){
 struct char_data ch={0};struct descriptor_data d={0};struct json_object *section,*entries,*entry,*value;ch.desc=&d;GET_LEVEL(&ch)=1;
 const char *body="Intro\r\n\r\n\tySee Also:\tn CRAFTING, LITANY OF RIGHTEOUSNESS,\r\n manyshot, fireball, MISSING, STAFF, FILEALIAS, PARTIAL.\r\n\r\nNext section: keep this.\r\n";
 section=web_help_see_also(&ch,body);assert(section);assert(!d.output_capture);
 assert(json_object_object_get_ex(section,"entries",&entries));assert(json_object_array_length(entries)==8);
 for(int i=0;i<8;i++){entry=json_object_array_get_idx(entries,i);bool linked=json_object_object_get_ex(entry,"topic",&value);assert(linked==(i<4 || i==6));}
 assert(json_object_object_get_ex(section,"text",&value));assert(strstr(json_object_get_string(value),"See Also"));assert(!strstr(json_object_get_string(value),"Next section"));json_object_put(section);
 GET_LEVEL(&ch)=LVL_IMMORT;assert(web_help_reference_exists(&ch,"STAFF"));
 assert(!web_help_see_also(&ch,"No references here."));assert(!web_help_see_also(&ch,NULL));
 section=web_help_see_also(&ch,"\033[36mSEE ALSO:\033[0m\n\nCRAFTING\n");assert(section);json_object_put(section);
 struct output_capture outer={0};d.output_capture=&outer;assert(web_help_reference_exists(&ch,"manyshot"));assert(d.output_capture==&outer);d.output_capture=NULL;
 char huge[1000]="See also: ";for(int i=0;i<40;i++)strcat(huge,"MISSING,");assert(!web_help_see_also(&ch,huge));
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='help-refs-') as tmp:
 p=Path(tmp)/'test.c';binary=Path(tmp)/'test';p.write_text(code)
 subprocess.run(['cc','-fsanitize=address,undefined','-g','-std=gnu99','-I'+str(root/'src'),str(p),'-ljson-c','-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)
print('Help references: colored headings, wrapped multiword topics, exact aliases, missing/restricted topics, native feat/spell help, capture isolation, footer preservation and bounds passed.')
