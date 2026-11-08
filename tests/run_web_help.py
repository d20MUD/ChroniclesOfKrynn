#!/usr/bin/env python3
"""Exercise production help capture, pager bypass, response IDs and opt-in."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
def function(file, name):
    source = (root / file).read_text()
    start = source.index(name)
    return source[start:source.index('\n}', start) + 2]

code = r'''
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "modify.h"
#include "protocol.h"
#include <json-c/json.h>
#include <assert.h>
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
#undef CREATE
#define CREATE(p,t,n) do { (p)=calloc((n),sizeof(t));assert(p); } while(0)
#undef GET_PAGE_LENGTH
static int page_length=40;
#define GET_PAGE_LENGTH(ch) page_length
#undef ACMDU
#define ACMDU(name) void name(struct char_data *ch,const char *argument,int cmd,int subcmd)
static int terminal_writes, pager_calls;
static char *response;
static char *body;
static struct descriptor_data descriptor;
static struct char_data character;
static protocol_t protocol;
static MSDP_t help_variable;
static MSDP_t *variables[eMSDP_MAX];
void MSDPSetString(struct descriptor_data *d,variable_t var,const char *text) {
  assert(!d->output_capture);assert(var==eMSDP_HELP_INFO);
  free(response);response=strdup(text);
}
void MSDPFlush(struct descriptor_data *d,variable_t var) { assert(help_variable.bDirty); }
static int count_pages(char *str,struct char_data *ch) { pager_calls++;return 1; }
void paginate_string(char *str,struct descriptor_data *d) { pager_calls++; }
void show_string(struct descriptor_data *d,char *input) { pager_calls++; }
'''
code += '\n' + function('src/comm.c', 'static void capture_output_printf(')
code += r'''
size_t write_to_output(struct descriptor_data *d,const char *format,...) {
  va_list args;va_start(args,format);
  if(d->output_capture)capture_output_printf(d->output_capture,format,args);
  else terminal_writes++;
  va_end(args);return 100;
}
'''
code += '\n' + function('src/modify.c', 'void page_string(')
code += r'''
static void do_help_text(struct char_data *ch,const char *argument,int cmd,int subcmd) {
  if(ch->desc->output_capture) {
    write_to_output(ch->desc,"Help for %s\n",argument);
    page_string(ch->desc,body,1);
  } else write_to_output(ch->desc,"Normal terminal help");
}
size_t strlcpy(char *dst,const char *src,size_t size) {
  size_t length=strlen(src);if(size){size_t n=length<size-1?length:size-1;memcpy(dst,src,n);dst[n]=0;}return length;
}
'''
code += '\n' + function('src/help.c', 'ACMDU(do_help)\n{')
code += r'''
static struct json_object *field(struct json_object *o,const char *key) {
  struct json_object *value;assert(json_object_object_get_ex(o,key,&value));return value;
}
int main(void) {
  character.desc=&descriptor;descriptor.character=&character;
  descriptor.pProtocol=&protocol;protocol.pVariables=variables;
  variables[eMSDP_HELP_INFO]=&help_variable;
  body=malloc(OUTPUT_CAPTURE_LIMIT*2+1);memset(body,'x',OUTPUT_CAPTURE_LIMIT*2);body[OUTPUT_CAPTURE_LIMIT*2]=0;
  protocol.bMSDP=true;help_variable.bReport=false;
  do_help(&character,"ranged",0,0);assert(terminal_writes==1 && !response);
  help_variable.bReport=true;descriptor.showstr_count=7;descriptor.showstr_page=3;
  do_help(&character,"ranged",0,0);
  assert(!descriptor.output_capture);assert(!pager_calls);assert(terminal_writes==1);
  assert(descriptor.showstr_count==7 && descriptor.showstr_page==3);
  assert(strlen(response)<MAX_VARIABLE_LENGTH);
  struct json_object *decoded=json_tokener_parse(response);assert(decoded);
  assert(!strcmp(json_object_get_string(field(decoded,"topic")),"ranged"));
  assert(json_object_get_boolean(field(decoded,"truncated")));
  assert(strlen(json_object_get_string(field(decoded,"body")))==OUTPUT_CAPTURE_LIMIT);
  char *previous_id=strdup(json_object_get_string(field(decoded,"id")));json_object_put(decoded);
  free(body);body=strdup("Quotes: \" \\ \n\tRred\tn <script>unsafe</script>");
  do_help(&character,"ranged",0,0);
  decoded=json_tokener_parse(response);assert(decoded);
  assert(strcmp(previous_id,json_object_get_string(field(decoded,"id"))));
  assert(!json_object_get_boolean(field(decoded,"truncated")));
  assert(strstr(json_object_get_string(field(decoded,"body")),body));
  assert(!descriptor.output_capture && !pager_calls);
  json_object_put(decoded);free(previous_id);free(body);free(response);
  puts("Web help capture, pager bypass, limits, JSON and terminal fallback passed");
}
'''
with tempfile.TemporaryDirectory() as temp:
    c = Path(temp) / 'help.c'
    exe = Path(temp) / 'help'
    c.write_text(code)
    subprocess.run(['cc', '-std=gnu11', '-I'+str(root/'src'), str(c), '-ljson-c', '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
