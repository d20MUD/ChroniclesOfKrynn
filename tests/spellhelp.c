/* Exercise the real command and renderer without starting a MUD. */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "spells.h"
#include "psionics.h"
#include "domains_schools.h"
#include "constants.h"
#include "modify.h"
#include "act.h"
#include "spellhelp.h"
#include <assert.h>
#include "perfmon.h"

void PERF_prof_sect_init(struct PERF_prof_sect **ptr, const char *id) {}
void PERF_prof_sect_enter(struct PERF_prof_sect *ptr) {}
void PERF_prof_sect_exit(struct PERF_prof_sect *ptr) {}

struct config_data config_info;
struct spell_info_type spell_info[TOP_SKILL_DEFINE + 1];
struct psionic_power_data psionic_powers[MAX_SPELLS + 1];
struct domain_info domain_list[NUM_DOMAINS];
const char *class_names[NUM_CLASSES + 1];
const char *position_types[NUM_POSITIONS + 1];
const char *spell_schools[NUM_SCHOOLS + 1];
static char output[65536];
void load_batch_registration(void);
void free_batch_registration(void);
void load_psionic_registration(void);
void free_psionic_registration(void);

bool is_spellnum_psionic(int id) { return id >= PSIONIC_POWER_START && id <= PSIONIC_POWER_END; }
int get_spell_circle(int id, int cls) { return (spell_info[id].min_level[cls]+1)/2; }
size_t send_to_char(struct char_data *ch, const char *format, ...)
{
  va_list ap;
  int count;
  va_start(ap, format);
  count = vsnprintf(output + strlen(output), sizeof(output)-strlen(output), format, ap);
  va_end(ap);
  return count;
}
void page_string(struct descriptor_data *d, char *text, int keep)
{
  assert(keep);
  snprintf(output, sizeof(output), "%s", text);
}

static void registered(int id, const char *name)
{
  int i;
  spell_info[id].name = name;
  spell_info[id].actual_ability = true;
  spell_info[id].min_position = POS_FIGHTING;
  for(i=0; i<NUM_CLASSES; i++) spell_info[id].min_level[i] = LVL_IMMORT;
}
/* Check the actual colored columns before simplifying whitespace for semantic assertions. */
static void check_alignment(const char *text)
{
  const char *line, *end, *cursor, *colon;
  int visible;
  if (text) {
    assert(!strstr(text,"Preparation") && !strstr(text,"Summon scaling"));
    assert(!strstr(text,"Damage calculation"));
    assert(!strstr(text,"Levels 1-10:") && !strstr(text,"Levels 11-20:") && !strstr(text,"Levels 21+:"));
  }
  if (!text || (strncmp(text, "\tcSpell", 7) && strncmp(text, "\tcPower", 7))) return;
  for (line=text; *line; line=end+2)
  {
    end=strstr(line,"\r\n"); assert(end);
    visible=0;
    for(cursor=line; cursor<end; cursor++) {
      if(*cursor=='\t' && cursor+1<end) cursor++;
      else visible++;
    }
    assert(visible<=80);
    if(!strncmp(line,"\tc",2)) {
      colon=strchr(line,':'); assert(colon && colon<end);
      assert(colon-(line+2)==20);
      assert(!strncmp(colon,":\tn ",4));
    } else {
      for(visible=0; visible<22; visible++) assert(line[visible]==' ');
    }
  }
}
static void simplify_layout(char *text)
{
  char *read=text, *write=text, *trim;
  while(*read) {
    if(!strncmp(read,"\r\n                      ",24)) {
      *write++=' '; read+=24;
    } else if(!strncmp(read,":\tn ",4)) {
      trim=write;
      while(trim>text && trim[-1]==' ') trim--;
      write=trim;
      memcpy(write,read,4); write+=4; read+=4;
    } else *write++=*read++;
  }
  *write=0;
}
static char *render_checked(int id)
{
  char *text=spellhelp_render(id);
  if(text) {check_alignment(text); simplify_layout(text);}
  return text;
}
static void command(struct char_data *ch, const char *arg)
{
  output[0]=0;
  do_spellhelp(ch, arg, 0, 0);
  check_alignment(output); simplify_layout(output);
}
int main(void)
{
  int i;
  char *text, *long_message;
  struct char_data ch={0}, before;
  struct player_special_data ps={0}, ps_before;
  struct descriptor_data descriptor={0};
  struct spell_info_type info_before;
  struct psionic_power_data power_before;
  int ids[] = {PSIONIC_DEADLY_FEAR, SPELL_MAGIC_MISSILE, SPELL_CURE_LIGHT,
    PSIONIC_MENTAL_BARRIER, SPELL_WORD_OF_RECALL, SPELL_SUMMON_CREATURE_1, SPELL_ACID_FOG,
    SPELL_FIREBALL, SPELL_BLINDNESS, SPELL_GLITTERDUST, SPELL_BLESS, SPELL_SHIELD,
    SPELL_CURE_MODERATE, SPELL_CURE_SERIOUS, SPELL_CURE_CRITIC, SPELL_HEAL,
    SPELL_CURE_BLIND, SPELL_REMOVE_POISON, SPELL_REMOVE_CURSE, SPELL_REMOVE_PARALYSIS,
    SPELL_CREATE_FOOD, SPELL_CREATE_WATER, SPELL_GROUP_HEAL, SPELL_MASS_CURE_LIGHT,
    SPELL_MASS_CURE_MODERATE, SPELL_MASS_CURE_SERIOUS, SPELL_MASS_CURE_CRIT,
    SPELL_FOG_CLOUD, SPELL_SOLID_FOG, PSIONIC_PSIONIC_BLAST};
  for(i=0; i<NUM_CLASSES; i++) class_names[i]="test class";
  for(i=0; i<NUM_POSITIONS; i++) position_types[i]="test position";
  for(i=0; i<NUM_SCHOOLS; i++) spell_schools[i]="test school";
  registered(PSIONIC_DEADLY_FEAR,"deadly fear");
  registered(SPELL_MAGIC_MISSILE,"magic missile");
  registered(SPELL_MAGIC_STONE,"magic stone");
  registered(SPELL_FIREBALL,"fireball");
  registered(SPELL_SUMMON,"secret effect"); spell_info[SPELL_SUMMON].no_player=true;
  registered(SPELL_POISON,"disabled spell"); spell_info[SPELL_POISON].cant_cast=true;
  registered(SPELL_BLESS,"!internal");
  registered(SPELL_CURE_LIGHT,"cure light");
  registered(PSIONIC_MENTAL_BARRIER,"mental barrier");
  registered(SPELL_WORD_OF_RECALL,"word of recall");
  registered(SPELL_SUMMON_CREATURE_1,"summon creature i");
  registered(SPELL_ACID_FOG,"acid fog");
  spell_info[PSIONIC_DEADLY_FEAR].min_level[CLASS_PSIONICIST]=7;
  spell_info[PSIONIC_DEADLY_FEAR].routines=MAG_DAMAGE;
  spell_info[PSIONIC_DEADLY_FEAR].targets=TAR_CHAR_ROOM|TAR_NOT_SELF;
  spell_info[PSIONIC_DEADLY_FEAR].violent=true;
  spell_info[PSIONIC_DEADLY_FEAR].time=4;
  psionic_powers[PSIONIC_DEADLY_FEAR].psp_cost=7;
  psionic_powers[PSIONIC_DEADLY_FEAR].can_augment=true;
  psionic_powers[PSIONIC_DEADLY_FEAR].augment_amount=2;
  psionic_powers[PSIONIC_DEADLY_FEAR].max_augment=100;
  psionic_powers[PSIONIC_DEADLY_FEAR].power_type=TELEPATHY;
  domain_list[0].name="test domain";
  domain_list[0].domain_spells[3]=SPELL_FIREBALL;
  ch.player_specials=&ps; ch.desc=&descriptor;
  GET_AUGMENT_PSP(&ch)=13; GET_DC_BONUS(&ch)=7; GET_PSP(&ch)=99;
  before=ch; ps_before=ps;
  info_before=spell_info[PSIONIC_DEADLY_FEAR];
  power_before=psionic_powers[PSIONIC_DEADLY_FEAR];
  command(&ch, "  DEADLY FEAR   ");
  assert(strstr(output,"\tcSchool:\tn Telepathy"));
  assert(strstr(output,"\tcClasses:\tn test class level 7 (circle 4)"));
  assert(strstr(output,"\tcAugmentation:\tn"));
  assert(!strstr(output,"Base cost") && !strstr(output,"Expiration message") && !strstr(output,"Coverage:"));
  assert(strstr(output,"Will negates, then Fortitude partial"));
  assert(strstr(output,"8 additional PSP"));
  assert(strstr(output,"Death-magic immunity"));
  assert(!memcmp(&ch,&before,sizeof(ch)) && !memcmp(&ps,&ps_before,sizeof(ps)));
  assert(!memcmp(&info_before,&spell_info[PSIONIC_DEADLY_FEAR],sizeof(info_before)));
  assert(!memcmp(&power_before,&psionic_powers[PSIONIC_DEADLY_FEAR],sizeof(power_before)));
  output[0] = '\0';
  assert(spellhelp_try_show(&ch, "  'DEADLY FEAR'  "));
  assert(strstr(output, "Will negates"));
  output[0] = '\0';
  assert(spellhelp_try_show(&ch, "magic missile"));
  assert(strstr(output, "1d4+1"));
  output[0] = '\0';
  assert(spellhelp_try_show(&ch, "deadly f"));
  output[0] = '\0';
  assert(!spellhelp_try_show(&ch, "magic"));
  assert(!spellhelp_try_show(&ch, "secret"));
  assert(!spellhelp_try_show(&ch, "disabled"));
  assert(!spellhelp_try_show(&ch, "!internal"));
  assert(!spellhelp_try_show(&ch, "coverage"));
  assert(!spellhelp_try_show(&ch, ""));
  assert(!spellhelp_try_show(&ch, "ordinary help topic"));
  assert(!*output);
  command(&ch,"magic"); assert(strstr(output,"more specific") && strstr(output,"magic stone") && strstr(output,"magic missile"));
  command(&ch,"magic missile"); assert(strstr(output,"1d4+1") && !strstr(output,"more specific"));
  command(&ch,"'deadly fear'"); assert(strstr(output,"Will negates"));
  command(&ch,"\"deadly fear\""); assert(strstr(output,"Will negates"));
  command(&ch,"deadly f"); assert(strstr(output,"Will negates"));
  command(&ch,"secret"); assert(strstr(output,"No public"));
  command(&ch,"disabled"); assert(strstr(output,"No public"));
  command(&ch,"!internal"); assert(strstr(output,"No public"));
  command(&ch,""); assert(strstr(output,"Usage:"));
  command(&ch,"fireball"); assert(strstr(output,"10d6") && strstr(output,"test domain (circle 4)"));
  command(&ch,"coverage"); assert(strstr(output,"9 core-mechanics entries"));
  for(i=0; i<(int)(sizeof(ids)/sizeof(ids[0])); i++) {
    if(!spellhelp_public(ids[i])) registered(ids[i],"fixture spell");
    text=render_checked(ids[i]); assert(text && strstr(text,"\tcDescription:\tn") && strstr(text,"\tcDamage:\tn")); free(text);
  }
  assert(!render_checked(-1) && !render_checked(TOP_SPELL_DEFINE+1));
  /* The renderer reads updated runtime registration, not copied spello arguments. */
  spell_info[SPELL_FIREBALL].touch_spell=true;
  spell_info[SPELL_FIREBALL].component=COMPONENT_VERBAL|COMPONENT_MATERIAL;
  spell_info[SPELL_FIREBALL].time=12;
  command(&ch,"fireball");
  assert(strstr(output,"\tcTouch:\tn Requires a successful touch attack"));
  assert(!strstr(output,"verbal, material") && !strstr(output,"casting units"));
  ch.desc=NULL; command(&ch,"fireball"); assert(strstr(output,"fireball"));
  registered(SPELL_CHILL_TOUCH,"not an ability"); spell_info[SPELL_CHILL_TOUCH].actual_ability=false;
  command(&ch,"not an ability"); assert(strstr(output,"No public"));
  registered(SPELL_HARM,"dead entry"); spell_info[SPELL_HARM].min_position=POS_DEAD;
  command(&ch,"dead entry"); assert(strstr(output,"No public"));
  assert(!spellhelp_public(NUM_SPELLS) && !spellhelp_public(0));
  long_message=malloc(40001); assert(long_message);
  memset(long_message,'x',40000); long_message[40000]=0;
  spell_info[SPELL_FIREBALL].wear_off_msg=long_message;
  text=render_checked(SPELL_FIREBALL); assert(text && strlen(text)<10000);
  assert(!strstr(text,"xxxxxxxx"));
  free(text);
  class_names[0]=long_message; spell_info[SPELL_FIREBALL].min_level[0]=1;
  text=render_checked(SPELL_FIREBALL); assert(text && strlen(text)>40000);
  class_names[0]="test class";
  free(text); free(long_message); spell_info[SPELL_FIREBALL].wear_off_msg=NULL;
  registered(SPELL_BLINDNESS,"blindness"); command(&ch,"blindness");
  assert(strstr(output,"-4 on attack rolls and armor class") && strstr(output,"Fortitude negates"));
  assert(strstr(output,"\tcDamage:\tn None."));
  registered(SPELL_BLESS,"bless"); command(&ch,"bless");
  assert(strstr(output,"Will saves") && strstr(output,"On an object") && strstr(output,"Removes Bane"));
  registered(SPELL_HEAL,"heal"); command(&ch,"heal");
  assert(strstr(output,"10 times caster level + 20") && strstr(output,"removes the Blindness"));
  registered(SPELL_GROUP_HEAL,"group heal"); command(&ch,"group heal");
  assert(strstr(output,"does not remove blindness"));
  registered(SPELL_CREATE_WATER,"create water"); command(&ch,"create water");
  assert(strstr(output,"becomes slime"));
  registered(SPELL_REMOVE_POISON,"remove poison"); command(&ch,"remove poison");
  assert(strstr(output,"food, drink containers, and fountains"));
  registered(PSIONIC_PSIONIC_BLAST,"psionic blast"); command(&ch,"psionic blast");
  assert(strstr(output,"Stuns eligible creatures") && strstr(output,"power resistance"));
  /* Synthetic unused boundary ID exercises the fallback renderer. */
  registered(PSIONIC_POWER_END,"undocumented test power"); command(&ch,"undocumented test power");
  assert(strstr(output,"Detailed effects are not yet documented.") && !strstr(output,"\tcDamage:"));
  /* Nonzero prep/summon settings remain hidden; damage and AC settings are shown. */
  CONFIG_PSIONIC_DAMAGE=25; CONFIG_ARCANE_DAMAGE=50; CONFIG_DIVINE_DAMAGE=0;
  CONFIG_ARCANE_PREP_TIME=CONFIG_DIVINE_PREP_TIME=CONFIG_ALCHEMY_PREP_TIME=67;
  CONFIG_PSIONIC_PREP_TIME=100; CONFIG_PLAYER_AC_CAP=70;
  CONFIG_SUMMON_LEVEL_1_10_HP=200; CONFIG_SUMMON_LEVEL_1_10_AC=150;
  CONFIG_SUMMON_LEVEL_1_10_HIT_DAM=150;
  CONFIG_SUMMON_LEVEL_11_20_HP=150; CONFIG_SUMMON_LEVEL_11_20_AC=125;
  CONFIG_SUMMON_LEVEL_11_20_HIT_DAM=125;
  CONFIG_SUMMON_LEVEL_21_30_HP=CONFIG_SUMMON_LEVEL_21_30_AC=CONFIG_SUMMON_LEVEL_21_30_HIT_DAM=115;
  command(&ch,"deadly fear");
  assert(strstr(output,"Psionic +25% (125% of normal damage)"));
  assert(strstr(output,"3d6 mental") && !strstr(output,"Preparation:"));
  CONFIG_PSIONIC_DAMAGE=40; command(&ch,"deadly fear");
  assert(strstr(output,"Psionic +40% (140% of normal damage)"));
  spell_info[SPELL_FIREBALL].min_level[CLASS_WIZARD]=5;
  command(&ch,"fireball");
  assert(strstr(output,"Wizard/Sorcerer/Bard +50% (150% of normal damage)"));
  assert(!strstr(output,"Cleric/Druid/Paladin/Ranger +0%"));
  assert(!strstr(output,"normal preparation time"));

  spell_info[SPELL_HEAL].min_level[CLASS_CLERIC]=11; command(&ch,"heal");
  assert(!strstr(output,"Global damage bonus:") && !strstr(output,"Divine 67%"));
  command(&ch,"mental barrier");
  assert(!strstr(output,"Global damage bonus:") && strstr(output,"total AC is capped at 70"));
  spell_info[SPELL_SUMMON_CREATURE_1].routines=MAG_SUMMONS;
  text=render_checked(SPELL_SUMMON_CREATURE_1);
  assert(!strstr(text,"maximum HP 200%") && !strstr(text,"each damage-dice dimension"));
  free(text);
  spell_info[SPELL_MAGIC_MISSILE].routines=MAG_LOOPS;
  spell_info[SPELL_MAGIC_MISSILE].min_level[CLASS_SUMMONER]=1;
  command(&ch,"magic missile");
  assert(!strstr(output,"Wizard/Sorcerer/Bard +50%"));
  assert(!strstr(output,"Arcane 67%"));
  assert(!strstr(output,"after the first damage event"));
  spell_info[SPELL_ACID_FOG].min_level[CLASS_WIZARD]=11;
  text=render_checked(SPELL_ACID_FOG);
  assert(!strstr(text,"Global damage bonus:") && strstr(text,"original caster's global damage bonus is not preserved")); free(text);
  spell_info[SPELL_CURE_LIGHT].min_level[CLASS_ALCHEMIST]=1;
  text=render_checked(SPELL_CURE_LIGHT);
  assert(!strstr(text,"Alchemy 67%") && !strstr(text,"Global damage bonus:")); free(text);
  CONFIG_PSIONIC_DAMAGE=0; command(&ch,"deadly fear");
  assert(!strstr(output,"Global damage bonus:") && !strstr(output,"Damage calculation:"));
  CONFIG_ARCANE_DAMAGE=CONFIG_DIVINE_DAMAGE=0;
  command(&ch,"fireball");
  assert(!strstr(output,"Global damage bonus:") && !strstr(output,"Damage calculation:"));
  text=render_checked(SPELL_ACID_FOG);
  assert(!strstr(text,"Periodic damage:")); free(text);
  CONFIG_ARCANE_DAMAGE=-10; command(&ch,"fireball");
  assert(strstr(output,"Global damage bonus:") && strstr(output,"-10% (90% of normal damage)"));
  /* Load the actual spello() implementation and literal calls for this batch. */
  load_batch_registration();
  for(i=1; i<=550; i++) {
    struct spell_info_type registration_before=spell_info[i];
    struct config_data config_before=config_info;
    struct char_data player_before=ch;
    struct player_special_data special_before=ps;
    text=render_checked(i);
    if((i>=325 && i<=381) || i==485 || i==490 || (i>=541 && i<=550)) {
      assert(!text && !spellhelp_public(i));
      assert(!memcmp(&registration_before,&spell_info[i],sizeof(registration_before)));
      assert(!memcmp(&config_before,&config_info,sizeof(config_before)));
      assert(!memcmp(&player_before,&ch,sizeof(ch)) && !memcmp(&special_before,&ps,sizeof(ps)));
      continue;
    }
    assert(text && !strstr(text,"Detailed effects are not yet documented"));
    if(i==1 || i==41 || i==47 || i==49 || i==420 || i==486) {
      assert(!spellhelp_public(i) && strstr(text,"Unavailable: legacy ID"));
    } else if(i==538 || i==441 || i==76 || i==161 || i==219 || i==271 || i==275 || i==304 || i==306 || i==382 || i==387 || i==388 || (i>=142 && i<=144)) {
      assert(!spellhelp_public(i) && strstr(text,"internal creature ability"));
    } else if(i>=501 && i<=503) {
      assert(!spellhelp_public(i) && strstr(text,"normal player casting disabled"));
    } else if(i>=504 && i<=506) {
      assert(!spellhelp_public(i) && strstr(text,"normal player casting explicitly disabled"));
    } else if(i==402 || i==445 || i==446 || i==447) {
      assert(!spellhelp_public(i) && strstr(text,"normal player casting disabled"));
    } else if((i>=241 && i<=243) || i==274 || i==277 || i==294 ||
              i==302 || i==315 || i==317 || i==318 || i==321) {
      assert(!spellhelp_public(i) && strstr(text,"Unavailable: unfinished ID"));
#if defined(CAMPAIGN_DL) || defined(CAMPAIGN_FR)
    } else if(i==239) {
      assert(!spellhelp_public(i) && strstr(text,"not registered for this campaign"));
#endif
    } else {
      assert(spellhelp_public(i) && strstr(text,"Description:") && strstr(text,"Damage:"));
    }
    assert(!memcmp(&registration_before,&spell_info[i],sizeof(registration_before)));
    assert(!memcmp(&config_before,&config_info,sizeof(config_before)));
    assert(!memcmp(&player_before,&ch,sizeof(ch)) && !memcmp(&special_before,&ps,sizeof(ps)));
    free(text);
  }
  command(&ch,"armor"); assert(strstr(output,"Unavailable") && strstr(output,"Mage Armor"));
  command(&ch,"ventriloquate"); assert(strstr(output,"cannot currently be cast"));
  command(&ch,"group armor"); assert(strstr(output,"Unavailable"));
  command(&ch,"group recall"); assert(strstr(output,"Unavailable") && strstr(output,"retained group handler"));
  command(&ch,"teleport"); assert(strstr(output,"Moves the caster") && strstr(output,"does not move the target"));
  command(&ch,"control weather"); assert(strstr(output,"currently makes no change"));
  command(&ch,"sanctuary"); assert(strstr(output,"no connected damage-reduction"));
  command(&ch,"burning hands"); assert(strstr(output,"maximum 8d6") && strstr(output,"single-target"));
  command(&ch,"chill touch"); assert(strstr(output,"maximum 5d4") && strstr(output,"Strength by 2"));
  command(&ch,"color spray"); assert(strstr(output,"1d4 illusion") && strstr(output,"level 5 or lower"));
  command(&ch,"lightning bolt"); assert(strstr(output,"maximum 15d10"));
  command(&ch,"harm"); assert(strstr(output,"Nd8 + N unholy damage") && strstr(output,"converts this spell to Heal"));
  command(&ch,"energy drain"); assert(strstr(output,"before damage resistance") && strstr(output,"Death Ward"));
  command(&ch,"poison"); assert(strstr(output,"quarters normal HP, PSP, and movement regeneration"));
  assert(strstr(output,"2d4 poison damage when recast"));
  command(&ch,"animate dead"); assert(strstr(output,"giant skeleton at 20-29") && strstr(output,"corpse's contents transfer"));
  command(&ch,"clone"); assert(strstr(output,"No sacrificial-entrails component"));
  command(&ch,"enchant item"); assert(strstr(output,"unworn, nonmagical") && strstr(output,"caster level / 7"));
  command(&ch,"identify"); assert(strstr(output,"identification statistics") && !strstr(output,"Unavailable"));
  command(&ch,"stone skin"); assert(strstr(output,"damage reduction 10") && strstr(output,"adamantine"));
  command(&ch,"mirror image"); assert(strstr(output,"Does not intercept spell damage"));
  command(&ch,"epic warding"); assert(strstr(output,"75 damage") && strstr(output,"105 per hit") && strstr(output,"three rounds"));
  command(&ch,"epic mage armor"); assert(strstr(output,"+20 armor bonus") && strstr(output,"+3 dodge bonus"));
  command(&ch,"greater ruin"); assert(strstr(output,"150% epic multiplier") && strstr(output,"reduced by 10"));
  command(&ch,"hellball"); assert(strstr(output,"No saving throw or spell resistance") && strstr(output,"two later rounds"));
  command(&ch,"dragon knight"); assert(strstr(output,"Every three combat rounds") && strstr(output,"one epic ally"));
  assert(!strstr(output,"Global damage bonus:"));
  command(&ch,"mummy dust"); assert(strstr(output,"-4 attack and -4 AC") && strstr(output,"epic preparation cast"));
  command(&ch,"cause light wound"); assert(strstr(output,"1d12 + min(7, caster level) holy damage") && strstr(output,"matching Cure spell"));
  command(&ch,"destruction"); assert(strstr(output,"rather than a separate instant-death check"));
  command(&ch,"fire breathe"); assert(strstr(output,"internal creature ability") && !spellhelp_public(SPELL_FIRE_BREATHE));
  command(&ch,"acid arrow"); assert(strstr(output,"3d6 acid damage per timed arrow") && strstr(output,"current combat opponent"));
  assert(!strstr(output,"Global damage bonus:"));
  command(&ch,"web"); assert(strstr(output,"preventing normal departure") && strstr(output,"does not check spell resistance"));
  command(&ch,"touch of idiocy"); assert(strstr(output,"separately rolled 1d6"));
  command(&ch,"true strike"); assert(strstr(output,"consumes the effect"));
  command(&ch,"polymorph self"); assert(strstr(output,"wildshape return") && strstr(output,"restores up to"));
  command(&ch,"continual flame"); assert(strstr(output,"creates a room fire")==NULL && strstr(output,"does not damage a target"));
  command(&ch,"scorching ray"); assert(strstr(output,"4d6 fire damage per ray") && strstr(output,"No saving throw"));
  command(&ch,"deafness"); assert(strstr(output,"one-in-five failure") && strstr(output,"50 rounds"));
  command(&ch,"false life"); assert(strstr(output,"+30 maximum HP") && strstr(output,"immediately adds 30"));
  command(&ch,"resist energy"); assert(strstr(output,"Subtracts 3 damage per hit") && strstr(output,"not 3%"));
  command(&ch,"endure elements"); assert(strstr(output,"10 percentage points") && strstr(output,"water"));
  command(&ch,"expeditious retreat"); assert(strstr(output,"adds 30 to movement speed"));
  command(&ch,"vampiric touch"); assert(strstr(output,"unholy damage") && strstr(output,"not calculated from damage actually dealt"));
  assert(strstr(output,"Requires a successful touch attack"));
  command(&ch,"halt undead"); assert(strstr(output,"level 11 or lower") && strstr(output,"1d3 + 1 rounds"));
  command(&ch,"deep slumber"); assert(strstr(output,"pixie casters bypass") && strstr(output,"below level 15"));
  command(&ch,"invisibility sphere"); assert(strstr(output,"Normal casting requires a group") && strstr(output,"+4 enhancement"));
  command(&ch,"clairvoyance"); assert(strstr(output,"without moving the caster") && strstr(output,"Non Detection"));
  command(&ch,"haste"); assert(strstr(output,"extra attack") && strstr(output,"30 to movement speed"));
  command(&ch,"slow"); assert(strstr(output,"duration is zero") && strstr(output,"next affect update"));
  command(&ch,"dispel magic"); assert(strstr(output,"first affect record") && strstr(output,"On self, removes all"));
  command(&ch,"acid splash"); assert(strstr(output,"2d4 to 6d4 acid damage") && strstr(output,"7, 15, 22, and 30"));
  command(&ch,"wizard eye"); assert(strstr(output,"original body remains") && strstr(output,"use return"));
  command(&ch,"fire shield"); assert(strstr(output,"50 percentage points of cold") && strstr(output,"1d6 fire damage"));
  assert(!strstr(output,"Global damage bonus:"));
  command(&ch,"cold shield"); assert(strstr(output,"50 percentage points of fire") && strstr(output,"1d6 cold damage"));
  command(&ch,"acid sheath"); assert(strstr(output,"50 percentage points of earth") && strstr(output,"2d6 acid damage"));
  command(&ch,"greater invisibility"); assert(strstr(output,"non-forced appearance") && strstr(output,"Self-only"));
  command(&ch,"minor globe"); assert(strstr(output,"class level is 4 or less") && strstr(output,"Device casts"));
  command(&ch,"reduce person"); assert(strstr(output,"does not add separate Dexterity or Strength"));
  command(&ch,"fire shield damage"); assert(!spellhelp_public(SPELL_FSHIELD_DAM) && strstr(output,"automatic effect") && strstr(output,"1d6 fire"));
  command(&ch,"cold shield damage"); assert(!spellhelp_public(SPELL_CSHIELD_DAM) && strstr(output,"1d6 cold"));
  command(&ch,"acid sheath damage"); assert(!spellhelp_public(SPELL_ASHIELD_DAM) && strstr(output,"2d6 acid"));
  command(&ch,"wall of force"); assert(strstr(output,"open exit") && strstr(output,"character level in object-timer ticks"));
  command(&ch,"cloudkill"); assert(strstr(output,"five-second affliction pulse") && strstr(output,"Arcane magic level d4 poison"));
  assert(!strstr(output,"Global damage bonus:"));
  command(&ch,"waves of fatigue"); assert(strstr(output,"-2 attack, -2 damage, and -2 AC") && strstr(output,"prevents normal movement regeneration"));
  command(&ch,"symbol of pain"); assert(strstr(output,"min(17, caster level)d6 unholy") && strstr(output,"not a placed symbol"));
  command(&ch,"dominate person"); assert(strstr(output,"100 + 4 times caster Charisma") && strstr(output,"No humanoid-only"));
  command(&ch,"feeblemind"); assert(strstr(output,"Intelligence, Wisdom, and Charisma") && strstr(output,"minus 3"));
  command(&ch,"nightmare"); assert(strstr(output,"Caster level d4 + 10 illusion") && strstr(output,"Fortitude negates fatigue"));
  command(&ch,"mind fog"); assert(strstr(output,"-10 on Will") && strstr(output,"single-target debuff"));
  command(&ch,"faithful hound"); assert(strstr(output,"hound as a charmed follower") && !strstr(output,"Summon scaling:"));
  command(&ch,"dismissal"); assert(strstr(output,"first charmed extraplanar") && strstr(output,"follower's master"));
  command(&ch,"cone of cold"); assert(strstr(output,"min(22, caster level)d6 cold") && strstr(output,"single-target"));
  command(&ch,"telekinesis"); assert(strstr(output,"59% chance") && strstr(output,"min(20, caster level)d4 bludgeoning"));
  command(&ch,"firebrand"); assert(strstr(output,"min(22, caster level)d6 fire"));
  command(&ch,"deathcloud"); assert(!spellhelp_public(SPELL_DEATHCLOUD) && strstr(output,"internal creature ability") && strstr(output,"Effect level d4 poison"));
  command(&ch,"freezing sphere"); assert(strstr(output,"min(24, caster level)d10") && strstr(output,"caster level / 2"));
  command(&ch,"summon creature vi"); assert(strstr(output,"dire tiger") && !strstr(output,"Summon scaling:"));
  command(&ch,"transformation"); assert(strstr(output,"base attack bonus to total character level") && strstr(output,"Prevents starting"));
  command(&ch,"eyebite"); assert(strstr(output,"three-fifths of maximum HP") && strstr(output,"75 rounds"));
  assert(!strstr(output,"Global damage bonus:"));
  command(&ch,"mass haste"); assert(strstr(output,"12 times caster level rounds per recipient") && strstr(output,"extra attack"));
  command(&ch,"greater heroism"); assert(strstr(output,"+4 morale") && strstr(output,"min(20, caster level)") && strstr(output,"+4 on skill checks, and fear immunity"));
  command(&ch,"anti magic field"); assert(strstr(output,"15 room-affect rounds") && strstr(output,"Existing affects are not removed"));
  command(&ch,"greater mirror image"); assert(strstr(output,"6 + caster level / 3") && strstr(output,"Does not intercept spell damage"));
  command(&ch,"true seeing"); assert(strstr(output,"bypasses normal hiding checks") && strstr(output,"20 + caster level"));
  command(&ch,"globe of invuln"); assert(strstr(output,"class level is 8 or less") && strstr(output,"Device casts and Warlock"));
  command(&ch,"greater dispelling"); assert(strstr(output,"2d2 dispels") && strstr(output,"Each successful attempt removes one affect record"));
  command(&ch,"grasping hand"); assert(strstr(output,"min(26, caster level)d6") && strstr(output,"2d4 - 1 rounds") && strstr(output,"separate Reflex save"));
  command(&ch,"summon creature vii"); assert(strstr(output,"level is set to 16") && !strstr(output,"Summon scaling:"));
  command(&ch,"power word blind"); assert(strstr(output,"No saving throw") && strstr(output,"No target-HP threshold"));
  command(&ch,"waves of exhaustion"); assert(strstr(output,"20 + caster level movement") && strstr(output,"No saving throw"));
  command(&ch,"mass hold person"); assert(strstr(output,"does not impose a level-11 cap") && strstr(output,"1d3 + 1 rounds"));
  command(&ch,"mass fly"); assert(strstr(output,"600 rounds per recipient") && strstr(output,"requires a group"));
  command(&ch,"displacement"); assert(strstr(output,"50% concealment") && strstr(output,"Blindsight bypasses"));
  command(&ch,"prismatic spray"); assert(strstr(output,"equal probability") && strstr(output,"min(26, caster level)d4 illusion") && strstr(output,"separate Will save"));
  command(&ch,"power word stun"); assert(strstr(output,"No saving throw") && strstr(output,"No target-HP threshold"));
  command(&ch,"protection from spells"); assert(strstr(output,"Adds 10 to spell resistance") && strstr(output,"+1 enhancement bonus to Will saves"));
  command(&ch,"thunderclap"); assert(strstr(output,"1d10 + caster level sound") && strstr(output,"Damage has no saving throw") && strstr(output,"Deafness immunity exits"));
  command(&ch,"spell mantle"); assert(strstr(output,"Absorbs two") && strstr(output,"3 times caster level rounds"));
  command(&ch,"mass wisdom"); assert(strstr(output,"2 + caster level / 5") && strstr(output,"enhancement bonus to Wisdom"));
  command(&ch,"mass charisma"); assert(strstr(output,"2 + caster level / 5") && strstr(output,"enhancement bonus to Charisma"));
  command(&ch,"clenched fist"); assert(strstr(output,"min(28, caster level)d11") && strstr(output,"100 seconds") && strstr(output,"one-in-three chance"));
  command(&ch,"incendiary cloud"); assert(strstr(output,"five-second affliction pulse") && strstr(output,"Arcane magic level d5"));
  assert(!strstr(output,"Global damage bonus:"));
  command(&ch,"summon creature viii"); assert(strstr(output,"level is set to 18") && !strstr(output,"Summon scaling:"));
  command(&ch,"horrid wilting"); assert(strstr(output,"min(28, caster level)d8") && strstr(output,"negative-energy damage"));
  command(&ch,"greater animation"); assert(strstr(output,"wight at 30+") && strstr(output,"set to level 18") && strstr(output,"corpse's contents transfer"));
  command(&ch,"irresistible dance"); assert(strstr(output,"checks the caster for Paralysis Resistance") && strstr(output,"regardless of the result"));
  command(&ch,"mass domination"); assert(strstr(output,"charmed followers") && strstr(output,"Will negates"));
  command(&ch,"scint pattern"); assert(strstr(output,"Confuses the target") && strstr(output,"2d4 + 2 rounds"));
  command(&ch,"refuge"); assert(strstr(output,"+15 to Move Silently") && strstr(output,"Refuge lasts 6 rounds"));
  command(&ch,"banish"); assert(strstr(output,"50% chance to stop") && strstr(output,"no extraplanar subrace"));
  command(&ch,"sunburst"); assert(strstr(output,"min(26, caster level)d5") && strstr(output,"Will for half damage; Reflex negates blindness") && strstr(output,"15 room-affect rounds"));
  command(&ch,"spell turning"); assert(strstr(output,"swapping caster and target") && strstr(output,"Device casts bypass"));
  command(&ch,"mind blank"); assert(strstr(output,"mind-affecting or charm immunity") && strstr(output,"50 rounds"));
  spell_info[SPELL_HORRID_WILTING].min_level[CLASS_WIZARD]=15;
  CONFIG_ARCANE_DAMAGE=50;
  command(&ch,"horrid wilting");
  assert(strstr(output,"Global damage bonus:") && strstr(output,"150% of normal damage"));
  CONFIG_ARCANE_DAMAGE=0;
  command(&ch,"horrid wilting"); assert(!strstr(output,"Global damage bonus:"));
  {
    static const struct {const char *name, *effect, *detail;} cases[] = {
      {"iron skin", "up to 50 damage", "35 times max(10, caster level)"},
      {"mass cunning", "Intelligence of 2 + caster level / 5", "requires a group"},
      {"blade of disaster", "level is set to 20", "Duration:"},
      {"summon creature ix", "level is set to 20", "10 out of 102"},
      {"wail of the banshee", "(caster level + 2)d5", "a separate Fortitude save negates fear"},
      {"power word kill", "10d8 mental damage", "121 HP or less"},
      {"enfeeblement", "Strength, Dexterity, and Constitution", "-4 modifier on the target's save"},
      {"weird", "Caster level d12", "caster for Phantasm Resistance"},
      {"shadow shield", "+5 shield AC", "physical damage reduction 12"},
      {"prismatic sphere", "runs its load trigger", "does not charm it"},
      {"implode", "current combat opponent", "Current caster level d6 piercing"},
      {"timestop", "14 rounds", "does not freeze every other creature"},
      {"greater spell mantle", "Absorbs four", "4 times caster level rounds"},
      {"mass enhance", "Strength, Dexterity, and Constitution", "2 + caster level / 5"},
      {"portal", "paired portals", "Two object-timer ticks"},
      {"gust of wind", "2d6 bludgeoning falling damage", "Spring Attack blocks"},
      {"holy weapon", "holyweapon command", "base damage dice and size"},
      {"incendiary", "internal creature ability", "Effect level d5 + effect level fire"},
      {"cure moderate", "3d4 + 10 + min(15, caster level)", "maximum"},
      {"cure serious", "4d4 + 15 + min(20, caster level)", "maximum"},
      {"remove fear", "fear and shaken spell effects", "Does not add lasting fear immunity"},
      {"cure deafness", "restoring ordinary hearing", "casting-fumble penalty"},
      {"faerie fog", "removing invisibility spell effects", "not a persistent room cloud"},
      {"mass cure light", "2d4 + 5 + min(10, caster level)", "requires a group"},
      {"aid", "2d6 + max(caster level, 15)", "minimum of 15"},
      {"bravery", "grants fear immunity", "25 + caster level rounds"},
      {"mass cure moderate", "3d4 + 10 + min(15, caster level)", "requires a group"},
      {"regeneration", "doubles normal HP regeneration", "4d4 + 15 + min(20, caster level)"},
      {"free movement", "Removes entangling spell effects", "paralysis and daze protection"},
      {"strengthen bones", "undead target +2 racial AC", "600 rounds"},
      {"mass cure serious", "4d4 + 15 + min(20, caster level)", "requires a group"},
      {"prayer", "+5 circumstance attack and damage", "4d12 + caster level maximum HP"},
      {"remove disease", "Removes Eyebite", "Does not grant lasting"},
      {"word of faith", "no effect handler", "No blindness, stun, death, or banishment"},
      {"dimensional lock", "Will negates", "Caster level + 12 rounds"},
      {"salvation", "records the current room", "consumes the saved Salvation flag"},
      {"spring of life", "Creates a spring object", "does not roll direct healing"},
      {"plane shift", "astral, ethereal, elemental, or prime", "Does not move the entire group"},
      {"storm of vengeance", "Ice Storm after six seconds", "min(28, current caster level)d9"},
      {"death shield", "unfinished ID", "no spell registration"},
      {"command", "unfinished ID", "no spell registration"},
      {"air walker", "unfinished ID", "no spell registration"},
      {"group summon", "Summons group members", "Dimensional Lock"},
      {"mass cure critic", "6d4 + 20 + min(25, caster level)", "requires a group"},
      {"charm animal", "animal NPC", "does not impose the level-7 cap"},
      {"faerie fire", "imposes -2 AC", "No saving throw"},
      {"goodberry", "food-template item", "configured food bonuses"},
      {"jump", "+10 to Acrobatics", "combined Druid and Ranger class levels"},
      {"magic fang", "+1 enhancement attack bonus", "does not add damage dice"}
    };
    size_t entry;
    for(entry=0;entry<sizeof(cases)/sizeof(cases[0]);entry++) {
      command(&ch,cases[entry].name);
      assert(strstr(output,cases[entry].effect));
      assert(strstr(output,cases[entry].detail));
    }
  }
  command(&ch,"gate");
#if defined(CAMPAIGN_DL) || defined(CAMPAIGN_FR)
  assert(strstr(output,"no gate is created") && !spellhelp_public(SPELL_PLANE_SHIFT));
#else
  assert(strstr(output,"two-way portal") && spellhelp_public(SPELL_PLANE_SHIFT));
#endif
  spell_info[SPELL_SHADOW_SHIELD].min_level[CLASS_WIZARD]=17;
  command(&ch,"shadow shield"); assert(strstr(output,"Armor class limit:") && strstr(output,"70"));
  spell_info[SPELL_GUST_OF_WIND].min_level[CLASS_WIZARD]=3;
  CONFIG_ARCANE_DAMAGE=50;
  command(&ch,"gust of wind"); assert(strstr(output,"Global damage bonus:") && strstr(output,"150% of normal damage"));
  spell_info[SPELL_IMPLODE].min_level[CLASS_CLERIC]=17;
  spell_info[SPELL_STORM_OF_VENGEANCE].min_level[CLASS_CLERIC]=17;
  CONFIG_DIVINE_DAMAGE=25;
  command(&ch,"implode"); assert(!strstr(output,"Global damage bonus:"));
  command(&ch,"storm of vengeance"); assert(!strstr(output,"Global damage bonus:"));
  {
    static const struct {const char *name, *effect, *detail;} cases[] = {
      {"magic stone", "1d6 + 1 bludgeoning", "up to five stones"},
      {"obscuring mist", "20% concealment", "divine caster level"},
      {"produce flame", "1d6 + min(5, caster level) fire", "underwater"},
      {"natures ally i", "dire badger", "Duration:"},
      {"natures ally ii", "dire boar", "Duration:"},
      {"natures ally iii", "dire wolf", "Duration:"},
      {"natures ally iv", "dire spider", "Duration:"},
      {"natures ally v", "dire bear", "Duration:"},
      {"natures ally vi", "dire tiger", "Duration:"},
      {"natures ally vii", "level is set to 16", "10 out of 102"},
      {"natures ally viii", "level is set to 18", "10 out of 102"},
      {"natures ally ix", "level is set to 20", "10 out of 102"},
      {"barkskin", "+5 at level 12", "200 times caster level rounds"},
      {"flame blade", "1d8 + min(10, caster level / 2)", "does not create a weapon"},
      {"flaming sphere", "2d6 fire damage", "Does not create a persistent sphere"},
      {"hold animal", "level 11 or lower", "1d3 + 1 rounds"},
      {"call lightning storm", "Nd12 + N + 20", "min(24, caster level)"},
      {"summon swarm", "1d6 + max(caster level, 5)", "currently continues into damage"},
      {"contagion", "Intelligence -1d4", "No initial saving throw"},
      {"greater magic fang", "actual Druid class level", "+5 at 20 or higher"},
      {"frost breathe", "Source character level d16 cold", "internal creature ability"},
      {"spike growth", "2d4 earth damage", "ridden mount takes the damage"},
      {"blight", "Nd6 + N earth damage", "min(15, caster level)"},
      {"reincarnate", "unimplemented numeric ID", "Unavailable: unfinished ID"},
      {"lightning breathe", "Source character level d16 electricity", "internal creature ability"},
      {"spike stone", "4d4 earth damage", "no separate flight immunity"},
      {"baleful polymorph", "unimplemented numeric ID", "Unavailable: unfinished ID"},
      {"death ward", "10 unholy damage and 20 negative-energy", "blocks Energy Drain"},
      {"hallow", "prevents evil characters", "1000 room-affect rounds"},
      {"insect plague", "min(6, caster level / 3)d6", "Wind Wall blocks"},
      {"unhallow", "prevents good characters", "1000 room-affect rounds"},
      {"wall of fire", "4d6 + creator character level fire", "Eligible creatures can pass through"},
      {"wall of thorns", "4d6 + creator character level earth", "no spell-resistance check"},
      {"fire seeds", "Source character level d4 + caster level", "no spell-resistance check"},
      {"confusion", "random room occupant", "Will negates"},
      {"mass endurance", "Constitution of 2 + caster level / 5", "Mass Enhance blocks"},
      {"mass strength", "Strength of 2 + caster level / 5", "Mass Enhance blocks"},
      {"mass grace", "Dexterity of 2 + caster level / 5", "Mass Enhance blocks"},
      {"magic resistance", "12 + the recipient's total character level", "resistance floor"},
      {"spellstaff", "one charge", "Barkskin, Cure Light Wounds, Endurance, Flame Strike, and Strength"},
      {"transport via plants", "Moves the caster", "city, field, forest"},
      {"creeping doom", "pulses do not deliver damage or poison", "five-second pulses"},
      {"fire storm", "min(20, caster level)d6 fire", "Reflex for half"},
      {"greater scrying", "unimplemented numeric ID", "Unavailable: unfinished ID"},
      {"sunbeam", "4d6 + caster level light", "a separate Reflex save negates blindness"},
      {"animal shapes", "bear (+8 Strength, +6 Constitution, +60 maximum HP)", "100 rounds per recipient"},
      {"control plants", "plant NPC", "Will negates"},
      {"finger of death", "1 + 10 times caster level unholy", "3d6 + min(25, caster level) unholy"},
      {"elemental swarm", "2d4 charmed elementals", "one randomly selected type"},
      {"aoe attack", "3d6 + 10 piercing", "no dedicated standalone spell-damage formula"}
    };
    size_t entry;
    for(entry=0;entry<sizeof(cases)/sizeof(cases[0]);entry++) {
      command(&ch,cases[entry].name);
      assert(strstr(output,cases[entry].effect));
      assert(strstr(output,cases[entry].detail));
    }
  }
  spell_info[SPELL_BARKSKIN].min_level[CLASS_DRUID]=3;
  command(&ch,"barkskin"); assert(strstr(output,"Armor class limit:") && strstr(output,"70"));
  spell_info[SPELL_FIRE_STORM].min_level[CLASS_DRUID]=13;
  CONFIG_DIVINE_DAMAGE=25;
  command(&ch,"fire storm"); assert(strstr(output,"Global damage bonus:") && strstr(output,"125% of normal damage"));
  CONFIG_DIVINE_DAMAGE=0;
  command(&ch,"fire storm"); assert(!strstr(output,"Global damage bonus:"));
  CONFIG_DIVINE_DAMAGE=25;
  spell_info[SPELL_SPIKE_GROWTH].min_level[CLASS_DRUID]=5;
  spell_info[SPELL_WALL_OF_FIRE].min_level[CLASS_DRUID]=7;
  command(&ch,"spike growth"); assert(!strstr(output,"Global damage bonus:"));
  command(&ch,"wall of fire"); assert(!strstr(output,"Global damage bonus:"));
  command(&ch,"creeping doom"); assert(!strstr(output,"Global damage bonus:"));
  command(&ch,"animal shapes"); assert(strstr(output,"+4 Stealth") && strstr(output,"base 2d6 damage dice") && strstr(output,"suppress wielded weapons and dual wielding"));
  {
    static const struct {const char *name, *effect, *detail;} cases[] = {
      {"shambler", "1d4 + 2", "Duration:"},
      {"shapechange", "unimplemented numeric ID", "Unavailable: unfinished ID"},
      {"blade barrier", "25d6 + 2 slashing", "including the original caster and allies"},
      {"blades", "Effect level d6 + 2 slashing", "internal creature ability"},
      {"battletide", "+3 enhancement attack and damage", "caps by 25"},
      {"innate darkness", "internal creature ability", "10 combat rounds"},
      {"doom", "-4 on attack rolls and -4 on skill checks", "10 times caster level rounds"},
      {"whirlwind", "(caster level + 1d3)d6", "air damage"},
      {"levitate", "preventing ordinary falling", "600 rounds"},
      {"draconic bloodline breath weapon", "actual Sorcerer class level d6", "otherwise Dd14"},
      {"vigorize light", "20d4 + 50 movement points", "rather than HP or PSP"},
      {"vigorize serious", "40d4 + 150 movement points", "rather than HP or PSP"},
      {"vigorize critical", "60d5 + 150 + 10 times caster level movement", "maximum movement"},
      {"group vigorize", "60d5 + 150 + 10 times caster level", "requires a group"},
      {"embalm", "unimplemented numeric ID", "Unavailable: unfinished ID"},
      {"continual light", "magical-light object", "does not create a room fire"},
      {"elemental resistance", "unimplemented numeric ID", "Unavailable: unfinished ID"},
      {"preserve", "unimplemented numeric ID", "Unavailable: unfinished ID"},
      {"resurrection", "online player", "Does not directly refill HP"},
      {"silence", "Will negates", "Psionic powers and Alchemist extracts"},
      {"minor create", "unimplemented numeric ID", "Unavailable: unfinished ID"},
      {"dispel invis", "invisible items in the caster's inventory", "carried/equipped object"},
      {"shield of faith", "min(5, 2 + caster level / 6)", "deflection AC"},
      {"group shield of faith", "min(5, 2 + caster level / 6)", "requires a group"}
    };
    size_t entry;
    for(entry=0;entry<sizeof(cases)/sizeof(cases[0]);entry++) {
      command(&ch,cases[entry].name);
      assert(strstr(output,cases[entry].effect));
      assert(strstr(output,cases[entry].detail));
    }
  }
  command(&ch,"battletide"); assert(strstr(output,"Prevents starting ordinary spell or power casts"));
  command(&ch,"resurrection"); assert(strstr(output,"9 times (stored XP / 10)") && strstr(output,"12 seconds"));
  command(&ch,"shield of faith"); assert(strstr(output,"Armor class limit:") && strstr(output,"70"));
  command(&ch,"group shield of faith"); assert(strstr(output,"Armor class limit:") && strstr(output,"70"));
  spell_info[SPELL_WHIRLWIND].min_level[CLASS_DRUID]=15;
  spell_info[SPELL_BLADE_BARRIER].min_level[CLASS_CLERIC]=11;
  CONFIG_DIVINE_DAMAGE=25;
  command(&ch,"whirlwind"); assert(strstr(output,"Global damage bonus:") && strstr(output,"125% of normal damage"));
  command(&ch,"blade barrier"); assert(!strstr(output,"Global damage bonus:"));
  command(&ch,"vigorize critical"); assert(!strstr(output,"Global damage bonus:"));
  CONFIG_DIVINE_DAMAGE=0;
  command(&ch,"whirlwind"); assert(!strstr(output,"Global damage bonus:"));
  command(&ch,"vigorize light"); assert(strstr(output,"dissolves Boneshard bleeding"));
  command(&ch,"group vigorize"); assert(strstr(output,"dissolves Boneshard bleeding on recipients"));
  {
    static const struct {const char *name, *effect, *detail;} cases[] = {
      {"electric shield damage", "1d6 electricity", "internal creature ability"},
      {"psychokinetic tincture", "min(5, max(1, actual Alchemist level / 4))", "50 times actual Alchemist class level"},
      {"augury", "zone reset instructions", "random-treasure chances"},
      {"psychokinetic fear", "-2 Wisdom", "does not currently require an active tincture"},
      {"mass false life", "+30 circumstance maximum HP", "Recipients:"},
      {"acid breathe", "Source character level d16 acid", "internal creature ability"},
      {"poison breathe", "Source character level d16 poison", "no matching timed poison-effect case"},
      {"entangle", "-2 Dexterity", "10 + caster level rounds"},
      {"dragon fear", "5d6 rounds", "immediately attempt fleeing twice"},
      {"cause fear", "one-in-three chance", "2d6 rounds"},
      {"shadow walk", "base movement speed to 400", "minimum room movement cost from 5 to 1"},
      {"circle of death", "min(20, caster level)d6 negative-energy", "Undead receive healing"},
      {"undeath to death", "min(20, caster level)d6 holy", "does not restrict targets to undead"},
      {"grasp of the dead", "Caster level d6 slashing", "does not apply entanglement"},
      {"grave touch", "-1 enhancement AC", "Fortitude negates"},
      {"incorporeal form", "+1 enhancement AC", "3 rounds"},
      {"lesser missile storm", "1d6 energy damage per missile", "min(10, caster level)"},
      {"shadow jump", "Moves the caster", "Both source and target must satisfy shadow conditions"},
      {"unholy weapon", "Unholy Avenger", "unholyweapon command"}
    };
    size_t entry;
    for(entry=0;entry<sizeof(cases)/sizeof(cases[0]);entry++) {
      command(&ch,cases[entry].name);
      assert(strstr(output,cases[entry].effect));
      assert(strstr(output,cases[entry].detail));
    }
  }
  assert(spellhelp_public(ALC_DISC_AFFECT_PSYCHOKINETIC) && spellhelp_public(PSYCHOKINETIC_FEAR));
  command(&ch,"psychokinetic tincture"); assert(strstr(output,"Armor class limit:") && strstr(output,"70"));
  command(&ch,"incorporeal form"); assert(strstr(output,"Armor class limit:") && strstr(output,"70"));
  command(&ch,"shadow walk"); assert(strstr(output,"Recipients:") && strstr(output,"Group members in the room"));
  spell_info[SPELL_CIRCLE_OF_DEATH].min_level[CLASS_WIZARD]=11;
  CONFIG_ARCANE_DAMAGE=50;
  command(&ch,"circle of death"); assert(strstr(output,"Global damage bonus:") && strstr(output,"150% of normal damage"));
  spell_info[SPELL_LESSER_MISSILE_STORM].min_level[CLASS_WIZARD]=7;
  command(&ch,"lesser missile storm"); assert(strstr(output,"Repeated hits:") && strstr(output,"first damage event"));
  command(&ch,"psychokinetic tincture"); assert(!strstr(output,"Global damage bonus:"));
  CONFIG_ARCANE_DAMAGE=0;
  command(&ch,"circle of death"); assert(!strstr(output,"Global damage bonus:"));

  /* Batch 401-450: authored mechanics and read-only command rendering. */
  command(&ch,"divine favor"); assert(strstr(output,"Grants a luck bonus to attack and damage rolls equal to min(3, ma"));
  command(&ch,"silverymoon recall"); assert(strstr(output,"Retained recall handler transports the selected player to Silvery"));
  command(&ch,"triboar recall"); assert(strstr(output,"Retained recall handler transports the selected player to Triboar"));
  command(&ch,"luskan recall"); assert(strstr(output,"Retained recall handler transports the selected player to Luskan."));
  command(&ch,"mirabar recall"); assert(strstr(output,"Retained recall handler transports the selected player to Mirabar"));
  command(&ch,"heal mount"); assert(strstr(output,"Restores 10 times caster level + 20 HP to the caster's paladin mo"));
  command(&ch,"resistance"); assert(strstr(output,"Grants +1 resistance bonus to Fortitude, Reflex, and Will saves."));
  command(&ch,"lesser restoration"); assert(strstr(output,"Currently does not remove ability penalties or other conditions: "));
  command(&ch,"restoration"); assert(strstr(output,"Currently does not remove ability penalties or other conditions: "));
  command(&ch,"hedging weapons"); assert(strstr(output,"Creates a recorded weapon count and matching initial deflection A"));
  command(&ch,"honeyed tongue"); assert(strstr(output,"Grants +5 competence to Diplomacy and takes the higher of the exi"));
  command(&ch,"shield of fortification"); assert(strstr(output,"Gives a 25% chance to prevent a critical hit and a separate 25% c"));
  command(&ch,"stunning barrier"); assert(strstr(output,"Grants +1 deflection AC and +1 resistance to all three saves"));
  command(&ch,"sun metal"); assert(strstr(output,"Adds 1d6 fire damage to eligible weapon hits through the flaming-"));
  command(&ch,"tactical acumen"); assert(strstr(output,"Grants an attack bonus of max(2, min(5, caster level / 4)) when t"));
  command(&ch,"veil of positive energy"); assert(strstr(output,"Grants +2 sacred AC against undead attackers and +2 on saves agai"));
  command(&ch,"bestow weapon proficiency"); assert(strstr(output,"Treats the recipient as proficient with all weapons while active."));
  command(&ch,"effortless armor"); assert(strstr(output,"Reduces armor check penalties by 1 + min(5, caster level / 5), cl"));
  command(&ch,"fire of entanglement"); assert(strstr(output,"Normal casting currently applies no effect: its registered affect"));
  command(&ch,"life shield"); assert(strstr(output,"Normal casting currently creates no shield or absorption reserve:"));
  command(&ch,"litany of defense"); assert(strstr(output,"Doubles the shield material/enhancement contribution in the equip"));
  command(&ch,"litany of righteousness"); assert(strstr(output,"While a group member carries this litany, an attacker with an aur"));
  command(&ch,"righteous vigor"); assert(strstr(output,"Starts with +1 morale attack bonus"));
  command(&ch,"sacred space"); assert(strstr(output,"Retained room handler sanctifies the room: evil occupants take -1"));
  command(&ch,"dancing weapon"); assert(strstr(output,"Schedules one attack per positive base-attack step (BAB, BAB - 5,"));
  command(&ch,"spiritual weapon"); assert(strstr(output,"Schedules one attack per positive base-attack step (BAB, BAB - 5,"));
  command(&ch,"undetectable alignment"); assert(strstr(output,"Sets the magically hidden alignment flag, concealing alignment fr"));
  command(&ch,"weapon of awe"); assert(strstr(output,"Grants +2 sacred damage"));
  command(&ch,"blinding ray"); assert(strstr(output,"Fires 1 + min(3, (caster level - 3) / 4) rays, using integer divi"));
  command(&ch,"holy javelin"); assert(strstr(output,"Requires an evil initial target, a ranged touch hit, and spell pe"));
  command(&ch,"invisibility purge"); assert(strstr(output,"Immediately reveals eligible creatures in the room and removes in"));
  command(&ch,"keen edge"); assert(strstr(output,"Expands critical threat range for piercing and slashing weapons t"));
  command(&ch,"weapon of impact"); assert(strstr(output,"Expands critical threat range for bludgeoning weapons and unarmed"));
  command(&ch,"greater magic weapon"); assert(strstr(output,"Provides an enhancement attack and damage bonus of min(5, caster "));
  command(&ch,"magic vestment"); assert(strstr(output,"Provides min(5, caster level / 4) enhancement for each qualifying"));
  command(&ch,"protection from energy"); assert(strstr(output,"Reduces each hit of fire, cold, air, earth, acid, electricity, so"));
  command(&ch,"communal protection from energy"); assert(strstr(output,"Applies Protection from Energy to group members in the room"));
  command(&ch,"searing light"); assert(strstr(output,"Deals light damage to one target, with damage dice determined by "));
  command(&ch,"divine power"); assert(strstr(output,"Grants caster level / 6 luck bonuses to Strength, attack, and dam"));
  command(&ch,"wind wall"); assert(strstr(output,"Surrounds the recipient with a wind wall"));
  command(&ch,"air walk"); assert(strstr(output,"Grants the normal Flying state, enabling movement handled by the "));
  command(&ch,"gaseous form"); assert(strstr(output,"Grants immaterial and levitating states"));
  command(&ch,"gas breathe"); assert(strstr(output,"Internal room-wide gas breath damage against eligible creatures."));
  command(&ch,"minor illusion"); assert(strstr(output,"Sets the mirror-image flag and image count to one"));
  command(&ch,"moonbeam"); assert(strstr(output,"Schedules five light-damage events at zero, six, twelve, eighteen"));
  command(&ch,"hellish rebuke"); assert(strstr(output,"Immediately deals fire damage to the selected target."));
  command(&ch,"ant haul"); assert(strstr(output,"Grants +6 enhancement to the Strength value used for encumbrance "));
  command(&ch,"mass ant haul"); assert(strstr(output,"Applies Ant Haul to group members in the room"));
  command(&ch,"corrosive touch"); assert(strstr(output,"Despite the name, this registration does not enable"));
  spell_info[SPELL_CORROSIVE_TOUCH].min_level[CLASS_WIZARD]=1;
  CONFIG_ARCANE_DAMAGE=50;
  command(&ch,"corrosive touch"); assert(strstr(output,"Global damage bonus:") && strstr(output,"150% of normal damage"));
  spell_info[SPELL_MOONBEAM].min_level[CLASS_DRUID]=3;
  CONFIG_DIVINE_DAMAGE=25;
  command(&ch,"moonbeam"); assert(!strstr(output,"Global damage bonus:"));
  command(&ch,"magic vestment"); assert(strstr(output,"Armor class limit:"));
  CONFIG_ARCANE_DAMAGE=CONFIG_DIVINE_DAMAGE=0;

  /* Batch 451-500: real registration names and reviewed mechanics. */
  command(&ch,"planar healing"); assert(strstr(output,"Grants fast healing 1. The current fast-healing helper cou"));
  command(&ch,"greater planar healing"); assert(strstr(output,"Grants fast healing 4. The current fast-healing helper cou"));
  command(&ch,"cushioning bands"); assert(strstr(output,"Provides damage reduction 2, bypassed by slashing or pierc"));
  command(&ch,"ghost wolf"); assert(strstr(output,"Summons a ghost-wolf follower with base level min(15, max("));
  command(&ch,"gird allies"); assert(strstr(output,"Grants pets in the room whose masters share the caster's g"));
  command(&ch,"protection from arrows"); assert(strstr(output,"Forces the next three eligible ranged attack attempts to m"));
  command(&ch,"spider climb"); assert(strstr(output,"Allows passage through the normal climbing movement check "));
  command(&ch,"warding weapon"); assert(strstr(output,"Grants +6 Concentration and +2 circumstance AC."));
  command(&ch,"aqueous orb"); assert(strstr(output,"Schedules 1 + caster level events six seconds apart, begin"));
  command(&ch,"summon mount"); assert(strstr(output,"Summons a mount follower using the summon-mount creature t"));
  command(&ch,"communal summon mount"); assert(strstr(output,"Summons a mount for each eligible group member in the room"));
  command(&ch,"human potential"); assert(strstr(output,"Grants the selected character +2 racial bonus to one chose"));
  command(&ch,"mass human potential"); assert(strstr(output,"Grants recipients in the room matching the caster's group "));
  command(&ch,"black tentacles"); assert(strstr(output,"Entangles eligible creatures in the room, restricting move"));
  command(&ch,"greater black tentacles"); assert(strstr(output,"Entangles eligible creatures in the room, restricting move"));
  command(&ch,"control summoned creature"); assert(strstr(output,"Wrests a charmed summoned NPC with an existing master into"));
  command(&ch,"charm monster"); assert(strstr(output,"Charms the selected creature into following the caster, en"));
  command(&ch,"mass charm monster"); assert(strstr(output,"Normal casting currently charms no creatures: registration"));
  command(&ch,"mass enlarge person"); assert(strstr(output,"Applies Enlarge Person to group members in the room: one s"));
  command(&ch,"mass reduce person"); assert(strstr(output,"Applies Reduce Person to group members in the room: one si"));
  command(&ch,"communal protection from arrows"); assert(strstr(output,"Applies Protection from Arrows to group members in the roo"));
  command(&ch,"rage"); assert(strstr(output,"Applies rage affects to group members in the room. The cur"));
  command(&ch,"communal resist energy"); assert(strstr(output,"Applies Resist Energy to group members in the room: subtra"));
  command(&ch,"siphon might"); assert(strstr(output,"Transfers Strength from an enemy to a selected recipient: "));
  command(&ch,"communal spider climb"); assert(strstr(output,"Applies Spider Climb to group members in the room, bypassi"));
  command(&ch,"caustic blood"); assert(strstr(output,"Acidic blood retaliates against eligible non-ranged attack"));
  command(&ch,"mass daze monster"); assert(strstr(output,"Applies Daze Monster to group recipients in the room: stun"));
  command(&ch,"hold monster"); assert(strstr(output,"Paralyzes one living creature, preventing ordinary actions"));
  command(&ch,"overland flight"); assert(strstr(output,"Starts travel to a named destination using the campaign fl"));
  command(&ch,"communal stone skin"); assert(strstr(output,"Applies Stone Skin to group members in the room: damage re"));
  command(&ch,"hostile juxtaposition"); assert(strstr(output,"Reflects the next positive-damage event back at its attack"));
  command(&ch,"greater hostile juxtaposition"); assert(strstr(output,"Stores three intended reflection charges and removes the l"));
  command(&ch,"banishing blade"); assert(strstr(output,"Attempts a reactive knockdown against an attacker, limited"));
  command(&ch,"planar soul"); assert(strstr(output,"Grants +2 morale initiative and Perception, plus alignment"));
  command(&ch,"repulsion"); assert(strstr(output,"Unregistered spell ID with a retained repulsion-field handler"));
  command(&ch,"geniekind"); assert(strstr(output,"Choose djinni, efreeti, marid, or shaitan as an extra argu"));
  command(&ch,"grand destiny"); assert(strstr(output,"Stores potential that the granddestiny command consumes fo"));
  command(&ch,"touch of fatigue"); assert(strstr(output,"Applies Fatigued, invoking shared penalties of -2 to attac"));
  command(&ch,"lesser rejuvenate eidolon"); assert(strstr(output,"Heals the caster's eidolon in the room for 2d5 + 5 + min(1"));
  command(&ch,"rejuvenate eidolon"); assert(strstr(output,"Heals the caster's eidolon in the room for 6d5 + 15 + min("));
  command(&ch,"greater rejuvenate eidolon"); assert(strstr(output,"Heals the caster's eidolon in the room for 10d5 + 25 + min"));
  command(&ch,"lesser evolution surge"); assert(strstr(output,"Grants the caster's eidolon +2 eidolon bonuses to Strength"));
  command(&ch,"evolution surge"); assert(strstr(output,"Grants the caster's eidolon +4 eidolon bonuses to Strength"));
  command(&ch,"greater evolution surge"); assert(strstr(output,"Grants the caster's eidolon +6 eidolon bonuses to Strength"));
  command(&ch,"lesser restore eidolon"); assert(strstr(output,"Accepts only the caster's eidolon in the room, but current"));
  command(&ch,"restore eidolon"); assert(strstr(output,"Accepts only the caster's eidolon in the room, but current"));
  command(&ch,"purified calling"); assert(strstr(output,"Heals the caster's eidolon in the room for 10 times caster"));
  spell_info[SPELL_AQUEOUS_ORB].min_level[CLASS_WIZARD]=5;
  spell_info[SPELL_CAUSTIC_BLOOD].min_level[CLASS_WIZARD]=7;
  CONFIG_ARCANE_DAMAGE=50;
  command(&ch,"aqueous orb"); assert(!strstr(output,"Global damage bonus:"));
  command(&ch,"caustic blood"); assert(!strstr(output,"Global damage bonus:"));
  command(&ch,"geniekind"); assert(!strstr(output,"Summon scaling:"));
  command(&ch,"gird allies"); assert(strstr(output,"Armor class limit:"));
  command(&ch,"mass daze monster"); assert(strstr(output,"Recipients:") && strstr(output,"Group members in the room"));
  command(&ch,"communal stone skin"); assert(strstr(output,"Group members in the room"));
  CONFIG_ARCANE_DAMAGE=0;

  /* Batch 501-550: exact names, disabled entries, and actual effects. */
  command(&ch,"recall to palanthas"); assert(strstr(output,"Retained recall handler transports the selected pl"));
  command(&ch,"recall to sanction"); assert(strstr(output,"Retained recall handler transports the selected pl"));
  command(&ch,"recall to solace"); assert(strstr(output,"Retained recall handler transports the selected pl"));
  command(&ch,"minor rapid buff"); assert(strstr(output,"Retained effect accelerates the automated buff lis"));
  command(&ch,"rapid buff"); assert(strstr(output,"Retained effect accelerates the automated buff lis"));
  command(&ch,"greater rapid buff"); assert(strstr(output,"Retained effect accelerates the automated buff lis"));
  command(&ch,"power word silence"); assert(strstr(output,"Silences the target, preventing ordinary speech an"));
  command(&ch,"holy aura"); assert(strstr(output,"Grants group members +4 deflection AC and +4 sacre"));
  command(&ch,"flame arrow"); assert(strstr(output,"Fires (caster level - 2) / 3 fire-damage attacks a"));
  command(&ch,"mass identify"); assert(strstr(output,"Displays the selected character's visible worn equ"));
  command(&ch,"fire bolt"); assert(strstr(output,"Deals fire damage to the selected target."));
  command(&ch,"jolt"); assert(strstr(output,"Deals electricity damage to the selected target."));
  command(&ch,"disrupt undead"); assert(strstr(output,"Deals holy damage to an undead target; other targe"));
  command(&ch,"arcane mark"); assert(strstr(output,"Inscribes the caster's configured arcane mark on a"));
  command(&ch,"enhanced diplomacy"); assert(strstr(output,"Grants +2 insight to Persuasion and Intimidate."));
  command(&ch,"flare"); assert(strstr(output,"Dazzles the target, imposing the shared -1 circums"));
  command(&ch,"grasp"); assert(strstr(output,"Stores a climbing-support effect. The legendary tr"));
  command(&ch,"guidance"); assert(strstr(output,"Adds 1d4 to the next general skill_roll check, the"));
  command(&ch,"lullaby"); assert(strstr(output,"Applies -5 Perception, making the target less obse"));
  command(&ch,"root"); assert(strstr(output,"Grants +3 resistance to the implemented knockdown/"));
  command(&ch,"stabilize"); assert(strstr(output,"Heals 10 HP only when current HP is below one tent"));
  command(&ch,"summon instrument"); assert(strstr(output,"Creates a chosen musical instrument in the caster'"));
  command(&ch,"virtue"); assert(strstr(output,"Grants a morale maximum-HP bonus of 1d4 + caster l"));
  command(&ch,"splinter storm"); assert(strstr(output,"Deals slashing damage to eligible creatures in the"));
  command(&ch,"shockwave"); assert(strstr(output,"Deals bludgeoning damage to eligible creatures in "));
  command(&ch,"poison breath"); assert(strstr(output,"Normal casting currently has no area dispatch case"));
  command(&ch,"summon solar"); assert(strstr(output,"Summons a solar epic ally at min(35, max(30, caste"));
  command(&ch,"blink"); assert(strstr(output,"Blinks between material and ethereal states: norma"));
  command(&ch,"hurl through hell"); assert(strstr(output,"Immediately deals mental damage to the target; doe"));
  command(&ch,"planar anchor"); assert(strstr(output,"Grants the no-teleport state, blocking travel path"));
  command(&ch,"bane"); assert(strstr(output,"Imposes -1 morale attack and Will saves on the sel"));
  command(&ch,"disguise self"); assert(strstr(output,"Grants +10 competence to Disguise."));
  command(&ch,"alter self"); assert(strstr(output,"Changes the caster's active race to a selected pla"));
  command(&ch,"litany of damnation"); assert(strstr(output,"While a group member carries this litany, an attac"));
  command(&ch,"aurak energy ray"); assert(strstr(output,"Fires one racial ranged-touch energy ray at a non-"));
  command(&ch,"aurak breath"); assert(strstr(output,"Exhales fumes at eligible creatures in the room. I"));
  command(&ch,"aurak death throes"); assert(strstr(output,"Internal Aurak death-explosion handler damages eli"));
  command(&ch,"protection from poison"); assert(strstr(output,"Grants +5 to supported poison saving throws."));
  command(&ch,"panacea"); assert(strstr(output,"Removes existing poison- or disease-flagged affect"));
  CONFIG_ARCANE_DAMAGE=50;
  spell_info[SPELL_FIRE_BOLT].min_level[CLASS_WIZARD]=1;
  command(&ch,"fire bolt"); assert(strstr(output,"Global damage bonus:") && strstr(output,"150% of normal damage"));
  CONFIG_ARCANE_DAMAGE=0;
  command(&ch,"fire bolt"); assert(!strstr(output,"Global damage bonus:"));
  command(&ch,"holy aura"); assert(strstr(output,"Armor class limit:") && strstr(output,"Group members in the room"));
  command(&ch,"summon solar"); assert(!strstr(output,"Summon scaling:"));
  command(&ch,"poison breath"); assert(!strstr(output,"Global damage bonus:"));
  command(&ch,"aurak death throes"); assert(strstr(output,"internal creature ability"));
  command(&ch,"solid fog"); assert(strstr(output,"50% concealment"));

  /* Full production psionic metadata; every definition is read-only and reviewed. */
  load_psionic_registration();
  for(i=1499;i<=1605;i++) {
    struct spell_info_type psi_info_before=spell_info[i];
    struct psionic_power_data psi_power_before=psionic_powers[i];
    struct config_data psi_config_before=config_info;
    struct char_data psi_player_before=ch;
    struct player_special_data psi_special_before=ps;
    text=render_checked(i);
    if(i==1499 || i==1605) {
      assert(!text && !spellhelp_public(i));
    } else {
      assert(text && !strstr(text,"Detailed effects are not yet documented"));
      if(spellhelp_public(i)) {
        assert(strstr(text,"Power:") && strstr(text,"School:") && strstr(text,"Damage:"));
        if(psionic_powers[i].can_augment)
          assert(strstr(text,"Augmentation:") && !strstr(text,"Effect details are not yet documented"));
      } else {
        assert(strstr(text,"Power:") && strstr(text,"Unavailable:"));
      }
    }
    free(text);
    assert(!memcmp(&psi_info_before,&spell_info[i],sizeof(psi_info_before)));
    assert(!memcmp(&psi_power_before,&psionic_powers[i],sizeof(psi_power_before)));
    assert(!memcmp(&psi_config_before,&config_info,sizeof(psi_config_before)));
    assert(!memcmp(&psi_player_before,&ch,sizeof(ch)) && !memcmp(&psi_special_before,&ps,sizeof(ps)));
  }
  command(&ch,"broker"); assert(strstr(output,"Grants enhancement bonuses to Diplomacy and A"));
  command(&ch,"call to mind"); assert(strstr(output,"Grants an enhancement Lore bonus of 4 + addit"));
  command(&ch,"catfall"); assert(strstr(output,"Grants +2 enhancement Dexterity and the Safe "));
  command(&ch,"crystal shard"); assert(strstr(output,"Fires a piercing crystal shard at one target."));
  command(&ch,"deceleration"); assert(strstr(output,"Imposes -2 enhancement Dexterity on a target "));
  command(&ch,"demoralize"); assert(strstr(output,"Affects eligible enemies in the room with Sha"));
  command(&ch,"energy ray"); assert(strstr(output,"Fires a ray using the selected psionic energy"));
  command(&ch,"force screen"); assert(strstr(output,"Grants shield AC of 4 + additional PSP / 4, p"));
  command(&ch,"inertial armor"); assert(strstr(output,"Grants armor AC of 4 + additional PSP / 2, pl"));
  command(&ch,"fortify"); assert(strstr(output,"Grants resistance bonuses to all three saves "));
  command(&ch,"inevitable strike"); assert(strstr(output,"Grants min(25, 20 + 2 times additional PSP) i"));
  command(&ch,"mind thrust"); assert(strstr(output,"Assaults one mind with mental damage."));
  command(&ch,"defensive precognition"); assert(strstr(output,"Grants insight bonuses to AC and all three sa"));
  command(&ch,"offensive precognition"); assert(strstr(output,"Grants insight bonuses to attack rolls of 2 +"));
  command(&ch,"offensive prescience"); assert(strstr(output,"Grants insight bonuses to damage rolls of 2 +"));
  command(&ch,"slumber"); assert(strstr(output,"Puts a target of level 4 + additional PSP or "));
  command(&ch,"psionic vigor"); assert(strstr(output,"Grants 5 + 5 times (additional PSP / 3) circu"));
  command(&ch,"bestow power"); assert(strstr(output,"Restores 2 + 2 times (additional PSP / 3) PSP"));
  command(&ch,"biofeedback"); assert(strstr(output,"Grants damage reduction 2 + additional PSP / "));
  command(&ch,"body equilibrium"); assert(strstr(output,"Grants Water Walk and Levitate, allowing supp"));
  command(&ch,"breach"); assert(strstr(output,"Normal manifestation currently does nothing: "));
  command(&ch,"concealing amorpha"); assert(strstr(output,"Grants 20% concealment in the normal concealm"));
  command(&ch,"concussion blast"); assert(strstr(output,"At 4 or more additional PSP, deals bludgeonin"));
  command(&ch,"detect hostile intent"); assert(strstr(output,"Grants Danger Sense, enabling the supported d"));
  command(&ch,"elfsight"); assert(strstr(output,"Grants infravision and +2 enhancement Percept"));
  command(&ch,"specified energy adaptation"); assert(strstr(output,"Grants resistance to the selected psionic ene"));
  command(&ch,"energy push"); assert(strstr(output,"Deals selected-energy damage and separately a"));
  command(&ch,"energy stun"); assert(strstr(output,"Deals selected-energy damage and separately a"));
  command(&ch,"inflict pain"); assert(strstr(output,"Imposes an attack and general skill-check penalty"));
  command(&ch,"mental disruption"); assert(strstr(output,"Dazes eligible enemies in the room, restricti"));
  command(&ch,"psychic bodyguard"); assert(strstr(output,"Grants an insight Will bonus equal to max(0, "));
  command(&ch,"recall agony"); assert(strstr(output,"Deals temporal damage to one target."));
  command(&ch,"swarm of crystals"); assert(strstr(output,"Deals slashing crystal damage to eligible cre"));
  command(&ch,"thought shield"); assert(strstr(output,"On mind-affecting immunity checks, resists wh"));
  command(&ch,"body adjustment"); assert(strstr(output,"Heals (1 + additional PSP / 2)d12 HP, up to m"));
  command(&ch,"concussive onslaught"); assert(strstr(output,"Schedules psionic-level waves, beginning imme"));
  command(&ch,"endorphin surge"); assert(strstr(output,"Grants +2 morale Strength and Constitution, +"));
  command(&ch,"energy retort"); assert(strstr(output,"Retaliates against eligible non-ranged damage"));
  command(&ch,"eradicate invisibility"); assert(strstr(output,"Removes invisibility effects and flags from e"));
  command(&ch,"heightened vision"); assert(strstr(output,"Grants Ultravision for supported darkness vis"));
  command(&ch,"energy burst"); assert(strstr(output,"Deals selected-energy damage to eligible crea"));
  command(&ch,"mind trap"); assert(strstr(output,"Creates a trap for hostile Telepathy or recog"));
  command(&ch,"sharpened edge"); assert(strstr(output,"Expands critical threat ranges for eligible s"));
  command(&ch,"ubiquitous vision"); assert(strstr(output,"Provides all-around vision through the combat"));
  command(&ch,"death urge"); assert(strstr(output,"Compels the target to stop normal combat and "));
  command(&ch,"empathetic feedback"); assert(strstr(output,"Retaliates against eligible non-ranged attack"));
  command(&ch,"energy adaptation"); assert(strstr(output,"Grants acid, fire, cold, electric, and sonic "));
  command(&ch,"incite passion"); assert(strstr(output,"Applies -2 circumstance attack and AC and -4 "));
  command(&ch,"intellect fortress"); assert(strstr(output,"Grants group members in the room +50 percenta"));
  command(&ch,"moment of terror"); assert(strstr(output,"Knocks the target into a sitting position and"));
  command(&ch,"power leech"); assert(strstr(output,"Schedules one event per caster psionic level."));
  command(&ch,"slip the bonds"); assert(strstr(output,"Grants Free Movement and removes Web/entangli"));
  command(&ch,"wither"); assert(strstr(output,"Applies a circumstance Strength penalty of (2"));
  command(&ch,"wall of ectoplasm"); assert(strstr(output,"Creates a wall blocking movement through a sp"));
  command(&ch,"ectoplasmic shambler"); assert(strstr(output,"Summons a psionic follower. Base level equals"));
  command(&ch,"pierce the veils"); assert(strstr(output,"Grants True Sight, enabling supported true-fo"));
  command(&ch,"planar travel"); assert(strstr(output,"Retained creation handler opens a gate to a r"));
  command(&ch,"power resistance"); assert(strstr(output,"Grants a power-resistance modifier of 12 + ma"));
  command(&ch,"psychic crush"); assert(strstr(output,"Attempts a lethal mental crush. A failed Will"));
  command(&ch,"psychoport"); assert(strstr(output,"Transports the caster to the selected charact"));
  command(&ch,"shatter mind blank"); assert(strstr(output,"Removes Mind Blank effects/flags from eligibl"));
  command(&ch,"shrapnel burst"); assert(strstr(output,"Deals piercing damage to eligible creatures i"));
  command(&ch,"tower of iron will"); assert(strstr(output,"Grants group members a power-resistance modif"));
  command(&ch,"upheaval"); assert(strstr(output,"Deals earth damage to eligible creatures in t"));
  command(&ch,"breath of the black dragon"); assert(strstr(output,"Deals acid damage to eligible creatures in th"));
  command(&ch,"brutalize wounds"); assert(strstr(output,"Changes supported weapon damage dice against "));
  command(&ch,"disintegration"); assert(strstr(output,"Fires a chaos-damage ray at one target."));
  command(&ch,"sustained flight"); assert(strstr(output,"Grants the normal Flying state."));
  command(&ch,"cosmic awareness"); assert(strstr(output,"Stores potential activated with cosmicawarene"));
  command(&ch,"energy conversion"); assert(strstr(output,"Grants the same five energy resistances as En"));
  command(&ch,"evade burst"); assert(strstr(output,"Grants Evasion, improving supported Reflex-fo"));
  command(&ch,"oak body"); assert(strstr(output,"Grants +5 natural armor, +4 enhancement Stren"));
  command(&ch,"body of iron"); assert(strstr(output,"Grants DR 15/adamantine with no finite reserv"));
  command(&ch,"psychosis"); assert(strstr(output,"Confuses the selected target; normal confusio"));
  command(&ch,"ultrablast"); assert(strstr(output,"Deals mental damage to eligible creatures in "));
  command(&ch,"greater psychoport"); assert(strstr(output,"Normal manifestation currently does nothing: "));
  command(&ch,"recall death"); assert(strstr(output,"Attempts lethal mental damage against one tar"));
  command(&ch,"shadow body"); assert(strstr(output,"Grants DR 10/magic, Ultravision, Water Breath"));
  command(&ch,"assimilate"); assert(strstr(output,"Deals chaos damage and separately marks the t"));
  command(&ch,"true metabolism"); assert(strstr(output,"Adds 10 to the normal HP-regeneration calcula"));
  command(&ch,"impale mind"); assert(strstr(output,"Deals epic mental damage. Its registered affe"));
  command(&ch,"razor storm"); assert(strstr(output,"Deals epic slashing damage to eligible creatu"));
  command(&ch,"psychokinetic thrashing"); assert(strstr(output,"Deals epic force damage to one target."));
  command(&ch,"epic psionic ward"); assert(strstr(output,"Grants insight AC of min(6, caster Intelligen"));
  command(&ch,"ectoplasmic sheen"); assert(strstr(output,"Unregistered power ID with no active manifest"));
  command(&ch,"mass missive"); assert(strstr(output,"Unregistered power ID with no active manifest"));
  command(&ch,"psionic lock"); assert(strstr(output,"Unregistered power ID with no active manifest"));
  command(&ch,"share pain"); assert(strstr(output,"Unregistered power ID with no active manifest"));
  command(&ch,"dispel psionics"); assert(strstr(output,"Unregistered power ID with no active manifest"));
  command(&ch,"forced shared pain"); assert(strstr(output,"Unregistered power ID with no active manifest"));
  command(&ch,"adapt body"); assert(strstr(output,"Unregistered power ID with no active manifest"));
  command(&ch,"remote view trap"); assert(strstr(output,"Unregistered power ID with no active manifest"));
  command(&ch,"barred mind"); assert(strstr(output,"Unregistered power ID with no active manifest"));
  command(&ch,"energy wave"); assert(strstr(output,"Unregistered power ID with no active manifest"));
  command(&ch,"greater psychoportation"); assert(strstr(output,"Unregistered power ID with no active manifest"));
  command(&ch,"apopsi"); assert(strstr(output,"Unregistered power ID with no active manifest"));
  command(&ch,"timeless body"); assert(strstr(output,"Unregistered power ID with no active manifest"));
  command(&ch,"barred mind personal"); assert(strstr(output,"Unregistered power ID with no active manifest"));
  command(&ch,"ectoplasmic goliath"); assert(strstr(output,"Unregistered power ID with no active manifest"));
  command(&ch,"absolute geas"); assert(strstr(output,"Internal perk proc: after eligible violent Te"));
  command(&ch,"hive commander mark"); assert(strstr(output,"Internal perk proc marks a target and gives t"));
  command(&ch,"gravity well"); assert(strstr(output,"The gravitywell perk command halves current m"));
  assert(spellhelp_public(PSIONIC_BREACH)); /* Registered but its handler is incomplete. */
  assert(!spellhelp_public(PSIONIC_ECTOPLASMIC_GOLIATH));
  assert(!spellhelp_public(PSIONIC_GRAVITY_WELL));
  assert(psionic_powers[PSIONIC_EMPATHIC_FEEDBACK].augment_amount==100);
  assert(psionic_powers[PSIONIC_EMPATHIC_FEEDBACK].max_augment==1);
  assert(psionic_powers[PSIONIC_IMPALE_MIND].is_epic);
  command(&ch,"deadly fear"); assert(strstr(output,"Fortitude") && strstr(output,"8 additional PSP"));
  command(&ch,"mental barrier"); assert(strstr(output,"deflection") && strstr(output,"each 4 increases the AC bonus"));
  command(&ch,"psionic blast"); assert(strstr(output,"stun") && strstr(output,"Each 2 additional PSP"));
  CONFIG_PSIONIC_DAMAGE=25;
  command(&ch,"mind thrust"); assert(strstr(output,"Psionic +25% (125% of normal damage)"));
  command(&ch,"energy retort"); assert(!strstr(output,"Global damage bonus:"));
  command(&ch,"concussive onslaught"); assert(!strstr(output,"Global damage bonus:"));
  CONFIG_PSIONIC_DAMAGE=0;
  command(&ch,"mind thrust"); assert(!strstr(output,"Global damage bonus:"));
  command(&ch,"force screen"); assert(strstr(output,"Armor class limit:"));
  command(&ch,"ectoplasmic shambler"); assert(!strstr(output,"Summon scaling:"));
  free_psionic_registration();
  puts("spellhelp: all registered psionic powers, retired definitions, perk IDs, augmentation, damage settings, and read-only state passed.");
  free_batch_registration();
  puts("spellhelp defined spell IDs through 540 (541-550 outside ordinary spell list): public, legacy, internal, unfinished and campaign-dependent entries, implemented outcomes, colors, alignment, and read-only rendering passed.");
  puts("spellhelp: lookup, ambiguity, public filtering, metadata, detailed entries, coverage, and read-only behavior passed.");
  return 0;
}
