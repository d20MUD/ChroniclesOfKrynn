#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "spec_abilities.h"
#include "clan_enchant.h"
#include "db.h"
#define E(id, rank, name, words) {WEAPON_SPECAB_##id, rank, name, words}
const struct clan_enchant_choice clan_enchant_choices[] = {
    E(ANARCHIC, 1, "Anarchic", "chaos havoc discord"),
    E(SPEED, 3, "Speed", "haste swift quicken"),
    E(AXIOMATIC, 1, "Axiomatic", "order decree balance"),
    E(BANE, 2, "Bane", "nemesis slayer scourge"),
    E(THUNDERING, 2, "Thundering", "thunder tempest rumble"),
    E(THROWING, 2, "Throwing", "hurl launch fling"),
    E(DANCING, 3, "Dancing", "dance waltz whirl"),
    E(UNHOLY, 1, "Unholy", "profane defile desecrate"),
    E(DEFENDING, 2, "Defending", "aegis bulwark ward"),
    E(VICIOUS, 3, "Vicious", "ferocity savage ravage"),
    E(DISRUPTION, 2, "Disruption", "exorcise banish requiem"),
    E(VORPAL, 5, "Vorpal", "decapitate sever snicker"),
    E(WOUNDING, 4, "Wounding", "lacerate bleed rend"),
    E(FLAMING, 1, "Flaming", "inferno blaze searing"),
    E(FLAMING_BURST, 3, "Flaming Burst", "eruption wildfire flare"),
    E(BLINDING, 4, "Blinding", "obscure dazzle eclipse"),
    E(FROST, 1, "Frost", "glacier rime freeze"),
    E(ADAPTIVE, 4, "Adaptive", "propel adapt flex"),
    E(GHOST_TOUCH, 1, "Ghost Touch", "specter phantom ethereal"),
    E(AGILE, 3, "Agile", "fleet grace nimble"),
    E(HOLY, 1, "Holy", "sanctify hallow radiance"),
    E(BEWILDERING, 2, "Bewildering", "bewilder daze perplex"),
    E(ICY_BURST, 3, "Icy Burst", "blizzard avalanche hailstorm"),
    E(CORROSIVE, 1, "Corrosive", "corrode dissolve caustic"),
    E(KEEN, 1, "Keen", "hone razor sharpen"),
    E(CORROSIVE_BURST, 3, "Corrosive Burst", "vitriol deluge acrid"),
    E(KI_FOCUS, 1, "Ki Focus", "focus center harmony"),
    E(EXHAUSTING, 2, "Exhausting", "weary fatigue languish"),
    E(VAMPIRIC, 4, "Vampiric", "drain sanguine thirst"),
    E(MIGHTY_CLEAVING, 2, "Mighty Cleaving", "cleave sunder hew"),
    E(LUCKY, 4, "Lucky", "fortune chance providence"),
    E(RETURNING, 2, "Returning", "return recall homeward"),
    E(INVIGORATING, 3, "Invigorating", "vigor renew revitalize"),
    E(SEEKING, 2, "Seeking", "snipe trueflight pursue"),
    E(SHOCK, 1, "Shock", "spark arc jolt"),
    E(SHOCKING_BURST, 3, "Shocking Burst", "storm surge fulminate")};
const size_t clan_enchant_count = sizeof(clan_enchant_choices) / sizeof(clan_enchant_choices[0]);
const struct clan_enchant_choice *clan_enchant_choice(int effect)
{
  size_t i;
  for (i = 0; i < clan_enchant_count; i++)
    if (clan_enchant_choices[i].effect == effect)
      return &clan_enchant_choices[i];
  return NULL;
}
struct obj_special_ability *clan_enchant_ability(struct obj_data *o)
{
  struct clan_weapon_enchantment *e = &o->clan_enchantment;
  if (!clan_enchant_choice(e->effect))
    return NULL;
  memset(&e->runtime, 0, sizeof(e->runtime));
  e->runtime.ability = e->effect;
  e->runtime.level = GET_OBJ_LEVEL(o);
  e->runtime.activation_method = special_ability_info[e->effect].activation_method;
  e->runtime.command_word = e->word;
  e->runtime.value[0] = e->bane_race;
  return &e->runtime;
}
static int effect_flag(int effect)
{
  switch (effect)
  {
  case WEAPON_SPECAB_FLAMING:
  case WEAPON_SPECAB_FLAMING_BURST:
    return ITEM_FLAMING;
  case WEAPON_SPECAB_FROST:
  case WEAPON_SPECAB_ICY_BURST:
    return ITEM_FROST;
  case WEAPON_SPECAB_SHOCK:
  case WEAPON_SPECAB_SHOCKING_BURST:
    return ITEM_SHOCK;
  case WEAPON_SPECAB_CORROSIVE:
  case WEAPON_SPECAB_CORROSIVE_BURST:
    return ITEM_CORROSIVE;
  case WEAPON_SPECAB_VICIOUS:
    return ITEM_VICIOUS;
  case WEAPON_SPECAB_VORPAL:
    return ITEM_VORPAL;
  case WEAPON_SPECAB_DISRUPTION:
    return ITEM_DISRUPTION;
  case WEAPON_SPECAB_SEEKING:
    return ITEM_SEEKING;
  case WEAPON_SPECAB_ADAPTIVE:
    return ITEM_ADAPTIVE;
  case WEAPON_SPECAB_AGILE:
    return ITEM_AGILE;
  case WEAPON_SPECAB_DEFENDING:
    return ITEM_DEFENDING;
  case WEAPON_SPECAB_KI_FOCUS:
    return ITEM_KI_FOCUS;
  default:
    return -1;
  }
}
void clan_enchant_activate(struct obj_data *o, bool active)
{
  int flag = effect_flag(o->clan_enchantment.effect);
  o->clan_enchantment.active = active;
  if (flag >= 0)
  {
    if (active)
      SET_OBJ_FLAG(o, flag);
    else
      REMOVE_OBJ_FLAG(o, flag);
  }
}
int clan_enchant_process(struct obj_data *o, struct char_data *ch, struct char_data *victim,
                         int method, const char *word)
{
  struct obj_special_ability *a;
  if (!o || !clan_enchant_choice(o->clan_enchantment.effect))
    return 0;
  if (method == ACTMTD_COMMAND_WORD)
  {
    if (!word || strcmp(word, o->clan_enchantment.word))
      return 0;
    clan_enchant_activate(o, !o->clan_enchantment.active);
    act(o->clan_enchantment.active ? "$p's clan enchantment awakens."
                                   : "$p's clan enchantment grows quiet.",
        FALSE, ch, o, NULL, TO_CHAR);
    if (!Crash_crashsave_checked(ch))
      send_to_char(ch, "The change could not be saved yet. Please save again.\r\n");
    return 1;
  }
  if (!o->clan_enchantment.active || !(a = clan_enchant_ability(o)))
    return 0;
  if ((a->activation_method & method) && special_ability_info[a->ability].special_ability_proc)
  {
    (*special_ability_info[a->ability].special_ability_proc)(a, o, ch, victim, method);
    return 1;
  }
  return 0;
}

bool clan_enchant_wielded(struct char_data *ch, int effect)
{
  int slots[] = {WEAR_WIELD_1, WEAR_WIELD_2H, WEAR_WIELD_OFFHAND};
  size_t i;
  for (i = 0; i < sizeof(slots) / sizeof(slots[0]); i++)
  {
    struct obj_data *o = GET_EQ(ch, slots[i]);
    if (o && o->clan_enchantment.effect == effect && o->clan_enchantment.active)
      return true;
  }
  return false;
}
