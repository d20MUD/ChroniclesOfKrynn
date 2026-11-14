#!/usr/bin/env python3
"""Exercise personal hunt selection, ownership and durable state in isolation."""
from pathlib import Path
import subprocess
import shlex
import runpy
from clan_database import database
root = Path(__file__).resolve().parents[1]
source = (root / 'src/hunts.c').read_text()
def function(signature):
    start = source.index(signature)
    return source[start:source.index('\n}', start) + 2] + '\n'
preamble = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
#define CAMPAIGN_DL
#define SPECIAL_DECL(n) int n(void)
typedef int room_rnum;
typedef int zone_vnum;
typedef int zone_rnum;
typedef int region_vnum;
struct char_data;
#include "hunts.h"
#include "mysql.h"
static MYSQL_POOL_CONN test_connection;
bool mysql_available=true;
static int acquisitions;
MYSQL_POOL_CONN *mysql_pool_acquire(void) { acquisitions++; return &test_connection; }
void mysql_pool_release(MYSQL_POOL_CONN *c) {}
#define CREATE(p,t,n) ((p)=calloc(n,sizeof(t)))
#define IS_NPC(c) ((c)->npc)
#define MOB_HUNTS_TARGET 1
#define MOB_FLAGGED(c,f) ((c)->hunt)
#define GET_IDNUM(c) ((c)->id)
#define GET_LEVEL(c) ((c)->level)
#define IN_ROOM(c) ((c)->room)
#define NOWHERE -1
#define log test_log
struct char_data { bool npc, hunt; long id; int level, room; struct char_data *next;
 struct { int hunt_cooldown, hunt_slot, hunt_type; long hunt_owner_id; time_t hunt_expires_at; } mob_specials;
};
struct { int number, coords[2]; } world[200];
static struct char_data *character_list;
struct hunt_type hunt_table[NUM_HUNT_TYPES];
static struct personal_hunts *hunt_records;
static time_t clock_now=2000000000;
static time_t test_time(time_t *t) { if(t)*t=clock_now;return clock_now; }
#define time test_time
static void test_log(const char *format, ...) {}
static int dice(int n, int count) { assert(count>0); return 1; }
static int real_room(int vnum) { return vnum>=100 && vnum<200 ? vnum-100 : NOWHERE; }
static int next_room;
static int get_random_road_room(int type) { return next_room++ % 100; }
static bool hunt_room_reaches_city(int room) { return true; }
static int spawned;
static long spawned_owner;
void create_hunt_mob(struct char_data *owner, struct personal_hunts *record, int slot) {
 assert(record->owner==owner->id);
 assert(hunt_table[record->targets[slot][0]].level <= owner->level);
 spawned++;spawned_owner=owner->id;
}
'''
code = preamble
for signature in ['static bool save_personal_hunts(', 'static bool parse_personal_hunt_targets(', 'static struct personal_hunts *load_personal_hunts(', 'static void retire_personal_hunts(', 'static struct personal_hunts *ensure_personal_hunts(', 'void create_hunts(void)', 'static struct char_data *find_personal_hunt_mob(', 'void check_hunt_room(', 'void select_hunt_coords(', 'int select_a_hunt(', 'bool remove_hunts_mob(']:
    code += function(signature)
code += r'''
static void clear_cache(void) { while(hunt_records) { struct personal_hunts *r=hunt_records; hunt_records=r->next; free(r); } }
static int count(struct personal_hunts *r, int level) {
 int i,n=0; for(i=0;i<AHUNT_1;i++) if(r->targets[i][0]) { assert(hunt_table[r->targets[i][0]].level<=level);n++; } return n;
}
int main(int argc, char **argv) {
 struct char_data a={0},b={0},mob={0},old={0};
 struct personal_hunts *ra,*rb;
 int i,slot,type,old_room; time_t expiry;
 assert(argc==2);
 test_connection.conn=mysql_init(NULL);assert(test_connection.conn);
 assert(mysql_real_connect(test_connection.conn,NULL,"root","","clan_test",0,argv[1],0));
 for(i=0;i<100;i++)world[i].number=100+i;
 for(i=1;i<NUM_HUNT_TYPES;i++)hunt_table[i].level=10+((i-1)%5)*5;
 a.id=101;b.id=202;a.level=30;b.level=15;
 assert(!ensure_personal_hunts(&a,false));
 ra=ensure_personal_hunts(&a,true);rb=ensure_personal_hunts(&b,true);
 assert(ra && rb && ra!=rb && count(ra,30)==5 && count(rb,15)==2);
 assert(ra->targets[0][1]!=rb->targets[0][1]);
 for(i=1;i<=34;i++) { struct char_data c={0}; c.id=1000+i;c.level=i;
   struct personal_hunts *r=ensure_personal_hunts(&c,true);assert(r);
   assert(count(r,i)==(i<10?0:(i>=30?5:1+(i-10)/5)));
 }
 assert(select_a_hunt(9)==HUNT_TYPE_NONE);
 { int before=acquisitions; for(i=0;i<100;i++)assert(ensure_personal_hunts(&a,true)==ra);assert(acquisitions==before); }
 { int parsed[AHUNT_1][AHUNT_2];assert(!parse_personal_hunt_targets("1 2",parsed));assert(!parse_personal_hunt_targets("99999999999999999999999999",parsed)); }
 // Encounter creation uses only the moving character's personal coordinates.
 a.room=real_room(ra->targets[0][1]);check_hunt_room(&a);assert(spawned==1 && spawned_owner==101);
 b.room=a.room;check_hunt_room(&b);assert(spawned==1);
 b.room=real_room(rb->targets[0][1]);check_hunt_room(&b);assert(spawned==2 && spawned_owner==202);
 // Persist completion before rewards, and never clear another owner's same species.
 slot=0;type=ra->targets[slot][0];expiry=ra->expires_at;
 mob.npc=mob.hunt=true;mob.mob_specials.hunt_owner_id=101;
 mob.mob_specials.hunt_expires_at=expiry;mob.mob_specials.hunt_slot=slot;mob.mob_specials.hunt_type=type;
 character_list=&mob;
 assert(find_personal_hunt_mob(ra,slot)==&mob);
 assert(!find_personal_hunt_mob(rb,slot));
 assert(!find_personal_hunt_mob(ra,slot+1));
 character_list=NULL;
 assert(!remove_hunts_mob(NULL));
 assert(remove_hunts_mob(&mob));assert(!ra->targets[slot][0] && rb->targets[slot][0]);
 assert(!remove_hunts_mob(&mob));
 clear_cache();ra=ensure_personal_hunts(&a,true);rb=ensure_personal_hunts(&b,true);
 assert(!ra->targets[slot][0] && count(rb,15)==2 && ra->expires_at==expiry);
 // The owner need not be loaded or online when the target is defeated.
 mob.mob_specials.hunt_owner_id=202;mob.mob_specials.hunt_expires_at=rb->expires_at;mob.mob_specials.hunt_type=rb->targets[0][0];
 clear_cache();assert(remove_hunts_mob(&mob));
 rb=ensure_personal_hunts(&b,true);assert(count(rb,15)==1);
 // A failed durable write leaves the assignment untouched and grants no completion.
 assert(!mysql_query(test_connection.conn,"RENAME TABLE personal_hunt_progress TO hidden_hunt_progress"));
 mob.mob_specials.hunt_slot=1;mob.mob_specials.hunt_type=rb->targets[1][0];
 assert(!remove_hunts_mob(&mob));assert(rb->targets[1][0]);
 assert(!mysql_query(test_connection.conn,"RENAME TABLE hidden_hunt_progress TO personal_hunt_progress"));
 rb=ensure_personal_hunts(&b,true);assert(rb && !rb->needs_reload);
 // A concurrent writer cannot be overwritten by stale cached state.
 { struct personal_hunts stale=*rb;
   assert(save_personal_hunts(rb));assert(!save_personal_hunts(&stale));
   assert(rb->revision==stale.revision+1);
 }
 // Refuse to treat a pooled uncommitted write as durable progress.
 assert(!mysql_autocommit(test_connection.conn,0));
 { struct personal_hunts before=*rb;assert(!save_personal_hunts(rb));assert(rb->revision==before.revision); }
 assert(!mysql_rollback(test_connection.conn));assert(!mysql_autocommit(test_connection.conn,1));
 // Another process inserting a previously absent owner is not silently replaced.
 { struct personal_hunts duplicate=*rb;duplicate.revision=0;assert(!save_personal_hunts(&duplicate)); }
 // Level loss removes all targets above the character's current level.
 a.level=14;ra=ensure_personal_hunts(&a,true);assert(count(ra,14)==0);
 old=mob;old.mob_specials.hunt_cooldown=-1;character_list=&old;
 old_room=next_room;clock_now=expiry+1;
 rb=ensure_personal_hunts(&b,true);assert(rb->expires_at>expiry && count(rb,15)==2 && next_room>old_room);
 assert(old.mob_specials.hunt_cooldown==50);
 assert(!remove_hunts_mob(&mob)); // Previous set must not clear the new assignment.
 // Bad SQL records fail closed rather than silently granting a new set.
 assert(!mysql_query(test_connection.conn,"INSERT INTO personal_hunt_progress(character_id,expires_at,targets) VALUES(99999,2000005000,'broken')"));
 assert(!load_personal_hunts(99999));
 clear_cache();mysql_available=false;assert(!load_personal_hunts(101));
 mysql_close(test_connection.conn);return 0;
}
'''
with database(('20261114_personal_hunts.sql',)) as (work, socket, sql, client):
    sql((root/'sql/20261114_personal_hunts.sql').read_text())  # Reapplication preserves rows.
    (work / 'test.c').write_text(code)
    flags=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','mariadb'],text=True))
    subprocess.run(['cc','-std=gnu99','-g','-fsanitize=address,undefined','-fno-pie','-no-pie','-I'+str(root/'src'),str(work/'test.c'),*flags,'-o',str(work/'test')],check=True)
    subprocess.run([str(work/'test'),str(socket)],cwd=work,check=True)
    convert=runpy.run_path(str(root/'scripts/import_personal_hunt_files.py'))['convert']
    legacy=work/'old-hunts';legacy.mkdir()
    payload=' '.join(['0']*35)
    (legacy/'900001').write_text('HUNTS1 900001 2000005000\n'+payload+'\n')
    sql(convert(legacy))
    assert sql('SELECT revision FROM personal_hunt_progress WHERE character_id=900001')=='1'
    # Duplicate SQL progress must be preserved, with the entire batch rolled back.
    (legacy/'900000').write_text('HUNTS1 900000 2000005000\n'+payload+'\n')
    result=subprocess.run(client,input=convert(legacy),text=True,capture_output=True)
    assert result.returncode and sql('SELECT COUNT(*) FROM personal_hunt_progress WHERE character_id=900000')=='0'
    assert sql('SELECT revision FROM personal_hunt_progress WHERE character_id=900001')=='1'
    (legacy/'900000').write_text('bad record')
    try: convert(legacy)
    except ValueError: pass
    else: raise AssertionError('Malformed hunt records must be rejected before SQL output')

print('Personal hunts MySQL tests passed: level caps, ownership, offline/restart persistence, cached reads, conflicts, expiry, and failed SQL writes.')
