#!/usr/bin/env python3
"""Check creation help aliases and captured ability text using production C."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
def function(file, signature):
    source = (root / file).read_text()
    start = source.index(signature)
    return source[start:source.index('\n}', start) + 2]

code = r'''
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "help.h"
#include "handler.h"
#include "comm.h"
#include "modify.h"
#include "interpreter.h"
#include <assert.h>
#undef log
#define log(...) assert(0)
static struct help_entry_list *alloc_help_entry(void) { return calloc(1, sizeof(struct help_entry_list)); }
static void free_help_entry_single(struct help_entry_list *e) {
  free(e->tag); free(e->entry); free(e->keywords); free(e->last_updated);
}
static char captured[4096];
size_t send_to_char(struct char_data *ch, const char *format, ...) {
  va_list args; va_start(args,format); vsnprintf(captured,sizeof(captured),format,args); va_end(args); return strlen(captured);
}
void page_string(struct descriptor_data *d, char *str, int keep) { assert(0 && "creation help must bypass paging"); }
'''
code += '\n' + function('src/handler.c', 'int is_name(')
code += '\n' + function('src/help.c', 'static struct help_entry_list *search_help_table(')
code += r'''
struct help_entry_list *search_help(const char *argument, int level) { return search_help_table(argument, level); }
'''
code += '\n' + function('src/help.c', 'void perform_help(')
code += r'''
struct help_index_element *help_table;
int top_of_helpt;
int main(void) {
  struct help_index_element entries[2] = {0};
  struct descriptor_data d = {0}; struct char_data ch = {0};
  struct help_entry_list *result;
  entries[0].keywords = "SECRET CLASS-WARRIOR"; entries[0].entry = "Restricted"; entries[0].min_level = LVL_IMPL + 1;
  entries[1].keywords = "WARRIOR FIGHTER CLASS-WARRIOR";
  entries[1].entry = ">Class Abilities:\nWarrior feat progression\n>Proficiencies:\nHeavy Armor\nTower Shields\n";
  help_table = entries; top_of_helpt = 2;
  assert(!search_help_table("class-missing", LVL_IMPL));
  result = search_help_table("ClAsS-WaRrIoR", LVL_IMPL); assert(result);
  assert(!strcmp(result->entry, entries[1].entry)); free_help_entry_single(result); free(result);
  d.character = &ch; ch.desc = &d; STATE(&d) = CON_QCLASS;
  perform_help(&d, "class-warrior"); assert(!strcmp(captured, entries[1].entry));
  STATE(&d) = CON_QRACE; entries[1].keywords = "HUMAN RACE-HUMAN";
  perform_help(&d, "race-human"); assert(!strcmp(captured, entries[1].entry));
  return 0;
}
'''
with tempfile.TemporaryDirectory() as directory:
    source = Path(directory) / 'creation_help.c'
    binary = Path(directory) / 'creation_help'
    source.write_text(code)
    subprocess.run(['gcc', '-std=gnu99', '-g', '-fsanitize=address', '-I', str(root / 'src'), str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('Creation help alias lookup and captured class/race information passed')
