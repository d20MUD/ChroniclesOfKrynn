#!/usr/bin/env python3
"""Exercise production prepared/spontaneous-cycle timing and queue codec without a live MUD."""
import pathlib, subprocess, tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
source=(ROOT/'src/spell_prep.c').read_text()
def function(name):
    start=source.index(name)
    start=source.rfind('\n',0,start)+1
    end=source.index('\n}',start)+2
    return source[start:end]
header=r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define TRUE true
#define FALSE false
#define MAX(a,b) ((a)>(b)?(a):(b))
#define MAX_INPUT_LENGTH 1024
#define MAX_PREP_QUEUE_SIZE 100
#define MAX_MEM 100
#define MAX_INNATE_QUEUE_SIZE 100
#define CLASS_SORCERER 1
#define CLASS_BARD 2
#define CLASS_INQUISITOR 3
#define CLASS_SUMMONER 4
struct prep_collection_spell_data {int spell,metamagic,prep_time,domain;bool clan_preparation_started,arcane_recovery_reduced;struct prep_collection_spell_data *next;};
struct innate_magic_data {int circle,domain,prep_time,metamagic;bool clan_preparation_started,arcane_recovery_reduced;struct innate_magic_data *next;};
struct char_data {int clan_arcane_percent;int64_t clan_arcane_expires_at;struct prep_collection_spell_data *head;struct innate_magic_data *innate;};
#define SPELL_PREP_QUEUE(ch,c) ((ch)->head)
#define INNATE_MAGIC(ch,c) ((ch)->innate)
#define IS_NPC(ch) false
#define GET_NAME(ch) "test"
#define log(...) ((void)0)
static int64_t now=100;
#define time(p) now
static int compute_spells_circle(struct char_data *ch,int c,int spell,int meta,int domain) {(void)ch;(void)c;(void)meta;(void)domain;return spell;}
static int compute_spells_prep_time(struct char_data *ch,int c,int circle,int domain) {(void)ch;(void)c;(void)circle;return domain?21:(circle==9?10:101);}
void reset_preparation_time(struct char_data *,int);
static void get_line(FILE *f,char *line) {assert(fgets(line,1024,f));}
static void prep_queue_add(struct char_data *ch,int c,int spell,int meta,int remaining,int domain) {(void)c;struct prep_collection_spell_data *p=calloc(1,sizeof(*p));assert(p);p->spell=spell;p->metamagic=meta;p->prep_time=remaining;p->domain=domain;p->next=ch->head;ch->head=p;}
static void innate_magic_add(struct char_data *ch,int c,int circle,int meta,int remaining,int domain) {(void)c;struct innate_magic_data *p=calloc(1,sizeof(*p));assert(p);p->circle=circle;p->metamagic=meta;p->prep_time=remaining;p->domain=domain;p->next=ch->innate;ch->innate=p;}
static void clear_innate(struct char_data *ch) {while(ch->innate){struct innate_magic_data *p=ch->innate;ch->innate=p->next;free(p);}}
static void clear(struct char_data *ch) {while(ch->head){struct prep_collection_spell_data *p=ch->head;ch->head=p->next;free(p);}}
'''
main=r'''
static void spontaneous_tests(void) {
 struct char_data ch={0},loaded={0};FILE *f;
 for(int c=CLASS_SORCERER;c<=CLASS_SUMMONER;c++) {
  for(int percent=10;percent<=20;percent+=5) {
   now=100;ch.clan_arcane_percent=percent;ch.clan_arcane_expires_at=200;
   innate_magic_add(&ch,c,1,0,101,0);innate_magic_add(&ch,c,1,0,101,0);
   start_clan_recovery_cycle(&ch,c);assert(ch.innate->prep_time==(101*(100-percent)+99)/100);
   assert(!ch.innate->next->clan_preparation_started);
   int saved=--ch.innate->prep_time;
   now=201;ch.clan_arcane_percent=0;reset_preparation_time(&ch,c);start_clan_recovery_cycle(&ch,c);assert(ch.innate->prep_time==saved);
   f=tmpfile();assert(f);save_innate_magic_by_class(f,&ch,c);fprintf(f,"-1 -1 -1 -1 -1\n");rewind(f);load_innate_magic_queue(f,&loaded);fclose(f);
   assert(loaded.innate->clan_preparation_started&&loaded.innate->prep_time==saved);
   assert(!loaded.innate->next->clan_preparation_started);
   loaded.clan_arcane_percent=20;loaded.clan_arcane_expires_at=999;start_clan_recovery_cycle(&loaded,c);assert(loaded.innate->prep_time==saved);
   clear_innate(&loaded);
   struct innate_magic_data *done=ch.innate;ch.innate=done->next;free(done);
   start_clan_recovery_cycle(&ch,c);assert(ch.innate->prep_time==101);clear_innate(&ch);
  }
  now=100;ch.clan_arcane_percent=0;innate_magic_add(&ch,c,1,0,101,0);start_clan_recovery_cycle(&ch,c);
  ch.clan_arcane_percent=20;ch.clan_arcane_expires_at=200;start_clan_recovery_cycle(&ch,c);assert(ch.innate->prep_time==101);clear_innate(&ch);
  innate_magic_add(&ch,c,9,0,10,0);start_clan_recovery_cycle(&ch,c);assert(ch.innate->prep_time==9);clear_innate(&ch);
  innate_magic_add(&ch,c,1,0,11,1);ch.innate->arcane_recovery_reduced=true;start_clan_recovery_cycle(&ch,c);assert(ch.innate->prep_time==9);
  ch.innate->prep_time=3;reset_preparation_time(&ch,c);start_clan_recovery_cycle(&ch,c);assert(ch.innate->prep_time==3);clear_innate(&ch);
  innate_magic_add(&ch,c,1,0,101,1);start_clan_recovery_cycle(&ch,c);ch.innate->prep_time=2;
  ch.innate->clan_preparation_started=false;reset_preparation_time(&ch,c);assert(ch.innate->prep_time==21);start_clan_recovery_cycle(&ch,c);assert(ch.innate->prep_time==17);clear_innate(&ch);
 }
 f=tmpfile();assert(f);fputs("1 1 0 40 0\n1 2 0 25 0 1\n-1 -1 -1 -1 -1\n",f);rewind(f);load_innate_magic_queue(f,&ch);fclose(f);
 assert(!ch.innate->arcane_recovery_reduced&&!ch.innate->clan_preparation_started);assert(ch.innate->next->arcane_recovery_reduced);clear_innate(&ch);
 now=100;puts("spontaneous-cycle tiers/classes, queue snapshots, restart and codec passed");
}
int main(void) {
 spontaneous_tests();
 struct char_data ch={0},loaded={0};FILE *f;
 for(int percent=10;percent<=20;percent+=5) {
  prep_queue_add(&ch,0,1,0,101,0);ch.clan_arcane_percent=percent;ch.clan_arcane_expires_at=200;
  start_clan_preparation_cycle(&ch,0);assert(ch.head->prep_time==(101*(100-percent)+99)/100);
  int saved=--ch.head->prep_time;ch.clan_arcane_percent=0;now=201;
  start_clan_preparation_cycle(&ch,0);reset_preparation_time(&ch,0);assert(ch.head->prep_time==saved);
  f=tmpfile();assert(f);save_prep_queue_by_class(f,&ch,0);fprintf(f,"-1 -1 -1 -1 -1\n");rewind(f);
  load_spell_prep_queue(f,&loaded);fclose(f);assert(loaded.head->clan_preparation_started);
  loaded.clan_arcane_percent=20;loaded.clan_arcane_expires_at=999;start_clan_preparation_cycle(&loaded,0);assert(loaded.head->prep_time==saved);
  clear(&loaded);clear(&ch);now=100;
 }
 // No-buff cycles also snapshot; activation cannot shorten one already running.
 prep_queue_add(&ch,0,9,0,10,0);ch.clan_arcane_percent=0;start_clan_preparation_cycle(&ch,0);
 ch.clan_arcane_percent=20;ch.clan_arcane_expires_at=200;start_clan_preparation_cycle(&ch,0);assert(ch.head->prep_time==10);clear(&ch);
 // A tail takes current eligibility only when it starts, not when queued.
 prep_queue_add(&ch,0,1,0,101,0);prep_queue_add(&ch,0,1,0,101,0);
 start_clan_preparation_cycle(&ch,0);assert(!ch.head->next->clan_preparation_started);
 struct prep_collection_spell_data *done=ch.head;ch.head=done->next;free(done);
 now=200;start_clan_preparation_cycle(&ch,0);assert(ch.head->prep_time==101);clear(&ch);now=100;
 // Circle minimum, domain timing, and preexisting Arcane Recovery reductions.
 prep_queue_add(&ch,0,9,0,10,0);start_clan_preparation_cycle(&ch,0);assert(ch.head->prep_time==9);clear(&ch);
 prep_queue_add(&ch,0,1,0,11,1);ch.head->arcane_recovery_reduced=true;reset_preparation_time(&ch,0);start_clan_preparation_cycle(&ch,0);assert(ch.head->prep_time==9);clear(&ch);
 // Ordinary interrupted/restarted cycle computes fresh, never from old remainder.
 prep_queue_add(&ch,0,1,0,30,1);start_clan_preparation_cycle(&ch,0);ch.head->prep_time=3;
 ch.head->clan_preparation_started=false;reset_preparation_time(&ch,0);assert(ch.head->prep_time==21);start_clan_preparation_cycle(&ch,0);assert(ch.head->prep_time==17);clear(&ch);
 // Older five/six-field saves remain loadable; markers belong to each entry.
 f=tmpfile();assert(f);fputs("0 1 0 50 0\n0 2 0 20 1 1\n-1 -1 -1 -1 -1\n",f);rewind(f);load_spell_prep_queue(f,&ch);fclose(f);
 assert(!ch.head->arcane_recovery_reduced&&!ch.head->clan_preparation_started);assert(ch.head->next->arcane_recovery_reduced);clear(&ch);
 puts("prepared-cycle timing and backward-compatible queue codec passed");return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='clan-preparation-') as tmp:
    path=pathlib.Path(tmp);driver=path/'test.c'
    driver.write_text(header+'\n'+'\n'.join(function(n) for n in ['void save_innate_magic_by_class(', 'void load_innate_magic_queue(', 'static void start_clan_recovery_cycle(', 'void save_prep_queue_by_class(', 'void load_spell_prep_queue(', 'static void start_clan_preparation_cycle(', 'void reset_preparation_time('])+main)
    subprocess.run(['cc','-std=gnu99','-Wall','-Wextra','-Werror','-Wno-unused-parameter','-fsanitize=address,undefined','-fno-pie','-no-pie',str(driver),'-o',str(path/'test')],check=True)
    subprocess.run([str(path/'test')],check=True)
