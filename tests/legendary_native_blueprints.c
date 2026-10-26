#define __wrap_main native_capture_unused
#include "legendary_native_capture.c"
#undef __wrap_main
#include "class.h"
#include "race.h"
#include "feats.h"
#include "treasure.h"
#include "vnums.h"
#include "legendary_native_builds.h"
int __wrap_main(int argc,char **argv)
{
  FILE *out;int role,wear,count;struct char_data *ch;
  if(argc!=3)return 2;out=fopen(argv[2],"w");if(!out)return 3;
  logfile=stderr;CONFIG_CONFFILE=strdup("etc/config");load_config();auto_pwipe=0;no_rent_check=1;
  circle_srandom(150026);event_init();init_lookup_table();boot_db();
  for(role=0;role<7;role++){
    circle_srandom(150026+role);ch=build_role(role);
    if(!ch){fprintf(stderr,"BLUEPRINT: could not build role %d\n",role);return 4;}
    count=0;for(wear=0;wear<NUM_WEARS;wear++)if(GET_EQ(ch,wear)&&legendary_equipment_eligible(ch,GET_EQ(ch,wear),legendary_equipment_slot(GET_EQ(ch,wear)),0))count++;
    fprintf(out,"{\"role\":\"%s\",\"class\":%d,\"level\":%d,\"eligible_items\":%d,\"max_hp\":%d,\"max_resource\":%d,\"str\":%d,\"dex\":%d,\"con\":%d,\"int\":%d,\"wis\":%d,\"cha\":%d}\n",build_roles[role],GET_CLASS(ch),GET_LEVEL(ch),count,GET_MAX_HIT(ch),GET_MAX_PSP(ch),GET_STR(ch),GET_DEX(ch),GET_CON(ch),GET_INT(ch),GET_WIS(ch),GET_CHA(ch));fflush(out);
  }
  fclose(out);return 0;
}
