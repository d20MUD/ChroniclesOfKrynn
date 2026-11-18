#!/usr/bin/env python3
"""Run the production domains command against captured player output."""
import re
import subprocess
import tempfile
from pathlib import Path

root = Path(__file__).resolve().parents[1]
source = (root / 'src/domains_schools.c').read_text()
command = source[source.index('ACMD(do_domains)'):]
names = re.findall(r'add_domain\(\s*DOMAIN_\w+,\s*"([^"]+)"', source)
names = [name for name in names if name != 'none']
assert len(names) == 15
preamble = r'''
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#define ACMD(n) void n(struct char_data *ch, const char *argument)
#define MAX_STRING_LENGTH 16384
#define MAX_INPUT_LENGTH 1024
#define NUM_DOMAINS 16
#define MAX_GRANTED_POWERS 5
#define MAX_DOMAIN_SPELLS 9
#define DOMAIN_UNDEFINED 0
#define DOMAIN_POWER_UNDEFINED 0
#define SPELL_RESERVED_DBC 0
#define CLASS_CLERIC 0
#define CLASS_INQUISITOR 1
#define QCYN ""
#define QNRM ""
#define QGRN ""
#define QRED ""
#define IS_NPC(ch) ((ch)->npc)
#define GET_1ST_DOMAIN(ch) ((ch)->first)
#define GET_2ND_DOMAIN(ch) ((ch)->second)
#define CLASS_LEVEL(ch, cls) ((ch)->levels[cls])
#define BONUS_CASTER_LEVEL(ch, cls) 0
struct char_data { int npc, first, second, levels[2]; void *desc; };
struct { const char *name, *description; int favored_weapon, granted_powers[5], domain_spells[9]; } domain_list[16];
struct { const char *name; } weapon_list[1] = {{"test weapon"}};
struct { const char *name; int domain[16]; } spell_info[10];
const char *domainpower_names[] = {"none", "test power"};
static char output[32768];
void send_to_char(struct char_data *ch, const char *fmt, ...) {
  (void)ch; va_list ap; va_start(ap, fmt);
  vsnprintf(output + strlen(output), sizeof(output) - strlen(output), fmt, ap); va_end(ap);
}
void page_string(void *d, char *buf, int flag) { (void)d; (void)flag; strcat(output, buf); }
int str_cmp(const char *a, const char *b) { return strcasecmp(a,b); }
int is_abbrev(const char *a, const char *b) { return *a && strlen(a)<=strlen(b) && !strncasecmp(a,b,strlen(a)); }
void skip_spaces_c(const char **p) { while (isspace((unsigned char)**p)) ++*p; }
const char *one_argument(const char *p, char *arg, size_t size) {
  size_t n=0; skip_spaces_c(&p);
  while (*p && !isspace((unsigned char)*p)) { if(n+1<size) arg[n++]=*p; ++p; }
  arg[n]=0; skip_spaces_c(&p); return p;
}
int get_class_highest_circle(struct char_data *ch, int cls) { (void)ch; (void)cls; return 9; }
void run(struct char_data *ch, const char *s);
'''
main = r'''
void run(struct char_data *ch, const char *s) { output[0]=0; do_domains(ch,s); }
int main(void) {
  struct char_data pc = {0};
  const char *names[] = {NAMES};
  for (int i=1;i<16;i++) {
    domain_list[i].name=names[i-1]; domain_list[i].description="test description";
    domain_list[i].granted_powers[0]=1;
    for(int j=0;j<9;j++) {domain_list[i].domain_spells[j]=j+1;spell_info[j+1].name="test spell";spell_info[j+1].domain[i]=2*j+1;}
  }
  run(&pc, "");
  for(int i=0;i<15;i++) assert(strstr(output,names[i]));
  assert(strstr(output,"Air                   ") && strstr(output,"Earth"));
  assert(!strstr(output,"test description") && !strstr(output,"test spell"));
  assert(strstr(output,"domains info <name>"));
  run(&pc,"info hEaLiNg");
  assert(strstr(output,"Healing") && !strstr(output,"Knowledge"));
  assert(strstr(output,"test description") && strstr(output,"test weapon") && strstr(output,"test power"));
  assert(strstr(output,"Circle 1: test spell") && strstr(output,"Circle 9: test spell"));
  run(&pc,"info hea"); assert(strstr(output,"Healing"));
  run(&pc,"info e"); assert(strstr(output,"ambiguous"));
  run(&pc,"info missing"); assert(strstr(output,"No such domain"));
  run(&pc,"info"); assert(strstr(output,"Usage:"));
  run(&pc,"garbage"); assert(strstr(output,"Usage:"));
  run(&pc,"spells"); assert(strstr(output,"no deity domains"));
  pc.first=1; pc.levels[0]=17;
  run(&pc,"spells"); assert(strstr(output,"Air Domain:") && strstr(output,"Circle 9"));
  pc.npc=1;run(&pc,"spells"); assert(strstr(output,"NPCs"));
  puts("PASS: domains columns, specific info, errors, and own spell listing");
}
'''.replace('NAMES', ','.join('"'+name+'"' for name in names))
with tempfile.TemporaryDirectory(prefix='domains-command-') as temp:
    c = Path(temp) / 'command.c'
    binary = Path(temp) / 'command'
    c.write_text(preamble + command + main)
    subprocess.run(['cc', '-std=c99', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
                    str(c), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
