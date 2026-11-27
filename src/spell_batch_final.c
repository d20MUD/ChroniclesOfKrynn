/* Final spell/power candidates: native affects, damage, DR and action economy. */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "spells.h"
#include "fight.h"
#include "spec_procs.h"
#include "religion.h"
#include "mud_event.h"
#include "actions.h"
#include "grapple.h"
#include "craft.h"
#include "assign_wpn_armor.h"
#include "domains_schools.h"
#include "psionics.h"
#include "dg_scripts.h"
#include "spell_batch_final.h"

/* A persisted runtime UID must not attach a prior process's curse to a new mob. */
static int64_t batch_runtime_stamp(void)
{
  static int64_t stamp;
  if (!stamp) stamp = ((int64_t)time(NULL) << 20) ^ (int64_t)getpid();
  return stamp;
}

static struct affected_type *batch_affect(struct char_data *ch, int spell)
{
  struct affected_type *af;
  if (!ch) return NULL;
  for (af = ch->affected; af; af = af->next)
    if (af->spell == spell && af->location == APPLY_SPECIAL) return af;
  return NULL;
}

static struct obj_data *batch_metal_weapon(struct char_data *ch)
{
  const int slots[] = {WEAR_WIELD_2H, WEAR_WIELD_1, WEAR_WIELD_OFFHAND};
  size_t i;
  for (i = 0; i < sizeof(slots)/sizeof(*slots); i++) {
    struct obj_data *obj = GET_EQ(ch, slots[i]);
    if (obj && GET_OBJ_TYPE(obj) == ITEM_WEAPON && IS_CONDUCTIVE_METAL(GET_OBJ_MATERIAL(obj))) return obj;
  }
  return NULL;
}

int final_batch_blade_bonus(struct char_data *ch)
{
  struct affected_type *af = batch_affect(ch, SPELL_DAZZLING_BLADE);
  const int slots[] = {WEAR_WIELD_2H, WEAR_WIELD_1, WEAR_WIELD_OFFHAND};
  size_t i;
  if (!af) return 0;
  for (i = 0; i < sizeof(slots)/sizeof(*slots); i++) {
    struct obj_data *obj = GET_EQ(ch, slots[i]);
    if (obj && GET_ID(obj) == af->epic_baseline) return af->modifier;
  }
  return 0;
}

/* Legacy flat DR consumes hit flags when queried; preserve them for this probe. */
bool final_batch_other_dr(struct char_data *ch)
{
  struct damage_reduction_type *dr;
  int critical = HIT_BY_CRITICAL(ch), sneak = HIT_BY_SNEAK_ATTACK(ch);
  int flat = compute_damage_reduction(ch, DAM_BLUDGEON);
  HIT_BY_CRITICAL(ch) = critical; HIT_BY_SNEAK_ATTACK(ch) = sneak;
  if (flat > 0 || religion_blessing_value(ch, CHURCH_DR) > 0) return TRUE;
  for (dr = GET_DR(ch); dr; dr = dr->next)
    if (dr->spell != SPELL_DEFENDING_BONE && dr->amount > 0) return TRUE;
  return FALSE;
}

bool final_batch_fervor(struct char_data *ch, int mode)
{
  struct affected_type *af = batch_affect(ch, SPELL_BLESSING_OF_FERVOR);
  return af && af->modifier == mode && !AFF_FLAGGED(ch, AFF_HASTE);
}

/* Dynamic bonuses allow an existing Haste to suppress Fervor without destroying
 * either effect. Battlesense choices are snapshotted, not shared mutable state. */
int final_batch_bonus(struct char_data *ch, int location)
{
  struct affected_type *af = batch_affect(ch, PSIONIC_BATTLESENSE);
  int bonus = 0;
  if (af) {
    if ((af->specific == BATTLESENSE_ATTACK && location == APPLY_HITROLL) ||
        (af->specific == BATTLESENSE_ARMOR && location == APPLY_AC_NEW)) bonus += af->modifier;
    if (af->specific == BATTLESENSE_DAMAGE && location == APPLY_DAMROLL) bonus += 2 * af->modifier;
    if (af->specific == BATTLESENSE_SPEED && location == APPLY_MOVE_SPEED) bonus += 5 * af->modifier;
  }
  if (final_batch_fervor(ch, FERVOR_DEFENSE) &&
      (location == APPLY_HITROLL || location == APPLY_AC_NEW || location == APPLY_SAVING_REFL)) bonus += 2;
  if (final_batch_fervor(ch, FERVOR_SPEED) && location == APPLY_MOVE_SPEED) bonus += 30;
  return bonus;
}

static bool batch_saved(struct char_data *ch, struct char_data *victim, int spell, int level, int school, int cls, int save_type)
{
  int old_class = CASTING_CLASS(ch), old_get_class = GET_CASTING_CLASS(ch);
  int old_spell = CASTING_SPELLNUM(ch), old_dc = GET_DC_BONUS(ch);
  bool saved;
  CASTING_CLASS(ch) = GET_CASTING_CLASS(ch) = cls;
  CASTING_SPELLNUM(ch) = spell; GET_DC_BONUS(ch) = 0;
  saved = savingthrow_full(ch, victim, save_type, 0, CAST_SPELL, level, school, spell);
  CASTING_CLASS(ch) = old_class; GET_CASTING_CLASS(ch) = old_get_class;
  CASTING_SPELLNUM(ch) = old_spell; GET_DC_BONUS(ch) = old_dc;
  return saved;
}

static void batch_join(struct char_data *ch, struct affected_type *af, int location, int modifier, int bonus)
{
  af->location = location; af->modifier = modifier; af->bonus_type = bonus;
  affect_join(ch, af, FALSE, FALSE, FALSE, FALSE);
}

void final_batch_affect(int level, struct char_data *ch, struct char_data *victim, int spell, int metamagic)
{
  struct affected_type af;
  struct obj_data *weapon;
  struct damage_reduction_type *dr;
  int duration = MAX(1, level), step = GET_AUGMENT_PSP(ch) / 2;
  long victim_uid = GET_ID(victim);
  new_affect(&af); af.spell = spell; af.epic_cl = level;
  af.epic_cast_serial = GET_ID(ch); af.specific = CASTING_CLASS(ch);
  af.epic_source = IS_NPC(ch) ? 0 : GET_IDNUM(ch);
  af.epic_baseline = batch_runtime_stamp();
  switch (spell) {
  case SPELL_DEFENDING_BONE:
    if (victim != ch || final_batch_other_dr(victim)) {
      send_to_char(ch, "Defending Bone requires your own body without another source of damage reduction.\r\n"); return;
    }
    duration = MIN(30000, level * 1200);
    break;
  case SPELL_RIGHTEOUS_MIGHT:
    if (victim != ch) return;
    if (GET_SIZE(ch) >= SIZE_COLOSSAL) return;
    if (affected_by_spell(ch, SPELL_ENLARGE_PERSON) || affected_by_spell(ch, SPELL_SHRINK_PERSON) ||
        affected_by_spell(ch, ABILITY_AUGMENT_PHYSICALITY)) {
      send_to_char(ch, "Another magical size change prevents Righteous Might.\r\n"); return;
    }
    break;
  case SPELL_THORN_BODY:
  case SPELL_VINE_STRIKE:
    if (victim != ch) return;
    if (spell == SPELL_VINE_STRIKE) duration = MIN(30000, level * 20);
    break;
  case SPELL_DAZZLING_BLADE:
    if (victim != ch || !(weapon = batch_metal_weapon(ch))) {
      send_to_char(ch, "Wield a metal weapon before casting Dazzling Blade.\r\n"); return;
    }
    duration = MIN(30000, level * 20); af.epic_baseline = GET_ID(weapon);
    break;
  case SPELL_DEBILITATING_PORTENT:
    if (is_immune_mind_affecting(ch, victim, TRUE) || mag_resistance(ch, victim, 0)) return;
    af.epic_save_dc = 10 + level / 2 + GET_WIS_BONUS(ch);
    af.specific = 0;
    break;
  case SPELL_BLESSING_OF_FERVOR:
    break;
  case PSIONIC_BATTLESENSE:
    if (is_immune_mind_affecting(ch, victim, FALSE)) return;
    break;
  case PSIONIC_CRYSTALLINE_BONDS:
    if (AFF_FLAGGED(victim, AFF_FREE_MOVEMENT) || affected_by_spell(victim, PSIONIC_SLIP_THE_BONDS)) return;
    if (GET_SIZE(victim) > SIZE_MEDIUM + (ch->spell_bonds_size_choice ? step : 0)) {
      send_to_char(ch, "That target is too large for these crystalline bonds.\r\n"); return;
    }
    if (IS_INCORPOREAL(victim)) return;
    if (savingthrow_full(ch, victim, SAVING_REFL, -step, CAST_SPELL, level, NOSCHOOL, spell)) return;
    break;
  default: return;
  }
  if (IS_SET(metamagic, METAMAGIC_EXTEND)) duration = MIN(30000, duration * 2);
  af.duration = duration;
  /* Remove old DR before adding the replacement node. */
  affect_from_char(victim, spell);
  if (spell == SPELL_DEFENDING_BONE || spell == SPELL_RIGHTEOUS_MIGHT) {
    dr = calloc(1, sizeof(*dr));
    if (!dr) return;
    dr->spell = spell; dr->duration = duration; dr->feat = FEAT_UNDEFINED;
    dr->amount = spell == SPELL_DEFENDING_BONE ? 5 : level >= 15 ? 10 : 5;
    dr->max_damage = spell == SPELL_DEFENDING_BONE ? MIN(50, level * 5) : -1;
    dr->bypass_cat[0] = spell == SPELL_DEFENDING_BONE ? DR_BYPASS_CAT_DAMTYPE : DR_BYPASS_CAT_ALIGNMENT;
    dr->bypass_val[0] = spell == SPELL_DEFENDING_BONE ? DR_DAMTYPE_BLUDGEONING :
      (IS_EVIL(ch) || (!IS_NPC(ch) && ch->player_specials->saved.channel_energy_type == CHANNEL_ENERGY_TYPE_NEGATIVE)) ? DR_ALIGNTYPE_GOOD : DR_ALIGNTYPE_EVIL;
    dr->next = GET_DR(victim); GET_DR(victim) = dr;
    batch_join(victim, &af, APPLY_DR, 0, BONUS_TYPE_UNDEFINED);
    if (spell == SPELL_RIGHTEOUS_MIGHT) {
      batch_join(victim, &af, APPLY_SIZE, 1, BONUS_TYPE_SIZE);
      batch_join(victim, &af, APPLY_STR, 4, BONUS_TYPE_SIZE);
      batch_join(victim, &af, APPLY_CON, 4, BONUS_TYPE_SIZE);
      batch_join(victim, &af, APPLY_DEX, -2, BONUS_TYPE_SIZE);
      batch_join(victim, &af, APPLY_AC_NEW, 2, BONUS_TYPE_NATURALARMOR);
    }
  } else if (spell == SPELL_DAZZLING_BLADE) {
    batch_join(victim, &af, APPLY_SPECIAL, MIN(5, 1 + level / 3), BONUS_TYPE_UNDEFINED);
  } else if (spell == SPELL_BLESSING_OF_FERVOR) {
    batch_join(victim, &af, APPLY_SPECIAL, ch->spell_fervor_choice, BONUS_TYPE_UNDEFINED);
  } else if (spell == PSIONIC_BATTLESENSE) {
    af.specific = ch->spell_battlesense_choice;
    batch_join(victim, &af, APPLY_SPECIAL, MIN(5, 1 + step), BONUS_TYPE_UNDEFINED);
  } else if (spell == PSIONIC_CRYSTALLINE_BONDS) {
    af.epic_save_dc = 20 + step;
    victim->spell_bonds_last_damage = 0;
    SET_BIT_AR(af.bitvector, AFF_ENTANGLED);
    SET_BIT_AR(af.bitvector, AFF_SILENCED);
    batch_join(victim, &af, APPLY_SPECIAL, 1 + (ch->spell_bonds_size_choice ? 0 : step), BONUS_TYPE_UNDEFINED);
    /* Attribution uses the caster for the initial hit; never access a victim
     * after damage without resolving its UID again. No power resistance. */
    damage(ch, victim, dice(3, 6), spell, DAM_PUNCTURE, FALSE);
    victim = find_char(victim_uid);
    if (!victim) return;
  } else {
    batch_join(victim, &af, APPLY_SPECIAL, level, BONUS_TYPE_UNDEFINED);
  }
  send_to_char(victim, "You are affected by %s.\r\n", spell_info[spell].name);
}

static bool batch_natural_attack(int type)
{
  return type == ATTACK_TYPE_UNARMED ||
         (type >= ATTACK_TYPE_PRIMARY_EVO_BITE && type <= ATTACK_TYPE_PRIMARY_EVO_TRAMPLE);
}

int final_batch_natural_hit(struct char_data *ch, struct char_data *victim, int attack_type)
{
  struct affected_type *af;
  int bonus = 0;
  if (!batch_natural_attack(attack_type) &&
      !(attack_type != ATTACK_TYPE_RANGED && (IS_WILDSHAPED(ch) || IS_MORPHED(ch)))) return 0;
  if (affected_by_spell(ch, SPELL_THORN_BODY)) bonus += dice(1, 6);
  af = batch_affect(ch, SPELL_VINE_STRIKE);
  if (af && !ch->spell_vine_used) {
    int level = af->epic_cl, duration = af->duration, cls = af->specific;
    ch->spell_vine_used = TRUE;
    bonus += dice(1, 6);
    if (!affected_by_spell(victim, SPELL_VINE_STRIKE_RESIST) && !IS_INCORPOREAL(victim) &&
        !AFF_FLAGGED(victim, AFF_FREE_MOVEMENT) && !affected_by_spell(victim, PSIONIC_SLIP_THE_BONDS) &&
        !mag_resistance(ch, victim, 0)) {
      struct affected_type vines;
      bool saved = batch_saved(ch, victim, SPELL_VINE_STRIKE, level, CONJURATION, cls, SAVING_REFL);
      new_affect(&vines); vines.duration = saved ? 1 : MAX(1, duration);
      vines.spell = saved ? SPELL_VINE_STRIKE_RESIST : SPELL_VINE_STRIKE_BIND;
      vines.location = APPLY_SPECIAL;
      if (!saved) SET_BIT_AR(vines.bitvector, AFF_ENTANGLED);
      affect_join(victim, &vines, FALSE, FALSE, FALSE, FALSE);
    }
  }
  return bonus;
}

int final_batch_thorns(struct char_data *attacker, struct char_data *victim, int attack_type)
{
  struct affected_type *af = batch_affect(victim, SPELL_THORN_BODY);
  struct obj_data *weapon;
  if (!af || attack_type == ATTACK_TYPE_RANGED || GET_POS(victim) <= POS_DEAD) return 0;
  weapon = get_wielded(attacker, attack_type);
  if (weapon && GET_OBJ_TYPE(weapon) == ITEM_WEAPON &&
      IS_SET(weapon_list[GET_WEAPON_TYPE(weapon)].weaponFlags, WEAPON_FLAG_REACH)) return 0;
  return dice((GRAPPLE_ATTACKER(victim) == attacker || GRAPPLE_TARGET(victim) == attacker) ? 2 : 1, 6) + MIN(15, af->epic_cl);
}

int final_batch_outgoing(struct char_data *ch, int amount, int spell)
{
  struct affected_type *af = batch_affect(ch, SPELL_DEBILITATING_PORTENT);
  if (ch->spell_portent_half_critical) { ch->spell_portent_half_critical = FALSE; return amount / 2; }
  if (amount <= 0) return amount;
  if (!af || spell == SPELL_THORN_BODY || spell == PSIONIC_CRYSTALLINE_BONDS_TICK) return amount;
  if (!savingthrow_flatdc(NULL, ch, SAVING_WILL, 0, af->epic_save_dc, CAST_INNATE, ENCHANTMENT, SPELL_DEBILITATING_PORTENT)) return amount / 2;
  return amount;
}

bool final_batch_cancel_critical(struct char_data *ch)
{
  struct affected_type *af = batch_affect(ch, SPELL_DEBILITATING_PORTENT);
  if (!af || !af->specific) return FALSE;
  /* specific is an explicit armed bit, not the caster class, for this spell. */
  if (af->specific != -1) return FALSE;
  affect_from_char(ch, SPELL_DEBILITATING_PORTENT);
  ch->spell_portent_half_critical = TRUE;
  return TRUE;
}

bool final_batch_struggle(struct char_data *ch)
{
  struct affected_type *af = batch_affect(ch, PSIONIC_CRYSTALLINE_BONDS);
  int result;
  if (!af && !affected_by_spell(ch, SPELL_VINE_STRIKE_BIND)) return FALSE;
  if (!is_action_available(ch, atSTANDARD, TRUE)) return TRUE;
  USE_STANDARD_ACTION(ch);
  if (!af) {
    affect_from_char(ch, SPELL_VINE_STRIKE_BIND);
    send_to_char(ch, "You pull the clinging vines away.\r\n"); return TRUE;
  }
  result = d20(ch) + MAX(GET_STR_BONUS(ch), compute_ability(ch, ABILITY_ESCAPE_ARTIST));
  if (result >= af->epic_save_dc) {
    affect_from_char(ch, PSIONIC_CRYSTALLINE_BONDS);
    send_to_char(ch, "You break free of the crystalline bonds!\r\n");
  } else send_to_char(ch, "The crystalline bonds hold fast.\r\n");
  return TRUE;
}

int final_batch_bonds_dc(struct char_data *ch)
{
  struct affected_type *af = batch_affect(ch, PSIONIC_CRYSTALLINE_BONDS);
  return af ? 20 + MAX(0, ch->spell_bonds_last_damage) / 2 : 0;
}

/* Snapshot runtime UIDs so lethal ticks cannot invalidate list iteration. */
void final_batch_round(void)
{
  struct char_data *ch;
  struct affected_type *af;
  long *ids;
  size_t count = 0, n = 0;
  for (ch = character_list; ch; ch = ch->next) {
    ch->spell_vine_used = FALSE;
    ch->spell_portent_half_critical = FALSE;
    if (batch_affect(ch, PSIONIC_CRYSTALLINE_BONDS)) count++;
  }
  if (!count) return;
  ids = calloc(count, sizeof(*ids));
  if (!ids) return;
  for (ch = character_list; ch; ch = ch->next)
    if (batch_affect(ch, PSIONIC_CRYSTALLINE_BONDS)) ids[n++] = GET_ID(ch);
  for (n = 0; n < count; n++) {
    ch = find_char(ids[n]); af = batch_affect(ch, PSIONIC_CRYSTALLINE_BONDS);
    if (ch && af && af->duration > 0 && GET_POS(ch) > POS_DEAD && IN_ROOM(ch) != NOWHERE &&
        !ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL) && !ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOMAGIC)) {
      struct char_data *caster = find_char(af->epic_cast_serial);
      if (af->epic_baseline != batch_runtime_stamp() || !caster || GET_POS(caster) <= POS_DEAD || IN_ROOM(caster) != IN_ROOM(ch) ||
          !aoeOK(caster, ch, PSIONIC_CRYSTALLINE_BONDS_TICK)) continue;
      int dealt = damage(caster, ch, dice(af->modifier, 6), PSIONIC_CRYSTALLINE_BONDS_TICK, DAM_PUNCTURE, FALSE);
      ch = find_char(ids[n]); af = batch_affect(ch, PSIONIC_CRYSTALLINE_BONDS);
      if (af && dealt >= 0) ch->spell_bonds_last_damage = dealt;
    }
  }
  free(ids);
}

ACMD(do_spellchoice)
{
  char option[MAX_INPUT_LENGTH], choice[MAX_INPUT_LENGTH];
  struct char_data *target;
  struct affected_type *af;
  argument = one_argument(argument, option, sizeof(option));
  one_argument(argument, choice, sizeof(choice));
  if (!*option) {
    send_to_char(ch, "spellchoice fervor defense|speed|attack|rise\r\n"
                     "spellchoice battlesense attack|armor|damage|speed\r\n"
                     "spellchoice bonds damage|size\r\n"
                     "spellchoice blade <enemy> (discharge)\r\n"
                     "spellchoice portent <enemy> (arm critical protection)\r\n"); return;
  }
  if (!str_cmp(option, "fervor")) {
    int mode = !str_cmp(choice,"defense") ? FERVOR_DEFENSE : !str_cmp(choice,"speed") ? FERVOR_SPEED :
               !str_cmp(choice,"attack") ? FERVOR_ATTACK : !str_cmp(choice,"rise") ? FERVOR_RISE : -1;
    if (mode < 0) { send_to_char(ch, "Choose defense, speed, attack or rise.\r\n"); return; }
    ch->spell_fervor_choice = mode;
  } else if (!str_cmp(option, "battlesense")) {
    int mode = !str_cmp(choice,"attack") ? BATTLESENSE_ATTACK : !str_cmp(choice,"armor") ? BATTLESENSE_ARMOR :
               !str_cmp(choice,"damage") ? BATTLESENSE_DAMAGE : !str_cmp(choice,"speed") ? BATTLESENSE_SPEED : -1;
    if (mode < 0) { send_to_char(ch, "Choose attack, armor, damage or speed.\r\n"); return; }
    ch->spell_battlesense_choice = mode;
  } else if (!str_cmp(option, "bonds")) {
    if (str_cmp(choice,"damage") && str_cmp(choice,"size")) { send_to_char(ch,"Choose damage or size.\r\n"); return; }
    ch->spell_bonds_size_choice = !str_cmp(choice,"size");
  } else if (!str_cmp(option,"blade") || !str_cmp(option,"portent")) {
    bool blade = !str_cmp(option,"blade");
    target = get_char_room_vis(ch, choice, NULL);
    if (!target || target == ch || IN_ROOM(ch) == NOWHERE || ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL) ||
        ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOMAGIC) || (IS_NPC(target) && MOB_FLAGGED(target, MOB_NOKILL)) ||
        !aoeOK(ch, target, SPELL_DAZZLING_BLADE_FLASH)) {
      send_to_char(ch,"Choose an eligible enemy here outside a peaceful or no-magic room.\r\n"); return;
    }
    if (blade) {
      struct affected_type flash;
      int level, cls;
      bool saved;
      af = batch_affect(ch, SPELL_DAZZLING_BLADE);
      if (!af || !final_batch_blade_bonus(ch)) { send_to_char(ch,"You need your dazzling metal weapon wielded.\r\n"); return; }
      if (!is_action_available(ch, atSWIFT, TRUE)) return;
      USE_SWIFT_ACTION(ch); level = af->epic_cl; cls = af->specific;
      affect_from_char(ch, SPELL_DAZZLING_BLADE);
      if (!is_immune_mind_affecting(ch, target, FALSE) && !mag_resistance(ch, target, 0)) {
        saved = batch_saved(ch,target,SPELL_DAZZLING_BLADE,level,ILLUSION,cls,SAVING_WILL);
        new_affect(&flash); flash.spell=SPELL_DAZZLING_BLADE_FLASH;flash.duration=1;
        if (!saved && can_blind(target)) SET_BIT_AR(flash.bitvector,AFF_BLIND);
        else SET_BIT_AR(flash.bitvector,AFF_DAZZLED);
        affect_join(target,&flash,FALSE,FALSE,FALSE,FALSE);
      }
      if (!FIGHTING(ch)) set_fighting(ch,target);
      if (!FIGHTING(target)) set_fighting(target,ch);
      act("Your weapon releases a dazzling flash at $N!",FALSE,ch,NULL,target,TO_CHAR);
    } else {
      af=batch_affect(target,SPELL_DEBILITATING_PORTENT);
      if (!af || af->epic_cast_serial != GET_ID(ch)) {send_to_char(ch,"That enemy does not bear your portent.\r\n");return;}
      if (!is_action_available(ch,atSWIFT,TRUE)) return;
      USE_SWIFT_ACTION(ch);af->specific=-1;
      send_to_char(ch,"Your portent will negate that enemy's next confirmed critical and halve its damage.\r\n");
    }
    return;
  } else { send_to_char(ch,"Use spellchoice for syntax.\r\n");return; }
  send_to_char(ch,"Choice set for your next cast or manifestation. Active effects retain their original choice.\r\n");
}
