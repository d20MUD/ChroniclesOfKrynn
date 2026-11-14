#!/usr/bin/env python3
"""Exercise the actual application handler without touching game data."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/clan.c').read_text()
start = source.index('ACMD(do_clanapply)\n{')
handler = source[start:source.index('\n/* clan award command */', start)]
preamble = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#define ACMD(name) void name(struct char_data *ch, char *argument, int cmd, int subcmd)
#define GET_LEVEL(c) ((c)->level)
#define GET_GOLD(c) ((c)->gold)
#define GET_CLANRANK(c) ((c)->rank)
#define GET_NAME(c) "Applicant"
#define IS_IN_CLAN(c) ((c)->clan != -1)
#define LVL_IMMORT 100
#define NO_CLAN -1
#define NO_CLANRANK 0
#define QYEL ""
#define QNRM ""
#define CLAN_ERR_IMMORTAL_RESTRICTION "immortal"
#define CLAN_ERR_ALREADY_IN_CLAN "member"
#define CLAN_ERR_INVALID_CLAN "invalid"
#define CLAN_ERR_LEVEL_TOO_LOW "level"
#define CLAN_ERR_CANNOT_AFFORD "gold"
typedef int clan_rnum;
struct char_data { int level, gold, clan, rank; };
struct { int applev, appfee, treasure, vnum; const char *clan_name; } clan_list[1] = {{1,0,10,1,"Test"}};
static int economy_allowed, economy_checks, mutations, membership, saves;
bool clan_persistence_access(struct char_data *ch) { return true; }
bool clan_persistence_economy_available(struct char_data *ch) { economy_checks++; return economy_allowed; }
void send_to_char(struct char_data *ch, const char *format, ...) {}
int get_clan_by_name(char *s) { return strcmp(s,"Test") ? -1 : 0; }
void decrease_gold(struct char_data *ch, int n) { mutations++; ch->gold -= n; }
void mark_clan_modified(int n) { mutations++; }
bool save_single_clan(int n) { mutations++; return true; }
bool set_clan(struct char_data *ch, int n) { membership++; ch->clan=n; return true; }
bool save_char_checked(struct char_data *ch, int n) { saves++; return true; }
void log_clan_activity(int n, const char *format, ...) {}
'''
checks = r'''
int main(void) {
 struct char_data ch={10,100,-1,0};
 do_clanapply(&ch,"",0,0); assert(!economy_checks && !membership);
 do_clanapply(&ch,"Test",0,0);
 assert(membership==1 && saves==1 && ch.clan==1 && ch.rank==0);
 assert(ch.gold==100 && clan_list[0].treasure==10 && !mutations && !economy_checks);
 ch.clan=-1; clan_list[0].appfee=5;
 do_clanapply(&ch,"Test",0,0);
 assert(economy_checks==1 && membership==1 && ch.gold==100 && !mutations);
 economy_allowed=1;
 do_clanapply(&ch,"Test",0,0);
 assert(membership==2 && ch.gold==95 && clan_list[0].treasure==15 && mutations==3);
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='clan-apply-') as directory:
    test = Path(directory) / 'test.c'
    binary = Path(directory) / 'test'
    test.write_text(preamble + handler + checks)
    subprocess.run(['cc', '-std=gnu99', '-fsanitize=address,undefined', '-fno-pie', '-no-pie', str(test), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('Clan apply: usage, free SQL applications, blocked payments, and legacy fees passed.')
