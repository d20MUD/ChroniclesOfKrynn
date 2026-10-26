/* Export authoritative costs/limits for offline balance analysis. */
#include "legendary.h"
#include <stdio.h>
#include <inttypes.h>
int main(void)
{
  int i;struct legendary_cost c;
  printf("{\"ranks\":[");
  for(i=1;i<=5;i++){
    legendary_cost_between(0,i,&c);
    printf("%s{\"rank\":%d,\"xp\":%"PRId64",\"gold\":%"PRId64",\"fragments\":%"PRId64",\"essences\":%"PRId64",\"catalysts\":%"PRId64"}",i==1?"":",",i,c.xp,c.gold,c.fragments,c.essences,c.catalysts);
  }
  printf("],\"daily_caps\":{\"fragments\":%d,\"essences\":%d,\"catalysts\":%d},\"foundations\":[",legendary_rules.daily_fragments,legendary_rules.daily_essences,legendary_rules.daily_catalysts);
  for(i=0;i<16;i++)printf("%s{\"id\":%d,\"points\":%d,\"value\":%d,\"ceiling\":%d}",i?",":"",legendary_foundations[i].id,legendary_foundations[i].points,legendary_foundations[i].value,legendary_foundations[i].ceiling);
  printf("],\"item_passives\":[");
  for(i=0;i<LEGENDARY_ITEM_PASSIVE_COUNT;i++){
    const struct legendary_item_passive_def *p=&legendary_item_passives[i];
    printf("%s{\"id\":%d,\"slots\":%u,\"ranks\":[%d,%d,%d,%d,%d],\"ceiling\":%d}",i?",":"",p->id,p->slots,
      legendary_item_passive_amount(p->id,1),legendary_item_passive_amount(p->id,2),legendary_item_passive_amount(p->id,3),legendary_item_passive_amount(p->id,4),legendary_item_passive_amount(p->id,5),p->ceiling);
  }
  puts("]}");return 0;
}
