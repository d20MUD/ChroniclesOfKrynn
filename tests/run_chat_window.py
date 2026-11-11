#!/usr/bin/env python3
"""Production MSDP chat routing and per-reader OOC identity rules."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
def function(file,name):
 s=(root/file).read_text();a=s.index(name);a=s.rfind('\n',0,a)+1;return s[a:s.index('\n}',a)+2]
header=r'''
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <json-c/json.h>
#define eMSDP_CLIENT_ID 0
#define PRF_ANONYMOUS 1
struct variable {const char *pValueString;};
struct protocol {bool bMSDP;struct variable *pVariables[1];};
struct descriptor_data {struct protocol *pProtocol;};
struct char_data {bool npc,anonymous,visible,introduced;};
#define IS_NPC(ch) ((ch)->npc)
#define CAN_SEE(reader,ch) ((reader)->visible)
#define PRF_FLAGGED(ch,flag) ((ch)->anonymous)
#define GET_NAME(ch) "Gicker"
static bool has_intro(struct char_data *reader,struct char_data *speaker){(void)speaker;return reader->introduced;}
static const char *show_pers(struct char_data *speaker,struct char_data *reader){(void)speaker;return reader->visible?"a hooded traveler":"someone";}
#define TRUE true
#define ACMD(name) void name(struct char_data *ch,const char *argument,int cmd,int subcmd)
static int communication_output_depth,events;
ACMD(do_say);
static ACMD(do_say_communication_impl){(void)ch;(void)argument;(void)cmd;(void)subcmd;assert(communication_output_depth>0);return;}
static ACMD(do_gen_comm_communication_impl){assert(communication_output_depth==1);do_say(ch,argument,cmd,subcmd);assert(communication_output_depth==1);}
static char received[4096];
static void MSDPSendPair(struct descriptor_data *d,const char *key,const char *data){
 (void)d;assert(!strcmp(key,"CHAT_MESSAGE"));struct json_object *o=json_tokener_parse(data),*text;
 assert(o&&json_object_object_get_ex(o,"text",&text));snprintf(received,sizeof(received),"%s",json_object_get_string(text));json_object_put(o);events++;
}
'''
main=r'''
int main(void){
 struct variable client={"LuminariWebClient"};struct protocol p={true,{&client}};struct descriptor_data d={&p};
 assert(!send_communication_event(&d,"room output"));communication_output_depth=1;
 const char *colored="\x1b[36mYou say, 'hello \\\"friend\\\"'\x1b[0m\r\n";
 assert(send_communication_event(&d,colored));assert(!strcmp(received,colored));
 assert(send_communication_event(&d,colored)&&events==2); // Equal messages are events, not deduplicated state.
 p.bMSDP=false;assert(!send_communication_event(&d,colored));p.bMSDP=true;
 client.pValueString="telnet";assert(!send_communication_event(&d,colored));client.pValueString=NULL;assert(!send_communication_event(&d,colored));
 communication_output_depth=0;
 struct char_data speaker={0},reader={.visible=true};
 do_gen_comm(&speaker,"nested communication",0,0);assert(communication_output_depth==0);
 do_say(&speaker,"",0,0);assert(communication_output_depth==0);
 assert(!strcmp(chat_channel_identity(&speaker,&reader),"Gicker"));speaker.anonymous=true;
 assert(!strcmp(chat_channel_identity(&speaker,&reader),"a hooded traveler"));reader.introduced=true;
 assert(!strcmp(chat_channel_identity(&speaker,&reader),"Gicker"));reader.visible=false;
 assert(!strcmp(chat_channel_identity(&speaker,&reader),"someone"));
 reader.visible=true;speaker.npc=true;assert(!strcmp(chat_channel_identity(&speaker,&reader),"a hooded traveler"));
 puts("MSDP chat: event repetition, colors, fallback and per-recipient anonymous identity passed");
}
'''
with tempfile.TemporaryDirectory(prefix='chat-window-') as tmp:
 p=Path(tmp);(p/'test.c').write_text(header+function('src/comm.c','static bool send_communication_event(')+function('src/act.comm.c','static const char *chat_channel_identity(')+function('src/act.comm.c','ACMD(do_say)')+function('src/act.comm.c','ACMD(do_gen_comm)')+main)
 subprocess.run(['cc','-std=gnu99','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-pie','-no-pie',str(p/'test.c'),'-ljson-c','-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
