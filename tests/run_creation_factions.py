#!/usr/bin/env python3
"""Exercise creation affiliation and location handlers, including sparse clan IDs."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'src/roleplay.c').read_text()
code=r'''
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
typedef int clan_rnum;
#define NO_CLAN -1
#define CAMPAIGN_DL
#define CON_CHAR_RP_MENU 62
#define CON_CHARACTER_DEITY_SELECT 78
#define CON_CHARACTER_DEITY_CONFIRM 79
#define NUM_DEITIES 3
#define DEITY_PANTHEON_NONE 0
#define STATE(d) ((d)->state)
#define GET_CLAN(ch) ((ch)->clan)
#define GET_HOMETOWN(ch) ((ch)->hometown)
#define GET_DEITY(ch) ((ch)->deity)
#define GET_CLANRANK(ch) ((ch)->rank)
#define GET_NAME(ch) "Newname"
#define UPPER(c) toupper((unsigned char)(c))
#define CLAN_NAME(i) clan_list[i].clan_name
struct char_data {int clan,rank,hometown,deity;};
struct descriptor_data {struct char_data *character;int state;};
static int num_of_clans=2;
static struct {int vnum;const char *clan_name,*description,*rank_name[6];} clan_list[]={
 {2,"Whitestone","First description",{"1","2","3","4","5","Member"}},
 {17,"Dragonarmies","Second description",{"1","2","3","4","5","Member"}}};
static struct {int clan;} player_table[1];
static char output[16000];
static void send_to_char(struct char_data *ch,const char *format,...) {va_list args;va_start(args,format);vsnprintf(output+strlen(output),sizeof(output)-strlen(output),format,args);va_end(args);}
static int real_clan(int vnum){for(int i=0;i<num_of_clans;i++)if(clan_list[i].vnum==vnum)return i;return NO_CLAN;}
static int get_ptable_by_name(const char *name){return 0;}
static bool set_clan(struct char_data *ch,int vnum){ch->clan=vnum;return true;}
static void show_character_rp_menu(struct descriptor_data *d){}
static const char *cities[]={"None","Palanthas","Sanction","Solace"};
static struct {const char *name;int pantheon;} deity_list[]={{"None",0},{"Paladine",1},{"Takhisis",2}};
static void display_deity_info(struct descriptor_data *d){}
static void display_deity_menu(struct descriptor_data *d){}

'''
for signature in ['void display_faction_menu(', 'void HandleStateCharacterFactionParseMenuChoice(', 'void HandleStateCharacterHometownParseMenuChoice(', 'void HandleStateCharacterDeityParseMenuChoice(', 'void HandleStateCharacterDeityConfirmParseMenuChoice(']:
    start=source.index(signature);code+=source[start:source.index('\n}',start)+2]+'\n'
code+=r'''
int main(void){
 struct char_data ch={0};struct descriptor_data d={&ch,76};
 display_faction_menu(&d);assert(strstr(output,"17)")&&strstr(output,"C) Help on Dragonarmies"));
 output[0]=0;char help[]="C";HandleStateCharacterFactionParseMenuChoice(&d,help);assert(strstr(output,"Second description")&&d.state==76);
 output[0]=0;char invalid[]="1";HandleStateCharacterFactionParseMenuChoice(&d,invalid);assert(!ch.clan&&strstr(output,"invalid"));
 char choose[]="17";HandleStateCharacterFactionParseMenuChoice(&d,choose);assert(ch.clan==17&&d.state==62);
 d.state=76;char none[]="0";HandleStateCharacterFactionParseMenuChoice(&d,none);assert(!ch.clan&&!ch.rank&&d.state==62);
 d.state=77;char city[]="3";HandleStateCharacterHometownParseMenuChoice(&d,city);assert(ch.hometown==3&&d.state==62);
 d.state=77;char badcity[]="4";HandleStateCharacterHometownParseMenuChoice(&d,badcity);assert(ch.hometown==3&&d.state==77);
 d.state=78;char hidden[]="0";HandleStateCharacterDeityParseMenuChoice(&d,hidden);assert(ch.deity==0&&d.state==78);
 char god[]="2";HandleStateCharacterDeityParseMenuChoice(&d,god);assert(ch.deity==2&&d.state==79);
 char no[]="n";HandleStateCharacterDeityConfirmParseMenuChoice(&d,no);assert(ch.deity==0&&d.state==78);
 HandleStateCharacterDeityParseMenuChoice(&d,god);char yes[]="y";HandleStateCharacterDeityConfirmParseMenuChoice(&d,yes);assert(ch.deity==2&&d.state==62);
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='creation-factions-') as directory:
    work=Path(directory);(work/'test.c').write_text(code)
    subprocess.run(['cc','-std=gnu99','-fsanitize=address,undefined','-fno-pie','-no-pie',str(work/'test.c'),'-o',str(work/'test')],check=True)
    subprocess.run([str(work/'test')],check=True)
print('Creation faction, hometown and deity handlers passed, including sparse IDs, hidden choices and confirmations.')
