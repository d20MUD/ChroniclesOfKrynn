#!/usr/bin/env python3
"""Exercise native who filters and structured row disclosure together."""
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'src/act.informative.c').read_text()
a=s.index('ACMD(do_who_text)')
b=s.index('      /* Only anonymous players require an introduction on the who list. */',a)
a=s.rfind('      if (d->original)',a,b)
selection=s[a:b]
code=r'''
#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
#include <stdio.h>
#include <json-c/json.h>
#define CAMPAIGN_DL
#define TRUE true
#define LVL_IMMORT 31
#define NO_CLAN -1
#define PRF_ANONYMOUS 1
#define PRF_QUEST 2
#define PRF_AFK 4
#define PRF_RP 8
#define PLR_KILLER 16
#define PLR_THIEF 32
#define GET_LEVEL(c) ((c)->level)
#define GET_NAME(c) ((c)->name)
#define GET_TITLE(c) ((c)->title)
#define PRF_FLAGGED(c,f) (((c)->flags&(f))!=0)
#define PLR_FLAGGED(c,f) PRF_FLAGGED(c,f)
#define CAN_SEE(c,t) ((t)->visible)
#define IN_ROOM(c) ((c)->room)
#define GET_CLASS(c) ((c)->classnum)
#define GET_CLAN(c) ((c)->clan)
#define GET_CLANRANK(c) ((c)->clanrank)
#define GROUP(c) ((c)->group)
#define GROUP_LEADER(g) ((g)->leader)
#define CLAN_NAME(c) "Faction"
#define PERS(t,c) ((t)->known ? (t)->name : "an unknown traveler")
#define str_cmp strcasecmp
struct group { struct char_data *leader; };
struct char_data { char *name,*title; int flags,level,room,classnum,clan,clanrank; bool visible,known; struct group *group; struct descriptor_data *desc; };
struct output_capture { struct json_object *web_data; bool truncated; };
struct descriptor_data { struct char_data *original,*character; bool playing; struct output_capture *output_capture; };
#define IS_PLAYING(d) ((d)->playing)
static bool identity_known(struct char_data *ch,struct char_data *tch) { return tch->known; }
static int perceived_race(struct char_data *tch,struct char_data *ch) { return 0; }
static int real_clan(int clan) { return clan; }
static struct { const char *type; } race_list[]={{"Human"}};
static struct { int zone; } world[2]={{1},{2}};
static int low=0,high=100,short_list,outlaws,questwho,localwho,who_room,showclass,showrace,showgroup,showleader;
static char name_search[128];
static struct { int min_level,max_level; } rank[]={{0,100}};
static struct json_object *selected(struct char_data *ch,struct descriptor_data *d) {
 struct char_data *tch; int i=0,c_n;
 for(int once=0;once<1;once++) {
''' + selection + r'''
 }
 struct json_object *players;
 assert(json_object_object_get_ex(ch->desc->output_capture->web_data,"players",&players));
 return players;
}
static void clear(struct output_capture *capture) { json_object_object_add(capture->web_data,"players",json_object_new_array()); }
int main(void) {
 struct output_capture capture={.web_data=json_object_new_object()};
 struct descriptor_data viewer_descriptor={.output_capture=&capture}, target_descriptor={.playing=true};
 struct char_data viewer={.desc=&viewer_descriptor}, target={.name="Secretname",.title="Secret Title",.level=12,.visible=true,.clan=1,.clanrank=1,.flags=PRF_ANONYMOUS};
 target_descriptor.character=&target;
 clear(&capture);struct json_object *players=selected(&viewer,&target_descriptor),*entry,*value;
 assert(json_object_array_length(players)==1);entry=json_object_array_get_idx(players,0);
 assert(!json_object_object_get_ex(entry,"level",&value));
 assert(!json_object_object_get_ex(entry,"race",&value));
 assert(!json_object_object_get_ex(entry,"faction",&value));
 assert(!strstr(json_object_to_json_string(entry),"Secret"));
 target.known=true;clear(&capture);players=selected(&viewer,&target_descriptor);entry=json_object_array_get_idx(players,0);
 assert(json_object_object_get_ex(entry,"level",&value)&&json_object_get_int(value)==12);
 assert(json_object_object_get_ex(entry,"faction",&value));
 target.level=40;clear(&capture);players=selected(&viewer,&target_descriptor);entry=json_object_array_get_idx(players,0);
 assert(!json_object_object_get_ex(entry,"level",&value) && !json_object_object_get_ex(entry,"race",&value));target.level=12;
 target.visible=false;clear(&capture);assert(!json_object_array_length(selected(&viewer,&target_descriptor)));target.visible=true;
 who_room=1;target.room=1;clear(&capture);assert(!json_object_array_length(selected(&viewer,&target_descriptor)));who_room=0;target.room=0;
 low=20;clear(&capture);assert(!json_object_array_length(selected(&viewer,&target_descriptor)));low=0;
 outlaws=1;clear(&capture);assert(!json_object_array_length(selected(&viewer,&target_descriptor)));outlaws=0;
 target_descriptor.playing=false;clear(&capture);assert(!json_object_array_length(selected(&viewer,&target_descriptor)));target_descriptor.playing=true;
 target.known=false;strcpy(name_search,"Secretname");clear(&capture);assert(!json_object_array_length(selected(&viewer,&target_descriptor)));
 json_object_put(capture.web_data);
 puts("Structured who: visibility, anonymous identities, introductions, room/level/outlaw filters and playing state passed");
}
'''
with tempfile.TemporaryDirectory(prefix='web-who-') as d:
 c=Path(d)/'who.c';binary=Path(d)/'who';c.write_text(code)
 subprocess.run(['cc','-std=gnu99','-fsanitize=address,undefined',str(c),'-ljson-c','-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)
