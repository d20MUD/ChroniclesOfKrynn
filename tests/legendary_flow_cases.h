/* Captures the real smith UI; the owner accepted this walkthrough for usability. */
static FILE *flow_capture;
static void flow_command(struct char_data *ch,const char *text)
{
  command(ch,text);
  if(flow_capture)fprintf(flow_capture,"> infuse %s\n%s\n",text,output);
}
static void flow_purchase(struct char_data *ch,const char *text)
{
  flow_command(ch,text);assert(strstr(output,"infuse confirm "));confirm(ch);
  if(flow_capture)fprintf(flow_capture,"> infuse confirm <quote-id>\n%s\n",output);
}
static void flow_cases(struct char_data *ch)
{
  const char *path=getenv("LEGENDARY_FLOW_CAPTURE");struct obj_data *o;
  technique_clean(ch);foundation_reset(ch);legendary_effects_test_time+=1000;CLASS_LEVEL(ch,CLASS_WARRIOR)=30;
  assert(estate_query("UPDATE legendary_characters SET infusion_xp=20000000,fragments=20000,catalysts=200,draconic=2000,martial=2000,arcane=2000,spiritual=2000,primal=2000,shadow=2000,version=version+1 WHERE player_id=1"));
  GET_GOLD(ch)=500000;assert(save_char_checked(ch,0));
  o=item("flowblade");o->item_number=0;GET_ID(o)=890001;GET_OBJ_LEVEL(o)=25;GET_OBJ_TYPE(o)=ITEM_WEAPON;
  SET_BIT_AR(GET_OBJ_WEAR(o),ITEM_WEAR_WIELD);obj_to_char(o,ch);
  if(path){flow_capture=fopen(path,"w");assert(flow_capture);}
  flow_command(ch,"");assert(strstr(output,"infuse next <item>")&&!strstr(output,"infuse reshape"));
  flow_command(ch,"next flowblade");assert(strstr(output,"2000000 infusion XP")&&strstr(output,"50000 gold")&&strstr(output,"permanent"));
  flow_command(ch,"paths flowblade");assert(strstr(output,"Slayer"));
  flow_purchase(ch,"awaken flowblade Slayer");
  flow_command(ch,"next flowblade");assert(strstr(output,"free foundation")&&strstr(output,"12 points"));
  flow_purchase(ch,"select flowblade primary foundation F02");
  flow_command(ch,"next flowblade");assert(strstr(output,"free additional item passive"));
  flow_purchase(ch,"select flowblade primary passive P12");
  assert(saved(o).channels[0].item_passive==12);
  flow_command(ch,"next flowblade");assert(strstr(output,"5000000 infusion XP")&&strstr(output,"100000 gold")&&strstr(output,"Only this channel advances"));
  flow_purchase(ch,"advance flowblade primary");
  flow_command(ch,"next flowblade");assert(strstr(output,"free technique"));
  flow_purchase(ch,"select flowblade primary technique T01");
  flow_command(ch,"next flowblade");assert(strstr(output,"infuse attune")&&strstr(output,"three powers"));
  flow_purchase(ch,"attune flowblade primary technique");
  flow_command(ch,"next flowblade");assert(strstr(output,"infuse use T01")&&strstr(output,"Equip the item"));
  test_wear(ch,o,WEAR_WIELD_1);
  if(flow_capture)fprintf(flow_capture,"> wield flowblade\n[Fixture equips the ordinary weapon.]\n\n");
  flow_command(ch,"use T01");assert(strstr(output,"You activate Measured strike"));
  flow_command(ch,"inspect flowblade primary");assert(strstr(output,"cooldown=90s"));
  if(flow_capture){fclose(flow_capture);flow_capture=NULL;}
  puts("legendary Phase 14: first-use navigator, exact next costs, free choices, attunement and first real activation PASS");
}
