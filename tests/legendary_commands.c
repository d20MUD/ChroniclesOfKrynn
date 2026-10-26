#define main estate_baseline_main
#include "estate_integration.c"
#undef main
#include "legendary_commands.h"
#include "legendary_transaction.h"
#include "legendary_earnings.h"
#include "spells.h"
#include "dg_event.h"
#include "actions.h"
#include "legendary_techniques.h"
#undef assert
#define assert(t) do {if(!(t)){fprintf(stderr,"commands assertion %d: %s\n%s\n",__LINE__,#t,output);abort();}} while(0)
int MIN(int a,int b){return a<b?a:b;}
struct sivak_form *active_sivak_form(struct char_data *ch){return NULL;}
static int signature_roll=1,signature_save=0;
int d20(struct char_data *ch){return signature_roll;}
int compute_mag_saves(struct char_data *ch,int kind,int modifier){return signature_save+modifier;}
struct index_data *mob_index;
mob_rnum top_of_mobt;
const char *ability_names[NUM_ABILITIES]={[1]="acrobatics",[4]="perception"};
/* Ordinary visibility/target-policy adapter; the engine checks are production. */
struct config_data config_info;
bool has_blindsight(struct char_data *ch){return false;}
bool room_is_dark(room_rnum room){return false;}
bool char_has_ultra(struct char_data *ch){return false;}
bool char_has_infra(struct char_data *ch){return false;}
bool is_outdoors(struct char_data *ch){return false;}
bool has_true_sight(struct char_data *ch){return false;}
bool can_see_hidden(struct char_data *ch,struct char_data *vict){return !AFF_FLAGGED(vict,AFF_HIDE);}
int ok_damage_shopkeeper(struct char_data *ch,struct char_data *vict){return true;}
bool is_mission_mob(struct char_data *ch,struct char_data *vict){return true;}
struct char_data *get_char_vis(struct char_data *ch,char *name,int *number,int where)
{struct char_data *p;for(p=world[IN_ROOM(ch)].people;p;p=p->next_in_room)if(!strcasecmp(GET_NAME(p),name))return p;return NULL;}
void update_pos(struct char_data *ch){if(GET_HIT(ch)<=0)GET_POS(ch)=POS_DEAD;}
/* Action/world adapters; production catalog and effect code are linked below. */
static bool standard_available=true;
static int action_duration;
struct zone_data *zone_table;
zone_rnum top_of_zone_table;
bool is_action_available(struct char_data *ch,action_type kind,bool message){return standard_available;}
void start_action_cooldown(struct char_data *ch,action_type kind,int duration){assert(kind==atSTANDARD);action_duration=duration;}
void attacks_of_opportunity(struct char_data *ch,int penalty){}
void char_from_furniture(struct char_data *ch){SITTING(ch)=NULL;}
int change_position(struct char_data *ch,int pos){GET_POS(ch)=pos;if(pos!=POS_SITTING)ch->legendary_knockdown=false;return 1;}
void new_affect(struct affected_type *af){memset(af,0,sizeof(*af));}
void affect_to_char(struct char_data *ch,struct affected_type *af)
{struct affected_type *p=malloc(sizeof(*p));*p=*af;p->next=ch->affected;ch->affected=p;}
bool affected_by_spell(struct char_data *ch,int spell)
{struct affected_type *p;for(p=ch->affected;p;p=p->next)if(p->spell==spell)return true;return false;}
void affect_remove(struct char_data *ch,struct affected_type *af)
{struct affected_type **at=&ch->affected;while(*at&&*at!=af)at=&(*at)->next;assert(*at);*at=af->next;free(af);affect_total(ch);}
int compute_damtype_reduction(struct char_data *ch,int type,struct char_data *attacker,int power){return GET_RESISTANCES(ch,type);}
int compute_damage_reduction(struct char_data *ch,int type){return 7;}
int compute_ability(struct char_data *ch,int skill){return GET_ABILITY(ch,skill);}
int House_can_enter(struct char_data *ch,room_vnum room){return true;}
int num_pc_in_room(struct room_data *room){return 0;}
bool is_natural_swimmer(struct char_data *ch){return false;}
int has_flight(struct char_data *ch){return AFF_FLAGGED(ch,AFF_FLYING);}
int has_scuba(struct char_data *ch,room_rnum room){return AFF_FLAGGED(ch,AFF_SCUBA);}
static void command(struct char_data *ch,const char *s)
{output[0]=0;legendary_infuse(ch,s);}
static void confirm(struct char_data *ch)
{
  char id[33],s[64],*at=strstr(output,"infuse confirm ");assert(at);assert(sscanf(at,"infuse confirm %32s",id)==1);
  snprintf(s,sizeof(s),"confirm %s",id);command(ch,s);assert(strstr(output,"confirmed and saved"));
}
static void perform(struct char_data *ch,const char *s){command(ch,s);confirm(ch);}
static struct legendary_item_record saved(struct obj_data *o)
{struct legendary_item_record r;assert(legendary_store_load(o->legendary_uuid,&r)==LEG_LOAD_OK);return r;}
#ifdef LEGENDARY_EQUIPMENT_TEST
#include "legendary_equipment_cases.h"
#ifdef LEGENDARY_FOUNDATIONS_TEST
#include "legendary_foundation_cases.h"
#include "legendary_item_passive_cases.h"
#endif
#endif
#if defined(LEGENDARY_EFFECTS_TEST) && !defined(LEGENDARY_TECHNIQUES_TEST)
#include "legendary_effect_cases.h"
#endif
#ifdef LEGENDARY_TECHNIQUES_TEST
#include "legendary_technique_cases.h"
#ifdef LEGENDARY_SIGNATURES_TEST
#include "legendary_signature_cases.h"
#ifdef LEGENDARY_RANKS_TEST
#include "legendary_rank_cases.h"
#ifdef LEGENDARY_FLOW_TEST
#include "legendary_flow_cases.h"
#endif
#ifdef LEGENDARY_SECURITY_TEST
#include "legendary_security_cases.h"
#endif
#endif
#endif
#endif
int main(int argc,char **argv)
{
  struct char_data ch,mob;struct descriptor_data d={0};struct obj_data *o,*other;
  struct legendary_item_record r;char id[33],s[80];long before;int i;
#ifdef LEGENDARY_SECURITY_TEST
  /* Simulate the new process image after a socket-preserving copyover. */
  if(argc==3&&!strcmp(argv[1],"--budget-copyover")) {
    conn=mysql_init(NULL);
    assert(mysql_real_connect(conn,NULL,"root",NULL,"estate_test",0,argv[2],0));
    world=calloc(2,sizeof(*world));assert(world);
    init_player(&ch,"copyoverledger",901);
    if(!legendary_budget_prime(&ch))return 73;
    legendary_budget_unload(&ch);mysql_close(conn);return 0;
  }
#endif
  assert(argc==2);conn=mysql_init(NULL);assert(mysql_real_connect(conn,NULL,"root",NULL,"estate_test",0,argv[1],0));
  assert(estate_init()&&legendary_schema_ready());
  assert(estate_query("CREATE TABLE player_save_objs(idnum INT PRIMARY KEY AUTO_INCREMENT,name VARCHAR(64),serialized_obj LONGTEXT,creation_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP)"));
  assert(estate_query("CREATE TABLE player_data(name VARCHAR(64) PRIMARY KEY,obj_save_header TEXT)"));
  assert(estate_query("INSERT INTO player_data(name) VALUES('owner')"));
  zone_table=calloc(1,sizeof(*zone_table));
  world=calloc(3,sizeof(*world));top_of_world=2;world[1].number=2200;world[2].number=6530;
  obj_index=calloc(1,sizeof(*obj_index));obj_index[0].vnum=500;top_of_objt=0;
  mob_index=calloc(1,sizeof(*mob_index));mob_index[0].vnum=600;top_of_mobt=0;
  init_player(&ch,"owner",1);GET_ID(&ch)=100;GET_LEVEL(&ch)=30;GET_HIT(&ch)=GET_MAX_HIT(&ch)=100;GET_POS(&ch)=POS_STANDING;ch.desc=&d;d.connected=CON_PLAYING;
  memset(&mob,0,sizeof(mob));SET_BIT_AR(MOB_FLAGS(&mob),MOB_ISNPC);GET_MOB_RNUM(&mob)=0;GET_ID(&mob)=101;GET_POS(&mob)=POS_STANDING;IN_ROOM(&mob)=NOWHERE;char_to_room(&mob,1);
  assert(estate_query("INSERT INTO legendary_characters(player_id,infusion_xp,fragments,catalysts,draconic,martial,arcane,spiritual,primal,shadow) VALUES(1,400000000,400000,2000,20000,20000,20000,20000,20000,20000)"));
  GET_GOLD(&ch)=20000000;assert(save_char_checked(&ch,0));
  o=item("blade");GET_ID(o)=200;o->item_number=0;GET_OBJ_LEVEL(o)=25;SET_BIT_AR(GET_OBJ_WEAR(o),ITEM_WEAR_WIELD);obj_to_char(o,&ch);
  command(&ch,"awaken blade Guardian");assert(strstr(output,"registered"));assert(!*o->legendary_uuid);
  assert(estate_query("INSERT INTO legendary_smiths VALUES(600,1,0)"));
  other=item("blade duplicate");GET_ID(other)=201;obj_to_char(other,&ch);
  command(&ch,"awaken blade Guardian");assert(strstr(output,"Ambiguous"));assert(!*o->legendary_uuid);extract_obj(other);
  command(&ch,"awaken blade Guardian");assert(strstr(output,"permanently"));assert(!*o->legendary_uuid);
  command(&ch,"cancel");assert(!*o->legendary_uuid);
  command(&ch,"awaken blade Guardian");assert(sscanf(strstr(output,"infuse confirm "),"infuse confirm %32s",id)==1);
  other=item("stone");GET_ID(other)=202;obj_to_char(other,&ch);snprintf(s,sizeof(s),"confirm %s",id);command(&ch,s);assert(strstr(output,"invalidated"));assert(!*o->legendary_uuid);extract_obj(other);
  perform(&ch,"awaken blade Guardian");r=saved(o);assert(r.channels[0].rank==1);assert(GET_OBJ_BOUND_ID(o)==1);assert(GET_GOLD(&ch)==19950000);
  legendary_infuse_cleanup(&ch);perform(&ch,"select blade primary foundation F01");r=saved(o);assert(r.channels[0].first_choices==1&&r.channels[0].foundation==1);assert(GET_GOLD(&ch)==19950000);
  perform(&ch,"select blade primary foundation F02");assert(GET_GOLD(&ch)==19930000);
  command(&ch,"advance blade");command(&ch,"mix 0 200 0 0 0 0");confirm(&ch);perform(&ch,"select blade primary technique T01");
  perform(&ch,"advance blade primary");perform(&ch,"select blade primary signature S09");
  perform(&ch,"advance blade");perform(&ch,"modify blade primary technique Ready");
  perform(&ch,"advance blade");r=saved(o);assert(r.channels[0].rank==5&&r.channels[0].modifier==2);
  perform(&ch,"reshape blade primary path Dragonbane");assert(saved(o).channels[0].path==1);
  perform(&ch,"prioritize blade primary 1");perform(&ch,"attune blade primary technique");perform(&ch,"attune blade primary signature");
  command(&ch,"loadout");assert(strstr(output,"Major 2"));legendary_infuse_cleanup(&ch);command(&ch,"loadout");assert(strstr(output,"Major 2"));
  perform(&ch,"unattune blade primary signature");
  command(&ch,"awaken blade supporting guard Guardian");assert(strstr(output,"actually wielded"));
  obj_from_char(o);GET_EQ(&ch,WEAR_WIELD_2H)=o;o->worn_by=&ch;o->worn_on=WEAR_WIELD_2H;
  perform(&ch,"awaken blade supporting guard Guardian");r=saved(o);assert(r.channels[1].rank==1&&r.channels[0].rank==5);
  perform(&ch,"advance blade supporting");perform(&ch,"select blade supporting technique T05");perform(&ch,"attune blade supporting technique");
  perform(&ch,"advance blade supporting");perform(&ch,"select blade supporting signature S01");
  perform(&ch,"attune blade supporting signature");command(&ch,"attune blade primary signature");assert(strstr(output,"Three major"));
  perform(&ch,"select blade primary technique T17");
  perform(&ch,"reshape blade primary path Wayfarer");r=saved(o);assert(!r.channels[0].technique&&!(r.channels[0].first_choices&2));
  before=GET_GOLD(&ch);perform(&ch,"select blade primary technique T01");assert(GET_GOLD(&ch)==before);
  perform(&ch,"select blade primary foundation F07 str");assert(saved(o).channels[0].subtype==0);
  perform(&ch,"select blade primary foundation F12 cold");assert(saved(o).channels[0].subtype==1);
  GET_ABILITY(&ch,1)=5;perform(&ch,"select blade primary foundation F08 1");assert(saved(o).channels[0].subtype==1);
  CLASS_LEVEL((&ch),CLASS_WIZARD)=1;
  command(&ch,"select blade primary foundation F11");assert(sscanf(strstr(output,"infuse confirm "),"infuse confirm %32s",id)==1);
  before=GET_GOLD(&ch);CLASS_LEVEL((&ch),CLASS_WIZARD)=0;snprintf(s,sizeof(s),"confirm %s",id);command(&ch,s);
  assert(strstr(output,"invalidated"));assert(GET_GOLD(&ch)==before&&saved(o).channels[0].foundation==8);
  before=scalar("SELECT arcane FROM legendary_characters WHERE player_id=1");perform(&ch,"exchange essence Martial Arcane 2");assert(scalar("SELECT arcane FROM legendary_characters WHERE player_id=1")==before+2);
  perform(&ch,"exchange catalyst 1");command(&ch,"exchange catalyst 1");assert(strstr(output,"unavailable"));
  command(&ch,"materials");assert(strstr(output,"UTC"));command(&ch,"choices blade primary");assert(strstr(output,"registered dragonlance required"));
  command(&ch,"advance blade supporting");assert(sscanf(strstr(output,"infuse confirm "),"infuse confirm %32s",id)==1);
  legendary_transaction_clock_offset=61;snprintf(s,sizeof(s),"confirm %s",id);command(&ch,s);assert(strstr(output,"stale"));legendary_transaction_clock_offset=0;assert(saved(o).channels[1].rank==3);
  command(&ch,"advance blade supporting");assert(sscanf(strstr(output,"infuse confirm "),"infuse confirm %32s",id)==1);
  char_from_room(&mob);char_to_room(&mob,2);snprintf(s,sizeof(s),"confirm %s",id);command(&ch,s);assert(strstr(output,"invalidated"));assert(saved(o).channels[1].rank==3);
  char_from_room(&mob);char_to_room(&mob,1);GET_POS(&ch)=POS_SLEEPING;command(&ch,"advance blade supporting");assert(strstr(output,"conscious"));GET_POS(&ch)=POS_STANDING;FIGHTING(&ch)=&mob;command(&ch,"advance blade supporting");assert(strstr(output,"out of combat"));FIGHTING(&ch)=NULL;
  assert(estate_query("UPDATE legendary_characters SET infusion_xp=0,version=version+1 WHERE player_id=1"));
  command(&ch,"advance blade supporting");assert(strstr(output,"Insufficient"));assert(saved(o).channels[1].rank==3);
  for(i=0;i<5;i++){const char *bad[]={"select blade primary foundation F07","select blade primary foundation F08 99","select blade primary foundation F11","select blade primary foundation F12 light","awaken 'unterminated"};command(&ch,bad[i]);assert(!strstr(output,"infuse confirm "));}
  #ifdef LEGENDARY_EQUIPMENT_TEST
  equipment_cases(&ch);
  #endif
  #ifdef LEGENDARY_FOUNDATIONS_TEST
  foundation_cases(&ch);
  item_passive_cases(&ch);
  #endif
  #if defined(LEGENDARY_EFFECTS_TEST) && !defined(LEGENDARY_TECHNIQUES_TEST)
  effect_cases(&ch,argv[1]);
  #endif
  #ifdef LEGENDARY_TECHNIQUES_TEST
  technique_cases(&ch);
  #ifdef LEGENDARY_SIGNATURES_TEST
  signature_cases(&ch);
  #ifdef LEGENDARY_RANKS_TEST
  rank_cases(&ch);
  #ifdef LEGENDARY_FLOW_TEST
  flow_cases(&ch);
  #endif
  #ifdef LEGENDARY_SECURITY_TEST
  security_cases(&ch,argv[1]);
  #endif
  #endif
  #endif
  #endif
  puts("legendary command integration: PASS");return 0;
}
