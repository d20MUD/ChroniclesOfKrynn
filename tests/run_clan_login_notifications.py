#!/usr/bin/env python3
"""Exercise clan login notices with production logic and player-file codecs."""
from pathlib import Path
import subprocess, tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'src/clan.c').read_text()
players=(root/'src/players.c').read_text()
def function(signature):
    a=source.index(signature+'\n{');return source[a:source.index('\n}',a)+2]+'\n'
headers='\n'.join('#include "'+h+'"' for h in ['conf.h','sysdep.h','structs.h','utils.h','db.h','comm.h','clan.h','interpreter.h','handler.h'])
code=headers+r'''
#include <assert.h>
#define QCYN "\tC"
#define QYEL "\tY"
#define QNRM "\tn"
#define C_NRM 0
#define CCNRM(ch,level) "\tn"
static struct clan_data clans[2];struct clan_data *clan_list=clans;int num_of_clans=2;
static struct player_index_element entries[6];struct player_index_element *player_table=entries;int top_of_p_table=5;
static struct char_data online;
struct char_data *character_list=&online;
static struct player_special_data online_saved;
struct player_special_data dummy_mob;
void basic_mud_log(const char *fmt,...) {}
static int writes, loads, member_notices, allow_priv=1, last_rank;
static int freed, approved_rank, approved_result, approved_pos;
static char last_notice[512];
static char member_text[512];
static char output[8192],record[4096];
size_t strlcpy(char *d,const char *s,size_t n) {size_t len=strlen(s);if(n)snprintf(d,n,"%s",s);return len;}
size_t send_to_char(struct char_data *ch,const char *fmt,...) {
  va_list ap;va_start(ap,fmt);int n=vsnprintf(output+strlen(output),sizeof(output)-strlen(output),fmt,ap);va_end(ap);return n;
}
void save_char(struct char_data *ch,int mode) {
  writes++;last_rank=GET_CLANRANK(ch);
  if(ch->player.name && !strcmp(ch->player.name,"Offline") && GET_CLANRANK(ch)>0) {
    approved_rank=GET_CLANRANK(ch);approved_result=ch->player_specials->saved.clan_application_result;
    approved_pos=GET_PFILEPOS(ch);
  }
  if(ch->player_specials->saved.clan_login_notices)
    strlcpy(last_notice,ch->player_specials->saved.clan_login_notices->text,sizeof(last_notice));
  if(ch->player.name && !strcmp(ch->player.name,"Member") && ch->player_specials->saved.clan_login_notices) {
    member_notices++;strlcpy(member_text,ch->player_specials->saved.clan_login_notices->text,sizeof(member_text));
  }
}
clan_rnum real_clan(clan_vnum id) {for(int i=0;i<2;i++)if(clans[i].vnum==id)return i;return NO_CLAN;}
struct char_data *is_playing(char *name) {return !strcmp(name,"Online")?&online:NULL;}
struct char_data *new_char(void) {struct char_data *ch=calloc(1,sizeof(*ch));ch->player_specials=calloc(1,sizeof(*ch->player_specials));return ch;}
int load_char(const char *name,struct char_data *ch) {
  loads++;ch->player.name=(char*)name;
  if(!strcmp(name,"Missing"))return -1;
  GET_CLAN(ch)=!strcmp(name,"Other")?20:10;
  GET_CLANRANK(ch)=!strcmp(name,"Member")?2:0;
  return 7;
}
#undef PERS
#define PERS(ch,viewer) GET_NAME(ch)
#undef ACMD
#define ACMD(name) void name(struct char_data *ch,const char *argument,int cmd,int subcmd)
const char *one_argument(const char *arg,char *out,size_t n) {
  size_t i=0;while(isspace((unsigned char)*arg))arg++;
  while(*arg&&!isspace((unsigned char)*arg)){if(i+1<n)out[i++]=*arg;arg++;}out[i]=0;return arg;
}
bool check_clanpriv(struct char_data *ch,int priv) {return allow_priv;}
void save_player_index(void) {}
void log_clan_activity(clan_vnum clan,const char *format,...) {}
void free_char(struct char_data *ch) {
  freed++;
  struct txt_block *n;
  while((n=ch->player_specials->saved.clan_login_notices)) {
    ch->player_specials->saved.clan_login_notices=n->next;free(n->text);free(n);
  }
  free(ch->player_specials);free(ch);
}
'''
code+=r'''
long get_ptable_by_name(const char *name) {
  for(int i=0;i<=top_of_p_table;i++)if(!strcasecmp(name,entries[i].name))return i;
  return -1;
}
int is_number(const char *s) {if(!*s)return 0;for(;*s;s++)if(!isdigit((unsigned char)*s))return 0;return 1;}
struct char_data *get_player_vis(struct char_data *ch,char *name,int *number,int where) {return !strcmp(name,"Online")?&online:NULL;}
int count_clan_members(clan_rnum c) {return 1;}
void mark_clan_modified(clan_rnum c) {}
void save_single_clan(clan_rnum c) {}
bool set_clan(struct char_data *ch,clan_vnum id) {GET_CLAN(ch)=id;save_char(ch,0);return TRUE;}
'''
for signature in ['static struct char_data *clan_online_player(const char *name)', 'void clan_restore_login_notice(struct char_data *ch, const char *text)', 'static void clan_queue_notice(struct char_data *ch, const char *color, const char *format, ...)', 'static void clan_notify_members(clan_rnum clan, const char *color, const char *format, ...)', 'static void clan_queue_application_result(struct char_data *ch, clan_rnum clan, bool approved)', 'static int clan_pending_applications(clan_rnum clan)', 'void clan_login_notifications(struct char_data *ch)']:
    code+=function(signature)
code+=function('ACMD(do_clanenrol)')
code+=function('ACMD(do_clanpromote)')
code+=function('ACMD(do_clandemote)')
a=players.index('  if (ch->player_specials->saved.clan_application_result)\n    BUFFER_WRITE');b=players.index('  if (GET_CLANRANK(ch)',a)
code+='\nstatic void save_notice(struct char_data *ch) {\n#define BUFFER_WRITE(...) snprintf(record,sizeof(record),__VA_ARGS__)\n'+players[a:b]+'\n#undef BUFFER_WRITE\n}\n'
a=players.index('        else if (!strcmp(tag, "CApp"))');a=players.index('{',a)+1;b=players.index('        else if (!strcmp(tag, "Clrk"))',a);part=players[a:b];part=part[:part.rfind('}')]
code+='\nstatic void load_notice(struct char_data *ch,const char *line) {\n'+part+'\n}\n'
a=players.index('  {\n    struct txt_block *notice;\n    for (notice = ch->player_specials->saved.clan_login_notices;')
b=players.index('  if (ch->player_specials->saved.clan_application_result)',a)
code+='\nstatic void save_notices(struct char_data *ch) {\n#define BUFFER_WRITE(...) snprintf(record+strlen(record),sizeof(record)-strlen(record),__VA_ARGS__)\n'+players[a:b]+'\n#undef BUFFER_WRITE\n}\n'
code+=r'''
int main(void) {
  struct char_data ch={0},reloaded={0};struct player_special_data saved={0},fresh={0};struct descriptor_data desc={0};
  ch.player_specials=&saved;ch.desc=&desc;GET_LEVEL(&ch)=10;GET_CLAN(&ch)=10;GET_CLANRANK(&ch)=2;
  clans[0].vnum=10;clans[0].clan_name="Silver Swords";clans[1].vnum=20;clans[1].clan_name="Red Robes";
  const char *names[]={"Online","Offline","Member","Other","Deleted","Missing"};
  for(int i=0;i<6;i++){entries[i].name=(char*)names[i];entries[i].clan=i==3?20:10;}
  entries[4].flags=PINDEX_DELETED;
  online.player.name="Online";online.player_specials=&online_saved;GET_CLAN(&online)=10;GET_CLANRANK(&online)=0;
  assert(clan_pending_applications(0)==2 && clan_pending_applications(1)==1);
  clan_login_notifications(&ch);assert(strstr(output,"Silver Swords has 2 applications")&&!strstr(output,"Red Robes"));
  assert(strstr(output,"\tY[CLAN APPLICATIONS]")&&strstr(output,"clan who 10") && strstr(output,"clan enrol <player>") && strstr(output,"clan expel <player>"));
  *output=0;GET_CLANRANK(&ch)=0;clan_login_notifications(&ch);assert(!*output); /* applicants aren't members */
  GET_LEVEL(&ch)=LVL_IMMORT;clan_login_notifications(&ch);assert(strstr(output,"Silver Swords")&&strstr(output,"Red Robes"));
  GET_LEVEL(&ch)=10;GET_CLAN(&ch)=0;*output=0;clan_login_notifications(&ch);assert(!*output);
  clan_queue_application_result(&ch,0,TRUE);save_notice(&ch);assert(!strcmp(record,"CApp: 1 Silver Swords\n"));
  reloaded.player_specials=&fresh;reloaded.desc=&desc;GET_LEVEL(&reloaded)=10;load_notice(&reloaded,record+6);
  clan_login_notifications(&reloaded);assert(strstr(output,"\tG[CLAN APPLICATION]")&&strstr(output,"APPROVED"));
  assert(!fresh.saved.clan_application_result && writes==1);
  *output=0;clan_login_notifications(&reloaded);assert(!*output&&writes==1);
  clan_queue_application_result(&ch,1,FALSE);save_notice(&ch);load_notice(&reloaded,record+6);
  clan_login_notifications(&reloaded);assert(strstr(output,"\tR[CLAN APPLICATION]")&&strstr(output,"Red Robes has been REJECTED"));
  assert(writes==2);*output=0;clan_login_notifications(&reloaded);assert(!*output);
  load_notice(&reloaded,"9 Bogus");assert(!fresh.saved.clan_application_result);
  ch.desc=NULL;clan_queue_application_result(&ch,0,TRUE);clan_login_notifications(&ch);assert(saved.saved.clan_application_result==1);
  /* Several events persist and retain their order, then clear in one save. */
  ch.desc=&desc;saved.saved.clan_application_result=0;
  clan_queue_notice(&ch,"\tG","PROMOTED to Captain in Silver Swords.");
  clan_queue_notice(&ch,"\tY","DEMOTED to Recruit in Silver Swords.");
  clan_queue_notice(&ch,"\tR","EXPELLED from Silver Swords.");
  *record=0;save_notices(&ch);
  assert(strstr(record,"CNot: \tG[CLAN] PROMOTED") && strstr(record,"DEMOTED") && strstr(record,"EXPELLED"));
  char *rest=record,*line;
  while((line=strtok_r(rest,"\n",&rest)))clan_restore_login_notice(&reloaded,line+6);
  *output=0;int before=writes;clan_login_notifications(&reloaded);
  assert(strstr(output,"PROMOTED")<strstr(output,"DEMOTED") && strstr(output,"DEMOTED")<strstr(output,"EXPELLED"));
  assert(writes==before+1 && !fresh.saved.clan_login_notices);
  *output=0;clan_login_notifications(&reloaded);assert(!*output && writes==before+1);
  /* Clan-wide events include offline members, but exclude applicants/deleted/other clans. */
  before=writes;clan_notify_members(0,"\tY","War has ended.");
  assert(writes==before+1 && member_notices==1 && strstr(member_text,"War has ended"));
  assert(!online_saved.saved.clan_login_notices);
  clan_notify_members(NO_CLAN,"\tY","Invalid");assert(member_notices==1);
  while(saved.saved.clan_login_notices) {
    struct txt_block *n=saved.saved.clan_login_notices;saved.saved.clan_login_notices=n->next;free(n->text);free(n);
  }
  /* Exercise both production command branches, with permission/no-op checks. */
  ch.player.name="Staff";GET_LEVEL(&ch)=LVL_STAFF;clans[0].ranks=6;
  clans[0].rank_name[0]="Leader";clans[0].rank_name[1]="Officer";clans[0].rank_name[2]="Member";
  online.desc=&desc;GET_CLANRANK(&online)=2;do_clanpromote(&ch,"Online",0,0);
  assert(GET_CLANRANK(&online)==1 && strstr(last_notice,"PROMOTED to Leader"));
  *output=0;clan_login_notifications(&online);assert(!online_saved.saved.clan_login_notices);
  do_clandemote(&ch,"Online",0,0);
  assert(GET_CLANRANK(&online)==2 && strstr(last_notice,"DEMOTED to Officer"));
  *output=0;clan_login_notifications(&online);
  do_clanpromote(&ch,"Member",0,0);assert(last_rank==1 && strstr(last_notice,"PROMOTED to Leader"));
  do_clandemote(&ch,"Member",0,0);assert(last_rank==3 && strstr(last_notice,"DEMOTED to Member"));
  before=writes;allow_priv=0;do_clanpromote(&ch,"Online",0,0);
  assert(writes==before && GET_CLANRANK(&online)==2 && !online_saved.saved.clan_login_notices);
  allow_priv=1;GET_CLANRANK(&online)=1;do_clanpromote(&ch,"Online",0,0);assert(writes==before);
  GET_CLANRANK(&online)=6;do_clandemote(&ch,"Online",0,0);assert(writes==before);
  /* Greater staff can approve without belonging to the target clan. */
  GET_LEVEL(&ch)=LVL_GRSTAFF;GET_CLAN(&ch)=0;allow_priv=0;
  GET_CLANRANK(&online)=0;*output=0;
  do_clanenrol(&ch,"",0,0);assert(strstr(output,"clan who <clan ID>"));
  do_clanenrol(&ch,"Online",0,0);
  assert(GET_CLANRANK(&online)==6 && online_saved.saved.clan_application_result==1);
  *output=0;clan_login_notifications(&online);
  GET_CLANRANK(&online)=0;GET_LEVEL(&ch)=LVL_STAFF;*output=0;
  do_clanenrol(&ch,"Online",0,0);assert(GET_CLANRANK(&online)==0); /* no cross-clan authority */
  GET_CLAN(&ch)=10;GET_CLANRANK(&ch)=2;*output=0;
  do_clanenrol(&ch,"Online",0,0);assert(GET_CLANRANK(&online)==0); /* clan permission denied */
  allow_priv=1;do_clanenrol(&ch,"Online",0,0);assert(GET_CLANRANK(&online)==6);
  *output=0;clan_login_notifications(&online);
  /* Offline approval saves both rank and queued decision before releasing
   * the loaded body. Validation failures must release it without saving. */
  GET_LEVEL(&ch)=LVL_GRSTAFF;GET_CLAN(&ch)=0;
  clans[0].rank_name[5]="Recruit";
  before=freed;*output=0;do_clanenrol(&ch,"Offline",0,0);
  assert(approved_rank==6 && approved_result==1 && approved_pos==7);
  assert(freed>before && strstr(output,"Offline is now a member"));
  before=writes;int old_freed=freed;*output=0;
  do_clanenrol(&ch,"Member",0,0);assert(writes==before && freed==old_freed+1 && strstr(output,"already enrolled"));
  old_freed=freed;*output=0;do_clanenrol(&ch,"Missing",0,0);
  assert(writes==before && freed==old_freed+1 && strstr(output,"Unable to load"));
  *output=0;do_clanenrol(&ch,"Deleted",0,0);assert(writes==before && strstr(output,"No such player"));
  *output=0;do_clanenrol(&ch,"Nobody",0,0);assert(writes==before && strstr(output,"No such player"));
  clans[0].max_members=1;old_freed=freed;*output=0;
  do_clanenrol(&ch,"Offline",0,0);assert(writes==before && freed==old_freed+1 && strstr(output,"maximum member limit"));
  clans[0].max_members=0;GET_LEVEL(&ch)=10;GET_CLAN(&ch)=20;GET_CLANRANK(&ch)=1;
  old_freed=freed;*output=0;do_clanenrol(&ch,"Offline",0,0);
  assert(writes==before && freed==old_freed+1 && strstr(output,"YOUR clan"));
  GET_CLAN(&ch)=10;*output=0;do_clanenrol(&ch,"Offline",0,0);
  assert(writes>before && approved_rank==6);
  puts("Clan login notices: online/offline applicants, member/staff scope, deleted players, colors, application decisions, ordered event persistence, personal/group scope, actual online/offline promotions/demotions, permissions and one-time delivery PASS");
}
'''
assert 'clan_login_notifications(d->character);' in (root/'src/interpreter.c').read_text()
assert source.count('clan_queue_application_result(v, c_n, FALSE);')==2
assert 'clan_queue_application_result(v, c_n, TRUE);' in source
with tempfile.TemporaryDirectory(prefix='clan-login-test-') as tmp:
    c=Path(tmp)/'check.c';c.write_text(code);exe=Path(tmp)/'check'
    subprocess.run(['cc','-std=gnu99','-Wall','-Wextra','-Wno-unused-parameter','-Wno-sign-compare','-I'+str(root/'src'),str(c),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
