#!/usr/bin/env python3
"""Check production class choices, caster/HD caps, study spending, gear and player tags."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
src = root / 'src'

def function(filename, signature):
    text = (src / filename).read_text()
    a = text.index(signature)
    return text[a:text.index('\n}', a) + 2] + '\n'

code = '\n'.join('#include "'+h+'"' for h in
                 ['conf.h','sysdep.h','structs.h','utils.h','feats.h','spells.h','class.h',
                  'comm.h','handler.h','oasis.h','db.h','dg_scripts.h'])
code += '\n#define FEAT_TYPE_NORMAL 1\n#define FEAT_TYPE_NORMAL_CLASS 2\n#define FEAT_TYPE_EPIC 3\n#define FEAT_TYPE_EPIC_CLASS 4\n#define FEAT_TYPE_NORMAL_TEAMWORK 5\n'
code += r'''
#include <assert.h>
struct class_table class_list[NUM_CLASSES];
struct feat_info feat_list[NUM_FEATS];
struct player_special_data dummy_mob;
static struct char_data *live;
static int menus;
int MIN(int a,int b) { return a<b?a:b; }
int MAX(int a,int b) { return a>b?a:b; }
void basic_mud_log(const char *format,...) {}
size_t write_to_output(struct descriptor_data *d,const char *format,...) { return 0; }
int get_dragon_disciple_arcane_class(struct char_data *ch) { return CLASS_UNDEFINED; }
int loremaster_levels_advancing_class(struct char_data *ch,int class) { return 0; }
int loremaster_levels_advancing_arcane(struct char_data *ch) { return 0; }
int loremaster_levels_advancing_divine(struct char_data *ch) { return 0; }
int compute_arcana_golem_level(struct char_data *ch) { return 0; }
int is_class_feat(int feat,int class,struct char_data *ch) { return false; }
bool isSorcBloodlineFeat(int feat) { return false; }
int has_feat_requirement_check(struct char_data *ch,int feat) {
  return HAS_REAL_FEAT(ch,feat) + (LEVELUP(ch) ? LEVELUP(ch)->feats[feat] : 0);
}
static void gen_feat_disp_menu(struct descriptor_data *d) { menus++; }
int parse_class_long(const char *arg) {
  if(!strcmp(arg,"wizard")) return CLASS_WIZARD;
  if(!strcmp(arg,"cleric")) return CLASS_CLERIC;
  return CLASS_UNDEFINED;
}
'''
for signature in ['bool is_caster_class(', 'int compute_bonus_caster_level(',
                  'int compute_arcane_level(', 'int compute_divine_level(',
                  'static bool has_item_typed_feat(', 'static int practiced_spellcaster_item_feat(',
                  'bool has_practiced_spellcaster_class(', 'int practiced_spellcaster_level(',
                  'int compute_caster_level(', 'bool practiced_spellcaster_choice_available(']:
    code += function('utils.c', signature)
code += function('utils.c', 'bool is_spellnum_psionic(')
code += 'bool isEpicSpell(int spellnum) { return IS_EPIC_SPELL(spellnum); }\n'
code += function('study.c', 'bool add_levelup_feat(')
code += function('study.c', 'static void practiced_spellcaster_menu(')
code += function('spell_prep.c', 'int get_class_highest_circle(')
study = (src / 'study.c').read_text()
a = study.index('  case STUDY_PRACTICED_CLASS:')
b = study.index('  /* Combat feats require', a)
code += 'void choose(struct descriptor_data *d,const char *arg) { struct char_data *ch=d->character; int number; switch(STUDY_PRACTICED_CLASS) {\n'
code += study[a:b] + '\n}}\n'

players = (src / 'players.c').read_text()
a = players.index('        else if (!strcmp(tag, "PScC"))')
b = players.index('        else if (!strcmp(tag, "PStg"))', a)
code += 'void load_choice(struct char_data *ch,const char *tag,const char *line) { int i; if(0) {}\n'
code += players[a:b] + '\n}\n'
a = players.index('  if (HAS_REAL_FEAT(ch, FEAT_PRACTICED_SPELLCASTER))')
b = players.index('  BUFFER_WRITE("Perk:', a)
code += '''char saved[256];
#define BUFFER_WRITE(...) snprintf(saved+strlen(saved),sizeof(saved)-strlen(saved),__VA_ARGS__)
void save_choices(struct char_data *ch) { int i; saved[0]='\\0';
'''
code += players[a:b] + '\n}\n#undef BUFFER_WRITE\n'

# Exercise the real context wrapper with a fake resolver that observes the trait bonus.
parser = (src / 'spell_parser.c').read_text()
a = parser.index('  /* Epic spells skip preparation, so choose their casting class explicitly. */')
b = parser.index('  if (!IS_NPC(ch))', a + len('  /* Epic spells skip preparation, so choose their casting class explicitly. */'))
code += 'int epic_class(struct char_data *ch,int spellnum) { int class_num=CLASS_UNDEFINED,i;\n'
code += parser[a:b] + '\nreturn class_num;\n}\n'
code += '''struct char_data *find_char(long id) { return live && GET_ID(live)==id ? live : NULL; }
static int resolve_call_magic(struct char_data *ch,struct char_data *victim,struct obj_data *obj,
 int spell,int meta,int level,int type) { return practiced_spellcaster_level(ch,CLASS_WIZARD,10); }
'''
code += function('spell_parser.c', 'int call_magic(')
code += r'''
int main(void) {
  struct char_data ch={0}, loaded={0}, npc={0};
  struct player_special_data special={0}, loaded_special={0};
  struct level_data level={0};
  struct descriptor_data d={0};
  struct oasis_olc_data olc={0};
  struct obj_data gear={0};
  int before;
  ch.player_specials=&special; loaded.player_specials=&loaded_special;
  special.levelup=&level; level.class=CLASS_WIZARD;
  d.character=&ch; d.olc=&olc;
  GET_ID(&ch)=100; live=&ch;
  GET_LEVEL(&ch)=15; CLASS_LEVEL((&ch),CLASS_WIZARD)=10;
  class_list[CLASS_WIZARD].name="wizard"; class_list[CLASS_CLERIC].name="cleric";
  feat_list[FEAT_PRACTICED_SPELLCASTER].can_stack=true;
  assert(practiced_spellcaster_choice_available(&ch,CLASS_WIZARD));
  assert(!practiced_spellcaster_choice_available(&ch,CLASS_WARRIOR));
  assert(!practiced_spellcaster_choice_available(&ch,-1));
  assert(!practiced_spellcaster_choice_available(&ch,NUM_CLASSES));
  level.feat_points=2; level.tempFeat=FEAT_PRACTICED_SPELLCASTER;
  choose(&d,"wizard");
  assert(level.feat_points==1 && level.feats[FEAT_PRACTICED_SPELLCASTER]==1);
  assert(level.practiced_spellcaster_classes[CLASS_WIZARD]);
  assert(!practiced_spellcaster_choice_available(&ch,CLASS_WIZARD));
  choose(&d,"wizard"); assert(level.feat_points==1); /* No duplicate spending. */
  choose(&d,"q"); assert(level.feat_points==1); /* Cancel is free. */
  CLASS_LEVEL((&ch),CLASS_CLERIC)=1;
  choose(&d,"cleric"); assert(level.feat_points==0 && level.feats[FEAT_PRACTICED_SPELLCASTER]==2);
  assert(level.practiced_spellcaster_classes[CLASS_CLERIC]);
  assert(practiced_spellcaster_level(&ch,CLASS_WIZARD,10)==10); /* Pending choice isn't active. */
  HAS_REAL_FEAT(&ch,FEAT_PRACTICED_SPELLCASTER)=1;
  ch.char_specials.saved.practiced_spellcaster_classes[CLASS_WIZARD]=true;
  assert(get_class_highest_circle(&ch,CLASS_WIZARD)==5);
  assert(practiced_spellcaster_level(&ch,CLASS_WIZARD,10)==14);
  assert(get_class_highest_circle(&ch,CLASS_WIZARD)==5); /* No new spell circles. */
  assert(practiced_spellcaster_level(&ch,CLASS_WIZARD,0)==0);
  assert(practiced_spellcaster_level(&ch,CLASS_CLERIC,1)==1); /* Different class unchanged. */
  GET_LEVEL(&ch)=12; assert(practiced_spellcaster_level(&ch,CLASS_WIZARD,10)==12);
  GET_LEVEL(&ch)=10; assert(practiced_spellcaster_level(&ch,CLASS_WIZARD,10)==10);
  assert(practiced_spellcaster_level(&ch,CLASS_WIZARD,14)==14); /* Preserve other above-HD bonuses. */
  GET_LEVEL(&ch)=15; assert(practiced_spellcaster_level(&ch,CLASS_WIZARD,10)==14);
  CLASS_LEVEL((&ch),CLASS_ELDRITCH_KNIGHT)=5;
  assert(BONUS_CASTER_LEVEL(&ch,CLASS_WIZARD)==5); /* Feat never enters spell progression. */
  assert(practiced_spellcaster_level(&ch,CLASS_WIZARD,15)==15); /* Fully progressed at HD. */
  GET_LEVEL(&ch)=20; assert(practiced_spellcaster_level(&ch,CLASS_WIZARD,15)==19);
  CLASS_LEVEL((&ch),CLASS_ELDRITCH_KNIGHT)=0;
  CLASS_LEVEL((&ch),CLASS_CLERIC)=0;
  CASTING_CLASS(&ch)=CLASS_WIZARD; assert(CASTER_LEVEL(&ch)==14);
  assert(epic_class(&ch,SPELL_HELLBALL)==CLASS_WIZARD);
  assert(epic_class(&ch,PSIONIC_IMPALE_MIND)==CLASS_PSIONICIST);
  CASTING_CLASS(&ch)=CLASS_CLERIC; assert(CASTER_LEVEL(&ch)==10);
  before=ch.char_specials.saved.practiced_spellcaster_classes[CLASS_CLERIC];
  load_choice(&loaded,"PScC","-1"); load_choice(&loaded,"PScC","99999");
  load_choice(&loaded,"PScC","garbage");
  assert(!loaded.char_specials.saved.practiced_spellcaster_classes[CLASS_WIZARD]);
  save_choices(&ch); assert(strstr(saved,"PScC: 0\n"));
  load_choice(&loaded,"PScC","0");
  assert(loaded.char_specials.saved.practiced_spellcaster_classes[CLASS_WIZARD]);
  HAS_REAL_FEAT(&loaded,FEAT_PRACTICED_SPELLCASTER)=1;
  GET_LEVEL(&loaded)=20; CLASS_LEVEL((&loaded),CLASS_WIZARD)=10;
  assert(practiced_spellcaster_level(&loaded,CLASS_WIZARD,10)==14);
  assert(ch.char_specials.saved.practiced_spellcaster_classes[CLASS_CLERIC]==before);
  assert(call_magic(&ch,NULL,NULL,SPELL_FIREBALL,0,10,CAST_SPELL)==14);
  assert(call_magic(&ch,NULL,NULL,SPELL_FIREBALL,0,10,CAST_WAND)==10);
  assert(!ch.practiced_spellcaster_suppressed);
  assert(call_magic(&ch,NULL,NULL,SPELL_FIREBALL,0,10,CAST_SPELL)==14);
  HAS_REAL_FEAT(&ch,FEAT_PRACTICED_SPELLCASTER)=0;
  assert(practiced_spellcaster_level(&ch,CLASS_WIZARD,10)==10);
  gear.affected[0].location=APPLY_FEAT;
  gear.affected[0].modifier=FEAT_PRACTICED_SPELLCASTER;
  gear.affected[0].specific=CLASS_WIZARD;
  GET_EQ(&ch,WEAR_BODY)=&gear;
  assert(practiced_spellcaster_level(&ch,CLASS_WIZARD,10)==14);
  assert(practiced_spellcaster_level(&ch,CLASS_CLERIC,10)==10);
  gear.affected[1]=gear.affected[0];
  assert(practiced_spellcaster_level(&ch,CLASS_WIZARD,10)==14); /* Gear never stacks the bonus. */
  GET_EQ(&ch,WEAR_BODY)=NULL;
  assert(practiced_spellcaster_level(&ch,CLASS_WIZARD,10)==10);
  SET_BIT_AR(MOB_FLAGS(&npc),MOB_ISNPC); GET_LEVEL(&npc)=20;
  assert(practiced_spellcaster_level(&npc,CLASS_WIZARD,20)==20);
  assert(CASTER_LEVEL(&npc)==20);
  puts("Practiced Spellcaster: study, class isolation, HD/progression caps, gear, save/load and item context passed.");
  return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='practiced-spellcaster-') as temp:
    source = Path(temp) / 'check.c'
    exe = Path(temp) / 'check'
    source.write_text(code)
    subprocess.run(['cc','-I',str(src),'-fsanitize=address,undefined','-g','-o',str(exe),str(source)],check=True)
    subprocess.run([str(exe)],check=True)
