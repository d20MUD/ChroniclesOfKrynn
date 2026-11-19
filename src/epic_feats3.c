/* Third catalogue release: bounded reactions and explicit effect provenance. */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "spells.h"
#include "spell_prep.h"
#include "psionics.h"
#include "feats.h"
#include "class.h"
#include "mud_event.h"
#include "actions.h"
#include "fight.h"
#include "db.h"
#include "mob_utils.h"
#include "dg_scripts.h"
#include "evolutions.h"
#include "assign_wpn_armor.h"
#include "legendary_effects.h"
#include "epic_feats.h"
#include "epic_feats2.h"
#include "epic_feats3.h"

bool epic_third_feat(int f)
{
  return (f >= FEAT_EPIC_PSIONIC_CONTINGENCY && f <= FEAT_EPIC_ANATOMICAL_SAVANT) ||
         f == FEAT_EPIC_DODGE;
}
int epic_third_class(int f)
{
  if (f == FEAT_EPIC_PSIONIC_CONTINGENCY || f == FEAT_EPIC_SENTINEL_MIND)
    return CLASS_PSIONICIST;
  if (f == FEAT_EPIC_EIDOLON_INTERCESSION || f == FEAT_EPIC_ADAPTIVE_ASPECT)
    return CLASS_SUMMONER;
  if (f == FEAT_LASTING_INSPIRATION)
    return CLASS_BARD;
  return -1;
}
static int highest(struct char_data *ch)
{
  int i, best = 0;
  for (i = 0; i < NUM_CLASSES; i++)
    if (epic_capacity_circle(i))
      best = MAX(best, get_class_highest_circle(ch, i));
  return best;
}
bool epic_third_weapon_choice(struct char_data *ch, int weapon, bool study)
{
  if (weapon < 0 || weapon >= NUM_WEAPON_TYPES)
    return FALSE;
  return IS_SET_AR(ch->char_specials.saved.combat_feats[CFEAT_WEAPON_FOCUS], weapon) ||
         (study && ch->desc && LEVELUP(ch) &&
          IS_SET_AR(LEVELUP(ch)->combat_feats[CFEAT_WEAPON_FOCUS], weapon));
}
bool epic_third_prerequisites(struct char_data *ch, int f, bool study)
{
  int i;
#define F(n) (study ? has_feat_requirement_check(ch, n) : HAS_REAL_FEAT(ch, n))
#define R(a, n) (GET_ABILITY(ch, a) >= n)
  if (!ch || IS_NPC(ch) || GET_LEVEL(ch) < 21)
    return FALSE;
  switch (f)
  {
  case FEAT_EPIC_PSIONIC_CONTINGENCY:
    return CLASS_LEVEL(ch, CLASS_PSIONICIST) >= 23 && GET_REAL_INT(ch) >= 23 &&
           R(ABILITY_SPELLCRAFT, 23) &&
           (is_a_known_spell(ch, CLASS_PSIONICIST, PSIONIC_INERTIAL_ARMOR) ||
            is_a_known_spell(ch, CLASS_PSIONICIST, PSIONIC_BIOFEEDBACK) ||
            is_a_known_spell(ch, CLASS_PSIONICIST, PSIONIC_VIGOR));
  case FEAT_EPIC_SENTINEL_MIND:
    return CLASS_LEVEL(ch, CLASS_PSIONICIST) >= 25 && GET_REAL_INT(ch) >= 25 &&
           R(ABILITY_CONCENTRATION, 23);
  case FEAT_EPIC_EIDOLON_INTERCESSION:
    return CLASS_LEVEL(ch, CLASS_SUMMONER) >= 23 && GET_REAL_CHA(ch) >= 23 &&
           F(FEAT_GREATER_SHIELD_ALLY);
  case FEAT_EPIC_ADAPTIVE_ASPECT:
    return CLASS_LEVEL(ch, CLASS_SUMMONER) >= 23 && F(FEAT_ASPECT) && R(ABILITY_SPELLCRAFT, 20);
  case FEAT_EPIC_SPELL_OPPORTUNITY:
    return R(ABILITY_SPELLCRAFT, 23) && F(FEAT_COMBAT_REFLEXES) && highest(ch) >= 3;
  case FEAT_EPIC_SPELL_STOWAWAY:
  case FEAT_TENACIOUS_WARDS:
    return R(ABILITY_SPELLCRAFT, 23) && highest(ch) >= 5;
  case FEAT_EPIC_DODGE:
    return GET_REAL_DEX(ch) >= 23 && F(FEAT_DODGE) && F(FEAT_MOBILITY) && R(ABILITY_ACROBATICS, 23);
  case FEAT_EPIC_HEROIC_RECOVERY:
    return GET_REAL_CON(ch) >= 21 && F(FEAT_ENDURANCE) && F(FEAT_DIEHARD) &&
           R(ABILITY_DISCIPLINE, 20);
  case FEAT_LASTING_INSPIRATION:
    return CLASS_LEVEL(ch, CLASS_BARD) >= 23 && GET_REAL_CHA(ch) >= 23 && F(FEAT_BARDIC_MUSIC) &&
           R(ABILITY_PERFORM, 23);
  case FEAT_EPIC_ANATOMICAL_SAVANT:
    if ((CLASS_LEVEL(ch, CLASS_ROGUE) < 20 && CLASS_LEVEL(ch, CLASS_ASSASSIN) < 8) ||
        GET_REAL_DEX(ch) < 23 || F(FEAT_SNEAK_ATTACK) < 8)
      return FALSE;
    if (!study && EPIC_CHOICES(ch).anatomy_weapon)
      return epic_third_weapon_choice(ch, EPIC_CHOICES(ch).anatomy_weapon - 1, FALSE);
    for (i = 0; i < NUM_WEAPON_TYPES; i++)
      if (epic_third_weapon_choice(ch, i, study))
        return TRUE;
    return FALSE;
  }
#undef F
#undef R
  return FALSE;
}
static bool suppressed(struct char_data *ch)
{
  return !ch || IN_ROOM(ch) == NOWHERE || ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOMAGIC);
}
static struct char_data *primary(struct char_data *ch)
{
  struct follow_type *f;
  for (f = ch->followers; f; f = f->next)
    if (IS_NPC(f->follower) && f->follower->master == ch && AFF_FLAGGED(f->follower, AFF_CHARM) &&
        MOB_FLAGGED(f->follower, MOB_EIDOLON) && !char_has_mud_event(f->follower, ePURGEMOB) &&
        IN_ROOM(f->follower) == IN_ROOM(ch) && GET_HIT(f->follower) > 0)
      return f->follower;
  return NULL;
}
static void arm(struct char_data *ch, int spell, int seconds)
{
  struct affected_type af;
  new_affect(&af);
  af.spell = spell;
  af.location = APPLY_SPECIAL;
  af.duration = MAX(1, seconds / 6);
  af.legendary_expiry = time(NULL) + seconds;
  affect_to_char(ch, &af);
}
static bool active(struct char_data *ch, int spell)
{
  struct affected_type *af;
  for (af = ch->affected; af; af = af->next)
    if (af->spell == spell && af->legendary_expiry > time(NULL))
      return TRUE;
  return FALSE;
}
static bool ward_spell(int spell)
{
  switch (spell)
  {
  case SPELL_MAGE_ARMOR:
  case SPELL_SHIELD:
  case SPELL_BARKSKIN:
  case SPELL_BLUR:
  case SPELL_FLY:
  case SPELL_RESIST_ENERGY:
  case SPELL_PROTECTION_FROM_ENERGY:
    return TRUE;
  default:
    return FALSE;
  }
}
void epic_third_failed_save(struct char_data *source, struct char_data *vict, int spell, int type,
                            int dc)
{
  vict->epic_last_save_spell = spell;
  vict->epic_last_save_type = type;
  vict->epic_last_save_dc = dc;
  vict->epic_last_save_source = source && !IS_NPC(source) ? GET_IDNUM(source) : 0;
}
void epic_third_provenance(struct char_data *ch, struct char_data *vict, struct affected_type *af,
                           int spell, int level, int casttype)
{
  af->epic_source = IS_NPC(ch) ? 0 : GET_IDNUM(ch);
  af->epic_cl = level;
  af->epic_cast_serial = ch->epic_cast_serial;
  af->epic_ward = casttype == CAST_SPELL && ch->epic_spell_depth == 1 && !ch->epic_reaction_depth &&
                          ward_spell(spell) && epic_feat_rank(ch, FEAT_TENACIOUS_WARDS)
                      ? 4
                      : 0;
  if ((vict->epic_last_save_spell == spell ||
       (spell == SPELL_POISON_BREATH && vict->epic_last_save_spell == SPELL_POISON)) &&
      (vict->epic_last_save_type == SAVING_FORT || vict->epic_last_save_type == SAVING_POISON) &&
      vict->epic_last_save_source == af->epic_source && !IS_NPC(vict) && !vict->epic_save_retry)
  {
    af->epic_save_dc = vict->epic_last_save_dc;
    af->epic_save_type = vict->epic_last_save_type;
  }
}
int epic_third_dispel_cl(struct char_data *source, struct char_data *vict)
{
  struct affected_type *af = vict->affected;
  return af && af->epic_ward && source != vict && !is_player_grouped(source, vict)
             ? af->epic_cl + af->epic_ward
             : CASTER_LEVEL(vict);
}
static bool mental(int spell)
{
  return is_fear_spell(spell) || spell == SPELL_FEAR || spell == SPELL_SCARE ||
         spell == SPELL_DRAGONFEAR || spell == PSIONIC_DEMORALIZE || spell == PSIONIC_DEADLY_FEAR ||
         spell == PSIONIC_MOMENT_OF_TERROR || spell == PSIONIC_PSYCHOSIS ||
         spell == SPELL_SCINT_PATTERN || spell == AFFECT_AURA_OF_TERROR ||
         spell == EVOLUTION_FRIGHTFUL_EFFECT || spell == WARLOCK_FRIGHTFUL_BLAST ||
         spell == WARLOCK_BEWITCHING_BLAST || spell == SPELL_CHARM ||
         spell == SPELL_CHARM_MONSTER || spell == SPELL_CHARM_ANIMAL ||
         spell == SPELL_DOMINATE_PERSON || spell == SPELL_MASS_DOMINATION ||
         spell == SPELL_CONFUSION || spell == ABILITY_VAMPIRIC_DOMINATION || spell == WARLOCK_CHARM;
}
int epic_third_sentinel(struct char_data *vict, int spell, int type, int base, int dc, int insight)
{
  int roll;
  if (IS_NPC(vict) || vict->epic_save_retry || !mental(spell) || suppressed(vict) ||
      !epic_feat_rank(vict, FEAT_EPIC_SENTINEL_MIND) || !active(vict, AFFECT_EPIC_SENTINEL))
    return -1;
  affect_from_char(vict, AFFECT_EPIC_SENTINEL);
  save_char(vict, 0);
  vict->epic_save_retry = TRUE;
  roll = d20(vict);
  base += MAX(0, epic_typed_increment(vict,
                               type == SAVING_FORT     ? APPLY_SAVING_FORT
                               : type == SAVING_REFL   ? APPLY_SAVING_REFL
                               : type == SAVING_POISON ? APPLY_SAVING_POISON
                               : type == SAVING_DEATH  ? APPLY_SAVING_DEATH
                                                       : APPLY_SAVING_WILL,
                               0, BONUS_TYPE_INSIGHT, 2) - insight);
  vict->epic_save_retry = FALSE;
  send_to_char(vict, "Your sentinel mind retries the saving throw against DC %d.\r\n", dc);
  return roll == 20 || (roll != 1 && roll + base >= dc);
}
int epic_third_intercept(struct char_data *source, struct char_data *vict, int damage,
                         unsigned int origin)
{
  struct char_data *ch, *pet;
  int amount = MIN(20, damage);
  if (origin || !source || source == vict || IS_NPC(vict) || amount <= 0 ||
      is_player_grouped(source, vict))
    return damage;
  for (ch = world[IN_ROOM(vict)].people; ch; ch = ch->next_in_room)
  {
    if (IS_NPC(ch) || ch == vict || !AWAKE(ch) || suppressed(ch) || !is_player_grouped(ch, vict) ||
        E3V(ch, E3_INTER_TARGET) != GET_IDNUM(vict) || E3V(ch, E3_INTER_END) <= time(NULL) ||
        !active(ch, AFFECT_EPIC_INTERCESSION) ||
        !epic_feat_rank(ch, FEAT_EPIC_EIDOLON_INTERCESSION))
      continue;
    pet = primary(ch);
    if (!pet || GET_HIT(pet) <= amount)
      continue;
    E3V(ch, E3_INTER_END) = 0;
    affect_from_char(ch, AFFECT_EPIC_INTERCESSION);
    GET_HIT(pet) -= amount;
    if (!save_char_pets_checked(ch))
    {
      save_char(ch, 0);
      return damage;
    }
    save_char(ch, 0);
    return damage - amount;
  }
  return damage;
}
static void contingency(struct char_data *ch)
{
  int spell = E3V(ch, E3_POWER), ml = MIN(GET_PSIONIC_LEVEL(ch), E3V(ch, E3_POWER_ML)),
      old = GET_AUGMENT_PSP(ch);
  struct affected_type af, *a;
  affect_from_char(ch, AFFECT_EPIC_CONTINGENCY);
  E3V(ch, E3_POWER) = 0;
  save_char(ch, 0);
  ch->epic_reaction_depth++;
  GET_AUGMENT_PSP(ch) = 0;
  if (spell == PSIONIC_VIGOR)
  {
    if (MAX(epic_temporary_remaining(ch), legendary_temporary_remaining(ch)) < 5)
    {
      epic_temporary_clear(ch);
      legendary_temporary_clear(ch);
      new_affect(&af);
      af.spell = AFFECT_EPIC_SHARED_VIGOR;
      af.location = APPLY_SPECIAL;
      af.modifier = 5;
      af.duration = 10;
      af.legendary_expiry = time(NULL) + 60;
      affect_to_char(ch, &af);
    }
  }
  else
    mag_affects(ml, ch, ch, NULL, spell, SAVING_WILL, CAST_INNATE, 0);
  for (a = ch->affected; a; a = a->next)
    if (a->spell == spell && a->duration > 10)
    {
      a->duration = 10;
      a->legendary_expiry = time(NULL) + 60;
    }
  GET_AUGMENT_PSP(ch) = old;
  ch->epic_reaction_depth--;
  save_char(ch, 0);
}
void epic_third_after_damage(struct char_data *source, struct char_data *vict, int damage,
                             unsigned int origin)
{
  if (origin || !source || source == vict || IS_NPC(vict) || damage <= 0 || GET_HIT(vict) <= 0 ||
      GET_POS(vict) <= POS_DEAD || GET_HIT(vict) > GET_MAX_HIT(vict) / 4 ||
      is_player_grouped(source, vict) || suppressed(vict) ||
      !epic_feat_rank(vict, FEAT_EPIC_PSIONIC_CONTINGENCY) ||
      !active(vict, AFFECT_EPIC_CONTINGENCY))
    return;
  contingency(vict);
}

void epic_third_cast_done(struct char_data *caster, struct char_data *target, int spell, int level,
                          int casttype, bool success)
{
  struct char_data *watcher;
  long caster_id, *watchers = NULL;
  int count = 0, index;
  if (!caster || IN_ROOM(caster) == NOWHERE || GET_HIT(caster) <= 0 || !success || casttype != CAST_SPELL || caster->epic_spell_depth != 0 ||
      caster->epic_reaction_depth || spell <= 0 || spell >= NUM_SPELLS || IS_EPIC_SPELL(spell) ||
      is_spellnum_psionic(spell))
    return;
  caster_id = GET_ID(caster);
  for (watcher = world[IN_ROOM(caster)].people; watcher; watcher = watcher->next_in_room)
    if (!IS_NPC(watcher) && watcher != caster)
    {
      long *grown = realloc(watchers, (count + 1) * sizeof(*watchers));
      if (!grown)
      {
        free(watchers);
        return;
      }
      watchers = grown;
      watchers[count++] = GET_ID(watcher);
    }
  for (index = 0; index < count; index++)
  {
    long watcher_id;
    watcher = find_char(watchers[index]);
    if (!watcher || IN_ROOM(watcher) != IN_ROOM(caster))
      continue;
    if (IS_NPC(watcher) || watcher == caster || GET_HIT(watcher) <= 0 || !AWAKE(watcher) ||
        suppressed(watcher) || AFF_FLAGGED(watcher, AFF_STUN) || AFF_FLAGGED(watcher, AFF_DAZED) ||
        AFF_FLAGGED(watcher, AFF_PARALYZED) || !CAN_SEE(watcher, caster))
      continue;
    watcher_id = GET_ID(watcher);
    if (target == caster && E3V(watcher, E3_STOW_END) > time(NULL) &&
        E3V(watcher, E3_STOW_TARGET) == GET_IDNUM(caster) && E3V(watcher, E3_STOW_SPELL) == spell &&
        is_player_grouped(watcher, caster) && epic_feat_rank(watcher, FEAT_EPIC_SPELL_STOWAWAY))
    {
      struct affected_type *af, copy;
      E3V(watcher, E3_STOW_END) = 0;
      save_char(watcher, 0);
      for (af = caster->affected; af; af = af->next)
        if (af->spell == spell && af->epic_source == GET_IDNUM(caster) &&
            af->epic_cast_serial == caster->epic_cast_serial && !(af->epic_flags & E3_TAIL))
        {
          copy = *af;
          copy.next = NULL;
          copy.duration = MAX(1, af->duration / 2);
          copy.legendary_expiry = time(NULL) + copy.duration * 6;
          copy.epic_ward = 0;
          copy.epic_cast_serial = 0;
          affect_join(watcher, &copy, FALSE, FALSE, FALSE, FALSE);
        }
      save_char(watcher, 0);
    }
    if (spell_info[spell].violent && E3V(watcher, E3_OP_END) > time(NULL) &&
        E3V(watcher, E3_OP_TARGET) == GET_ID(caster) &&
        epic_feat_rank(watcher, FEAT_EPIC_SPELL_OPPORTUNITY) && !IS_CASTING(watcher) &&
        !AFF_FLAGGED(watcher, AFF_SILENCED) && aoeOK(watcher, caster, SPELL_MAGIC_MISSILE))
    {
      int reaction = E3V(watcher, E3_OP_SPELL), cl = E3V(watcher, E3_OP_CLASS),
          oldclass = GET_CASTING_CLASS(watcher), oldspell = CASTING_SPELLNUM(watcher),
          oldmeta = CASTING_METAMAGIC(watcher);
      E3V(watcher, E3_OP_END) = 0;
      save_char(watcher, 0);
      if (cl < 0 || cl >= NUM_CLASSES || !is_a_known_spell(watcher, cl, reaction) ||
          is_player_grouped(watcher, caster))
        continue;
      watcher->epic_reaction_depth++;
      GET_CASTING_CLASS(watcher) = cl;
      CASTING_SPELLNUM(watcher) = reaction;
      CASTING_METAMAGIC(watcher) = 0;
      call_magic(watcher, caster, NULL, reaction, 0,
                 practiced_spellcaster_level(watcher, cl, CLASS_LEVEL(watcher, cl)), CAST_SPELL);
      if (find_char(watcher_id) == watcher)
      {
        watcher->epic_reaction_depth--;
        GET_CASTING_CLASS(watcher) = oldclass;
        CASTING_SPELLNUM(watcher) = oldspell;
        CASTING_METAMAGIC(watcher) = oldmeta;
      }
      if (find_char(caster_id) != caster || GET_POS(caster) == POS_DEAD)
        break;
    }
  }
  free(watchers);
  (void)level;
}
void epic_third_performance(struct char_data *ch, struct affected_type *af, int component)
{
  bool eligible = (af->spell == SKILL_DANCE_OF_PROTECTION && component < 2) ||
                  (af->spell == SKILL_SONG_OF_HEROISM && component < 2);
  if (!eligible || !epic_feat_rank(ch,FEAT_LASTING_INSPIRATION))
    return;
  af->epic_source = IS_NPC(ch) ? 0 : GET_IDNUM(ch);
  af->epic_flags = E3_PERFORMANCE;
  af->epic_baseline = time(NULL) + MAX(1, af->duration) * 6;
}
static void stop_music_matching(struct char_data *ch, int spell)
{
  struct char_data *recipient;
  struct affected_type *af;
  if (!epic_feat_rank(ch, FEAT_LASTING_INSPIRATION) || !AWAKE(ch) ||
      AFF_FLAGGED(ch, AFF_SILENCED) || GET_POS(ch) <= POS_STUNNED)
    return;
  for (recipient = world[IN_ROOM(ch)].people; recipient; recipient = recipient->next_in_room)
    if (is_player_grouped(ch, recipient))
      for (af = recipient->affected; af; af = af->next)
        if ((spell < 0 || af->spell == spell) && af->epic_source == GET_IDNUM(ch) &&
            (af->epic_flags & E3_PERFORMANCE) &&
            !(af->epic_flags & E3_TAIL) && af->epic_baseline > time(NULL))
        {
          af->epic_flags |= E3_TAIL;
          af->legendary_expiry = af->epic_baseline + 18;
          af->duration = MAX(1, (af->legendary_expiry - time(NULL) + 5) / 6);
        }
}
void epic_third_stop_music(struct char_data *ch)
{
  stop_music_matching(ch, -1);
}
void epic_third_stop_song(struct char_data *ch, int spell)
{
  stop_music_matching(ch, spell);
}

void epic_third_update(void)
{
  struct char_data *ch, *source;
  struct affected_type *af, *next;
  for (ch = character_list; ch; ch = ch->next)
  {
    if (IN_ROOM(ch) == NOWHERE)
      continue;
    for (af = ch->affected; af; af = next)
    {
      next = af->next;
      if (!(af->epic_flags & E3_TAIL))
        continue;
      for (source = world[IN_ROOM(ch)].people; source; source = source->next_in_room)
        if (!IS_NPC(source) && GET_IDNUM(source) == af->epic_source)
          break;
      if (!source || !AWAKE(source) || AFF_FLAGGED(source, AFF_SILENCED) ||
          !is_player_grouped(source, ch) ||
          ((af->epic_flags & E3_TAIL) && !epic_feat_rank(source, FEAT_LASTING_INSPIRATION)) ||
          (!(af->epic_flags & E3_TAIL) && !IS_PERFORMING(source)))
      {
        if(af->epic_baseline<=time(NULL)) affect_remove(ch,af);
        else
        {
          af->legendary_expiry=af->epic_baseline;
          af->duration=MAX(1,(af->epic_baseline-time(NULL)+5)/6);
          af->epic_flags=0; /* Keep the original native contribution, never renew a lost tail. */
        }
      }
    }
  }
}
int epic_third_evolution(struct char_data *ch, int evo, int native)
{
  if (IS_NPC(ch) || E3V(ch, E3_ASPECT_END) <= time(NULL) ||
      !epic_feat_rank(ch, FEAT_EPIC_ADAPTIVE_ASPECT))
    return native;
  if (evo == E3V(ch, E3_ASPECT_OLD))
    return MAX(0, native - 1);
  if (evo == E3V(ch, E3_ASPECT_NEW))
    return MAX(1, native);
  return native;
}
void epic_third_respec(struct char_data *ch)
{
  memset(EPIC_THIRD(ch).value, 0, sizeof(EPIC_THIRD(ch).value));
  affect_from_char(ch, AFFECT_EPIC_CONTINGENCY);
  affect_from_char(ch, AFFECT_EPIC_SENTINEL);
  affect_from_char(ch, AFFECT_EPIC_INTERCESSION);
}
bool epic_third_dodge(struct char_data *attacker, struct char_data *vict, int wtype, int damtype)
{
  if (IS_NPC(vict) || attacker->epic_attack_depth != 1 || attacker->epic_attack_critical ||
      !(wtype >= TOP_ATTACK_TYPES && wtype < BOT_WEAPON_TYPES) ||
      (damtype != DAM_RESERVED_DBC && damtype != DAM_BLUDGEON && damtype != DAM_SLICE &&
       damtype != DAM_PUNCTURE) ||
      GET_POS(vict) < POS_FIGHTING || !AWAKE(vict) || AFF_FLAGGED(vict, AFF_GRAPPLED) ||
      AFF_FLAGGED(vict, AFF_ENTANGLED) || AFF_FLAGGED(vict, AFF_PARALYZED) ||
      AFF_FLAGGED(vict, AFF_STUN) || AFF_FLAGGED(vict, AFF_DAZED) ||
      !epic_feat_rank(vict, FEAT_EPIC_DODGE) || EPIC_THIRD(vict).ready[E3_DODGE] > time(NULL))
    return FALSE;
  EPIC_THIRD(vict).ready[E3_DODGE] = time(NULL) + 30;
  save_char(vict, 0);
  attacker->epic_attack_dodged = TRUE;
  act("You evade $N's blow with epic agility!", FALSE, vict, 0, attacker, TO_CHAR);
  return TRUE;
}
/* Critical immunity is deliberately not used as precision-immunity metadata.
 * The native ordinary organs restriction is Essence of Undeath. */
int epic_third_precision_immunity(struct char_data *vict)
{
  if (HAS_SUBRACE(vict, SUBRACE_SWARM) ||
      (IS_NPC(vict) && (MOB_FLAGGED(vict, MOB_LEGENDARY_BOSS) || MOB_FLAGGED(vict, MOB_NOKILL))))
    return EPIC_PRECISION_HARD;
  return HAS_REAL_FEAT(vict, FEAT_ESSENCE_OF_UNDEATH) ? EPIC_PRECISION_ORDINARY
                                                      : EPIC_PRECISION_NONE;
}
bool epic_third_anatomy(struct char_data *ch, struct char_data *vict, struct obj_data *weapon)
{
  if (epic_third_precision_immunity(vict) != EPIC_PRECISION_ORDINARY || IS_NPC(ch) || !weapon ||
      ch->epic_attack_depth != 1 ||
      GET_WEAPON_TYPE(weapon) + 1 != EPIC_CHOICES(ch).anatomy_weapon ||
      !epic_feat_rank(ch, FEAT_EPIC_ANATOMICAL_SAVANT) ||
      EPIC_THIRD(ch).ready[E3_ANATOMY] > time(NULL) ||
      (IS_NPC(vict) && (MOB_FLAGGED(vict, MOB_LEGENDARY_BOSS) || MOB_FLAGGED(vict, MOB_NOKILL))) ||
      HAS_SUBRACE(vict, SUBRACE_SWARM))
    return FALSE;
  EPIC_THIRD(ch).ready[E3_ANATOMY] = time(NULL) + 6;
  save_char(ch, 0);
  return rand_number(1, 100) <= 50;
}
static bool recoverable(struct affected_type *af)
{
  return af->epic_save_dc > 0 &&
         (af->epic_save_type == SAVING_FORT || af->epic_save_type == SAVING_POISON) &&
         !af->legendary_flags &&
         (IS_SET_AR(af->bitvector, AFF_POISON) || IS_SET_AR(af->bitvector, AFF_DISEASE) ||
          IS_SET_AR(af->bitvector, AFF_SICKENED) || IS_SET_AR(af->bitvector, AFF_FATIGUED));
}
static int selected_power(const char *name)
{
  if (!str_cmp(name, "inertial armor"))
    return PSIONIC_INERTIAL_ARMOR;
  if (!str_cmp(name, "biofeedback"))
    return PSIONIC_BIOFEEDBACK;
  if (!str_cmp(name, "vigor"))
    return PSIONIC_VIGOR;
  return 0;
}
static int named_spell(const char *name)
{
  if (!str_cmp(name, "magic missile"))
    return SPELL_MAGIC_MISSILE;
  if (!str_cmp(name, "shocking grasp"))
    return SPELL_SHOCKING_GRASP;
  if (!str_cmp(name, "ray of enfeeblement"))
    return SPELL_RAY_OF_ENFEEBLEMENT;
  if (!str_cmp(name, "blur"))
    return SPELL_BLUR;
  if (!str_cmp(name, "resist energy"))
    return SPELL_RESIST_ENERGY;
  if (!str_cmp(name, "fly"))
    return SPELL_FLY;
  return 0;
}
static bool reserve(struct char_data *ch, int spell, int class, bool commit)
{
  int circle = compute_spells_circle(ch, class, spell, 0, 0);
  if (circle < 1 || circle > 2 || get_class_highest_circle(ch, class) < 3 ||
      !is_a_known_spell(ch, class, spell))
    return FALSE;
  if (is_spell_in_collection(ch, class, spell, 0))
    return !commit || spell_prep_gen_extract_by_class(ch, class, spell, 0) == class;
  if (class != CLASS_SORCERER && class != CLASS_BARD && class != CLASS_INQUISITOR &&
      class != CLASS_SUMMONER)
    return FALSE;
  if (compute_slots_by_circle(ch, class, circle) - count_total_slots(ch, class, circle) <= 0)
    return FALSE;
  if (commit)
    innate_magic_add(ch, class, circle, 0, compute_spells_prep_time(ch, class, circle, 0), 0);
  return TRUE;
}

bool epic_third_command(struct char_data *ch, const char *verb, const char *argument)
{
  static const struct
  {
    const char *name;
    int feat, use, seconds, action;
  } commands[] = {
      {"psi-contingency", FEAT_EPIC_PSIONIC_CONTINGENCY, E3_CONTINGENCY, 300, ACTION_STANDARD},
      {"sentinel-mind", FEAT_EPIC_SENTINEL_MIND, E3_SENTINEL, 300, ACTION_SWIFT},
      {"eidolon-intercession", FEAT_EPIC_EIDOLON_INTERCESSION, E3_INTERCESSION, 60, ACTION_STANDARD},
      {"intercession", FEAT_EPIC_EIDOLON_INTERCESSION, E3_INTERCESSION, 60, ACTION_STANDARD},
      {"adaptive-aspect", FEAT_EPIC_ADAPTIVE_ASPECT, E3_ASPECT, 300, ACTION_STANDARD},
      {"spell-opportunity", FEAT_EPIC_SPELL_OPPORTUNITY, E3_OPPORTUNITY, 60, ACTION_SWIFT},
      {"spell-stowaway", FEAT_EPIC_SPELL_STOWAWAY, E3_STOWAWAY, 300, ACTION_SWIFT},
      {"stowaway", FEAT_EPIC_SPELL_STOWAWAY, E3_STOWAWAY, 300, ACTION_SWIFT},
      {"recover", FEAT_EPIC_HEROIC_RECOVERY, E3_RECOVERY, 300, ACTION_STANDARD}};
  char input[MAX_INPUT_LENGTH], discard[MAX_INPUT_LENGTH], a[MAX_INPUT_LENGTH], b[MAX_INPUT_LENGTH];
  const char *rest;
  struct char_data *target = NULL;
  struct affected_type *af = NULL, copy;
  int i, spell = 0, cost = 0, cl = -1, old = 0, replacement = 0, index = 0, choice = 0;
  bool full;
  for (i = 0; i < (int)(sizeof(commands) / sizeof(commands[0])); i++)
    if (!str_cmp(verb, commands[i].name))
      break;
  if (i == (int)(sizeof(commands) / sizeof(commands[0])))
    return FALSE;
  if (!epic_feat_rank(ch, commands[i].feat))
  {
    send_to_char(ch, "You do not meet this epic feat's prerequisites.\r\n");
    return TRUE;
  }
  strlcpy(input, argument, sizeof(input));
  rest = one_argument(input, discard, sizeof(discard));
  skip_spaces_c(&rest);
  strlcpy(b, rest, sizeof(b));
  rest = one_argument(rest, a, sizeof(a));
  skip_spaces_c(&rest);
  if (*rest == '\'' || *rest == '\"')
    rest++;
  strlcpy(discard, rest, sizeof(discard));
  if (*discard && (discard[strlen(discard) - 1] == '\'' || discard[strlen(discard) - 1] == '\"'))
    discard[strlen(discard) - 1] = 0;
  if (!AWAKE(ch) || GET_HIT(ch) <= 0 || IS_CASTING(ch) || !can_continue(ch, TRUE))
    return TRUE;
#define REJECT(msg)                                                                                \
  do                                                                                               \
  {                                                                                                \
    send_to_char(ch, msg "\r\n");                                                                  \
    return TRUE;                                                                                   \
  } while (0)
  if (suppressed(ch))
    REJECT("Magic is suppressed here.");
  if (EPIC_THIRD(ch).ready[commands[i].use] > time(NULL))
    REJECT("That epic ability has not recovered.");
  full = commands[i].use == E3_CONTINGENCY || commands[i].use == E3_ASPECT;
  switch (commands[i].use)
  {
  case E3_CONTINGENCY:
    spell = selected_power(b);
    if (!spell || !is_a_known_spell(ch, CLASS_PSIONICIST, spell))
      REJECT("Choose a known power: inertial armor, biofeedback or vigor.");
    if (active(ch, AFFECT_EPIC_CONTINGENCY))
      REJECT("You already have a contingency armed.");
    cost = psionic_powers[spell].psp_cost + 5;
    break;
  case E3_SENTINEL:
    if (active(ch, AFFECT_EPIC_SENTINEL))
      REJECT("Your sentinel mind is already active.");
    cost = 15;
    break;
  case E3_INTERCESSION:
    target = get_char_vis(ch, a, NULL, FIND_CHAR_ROOM);
    if (!target || target == ch || IS_NPC(target) || !is_player_grouped(ch, target) ||
        GET_HIT(target) <= 0 || !primary(ch))
      REJECT("Choose a living player group ally here, with your primary eidolon present.");
    if (E3V(ch, E3_INTER_END) > time(NULL))
      REJECT("Your intercession is already armed.");
    break;
  case E3_ASPECT:
    if (FIGHTING(ch) || E3V(ch, E3_ASPECT_END) > time(NULL))
      REJECT("Adaptive Aspect requires you to be out of combat, without an active swap.");
    old = atoi(a);
    if (!str_cmp(discard, "gills"))
      replacement = EVOLUTION_GILLS;
    else if (!str_cmp(discard, "swim"))
      replacement = EVOLUTION_SWIM;
    else if (!str_cmp(discard, "scent"))
      replacement = EVOLUTION_SCENT;
    else if (!str_cmp(discard, "flight"))
      replacement = EVOLUTION_FLIGHT;
    if (old < 1 || old >= NUM_EVOLUTIONS || !HAS_REAL_EVOLUTION(ch, old) || !replacement ||
        old == replacement || HAS_EVOLUTION(ch, replacement) ||
        !evolution_list[replacement].pc_avail ||
        evolution_list[replacement].evolution_points > evolution_list[old].evolution_points)
    {
      send_to_char(ch, "Use epic adaptive-aspect <current aspect ID> <gills|swim|scent|flight>. "
                       "Current aspects:\r\n");
      for (index = 1; index < NUM_EVOLUTIONS; index++)
        if (HAS_REAL_EVOLUTION(ch, index))
          send_to_char(ch, "%d) %s (%d points)\r\n", index, evolution_list[index].name,
                       evolution_list[index].evolution_points);
      return TRUE;
    }
    for (index = 1; index < NUM_EVOLUTIONS; index++)
      if (index != old && HAS_REAL_EVOLUTION(ch, index))
        for (choice = 0; choice < 6; choice++)
          if (evolution_list[index].evolution_requirements[choice] == old)
            REJECT("Another active aspect depends on that choice.");
    break;
  case E3_OPPORTUNITY:
    target = get_char_vis(ch, a, NULL, FIND_CHAR_ROOM);
    spell = named_spell(discard);
    if (!target || target == ch || is_player_grouped(ch, target) ||
        !aoeOK(ch, target, SPELL_MAGIC_MISSILE) ||
        (spell != SPELL_MAGIC_MISSILE && spell != SPELL_SHOCKING_GRASP &&
         spell != SPELL_RAY_OF_ENFEEBLEMENT))
      REJECT("Choose a legal enemy here and magic missile, shocking grasp or ray of enfeeblement.");
    if (E3V(ch, E3_OP_END) > time(NULL))
      REJECT("A spell opportunity is already reserved.");
    for (cl = 0; cl < NUM_CLASSES; cl++)
      if (reserve(ch, spell, cl, FALSE))
        break;
    if (cl == NUM_CLASSES)
      REJECT("You need that known first/second-circle spell and an available slot in an eligible "
             "class.");
    break;
  case E3_STOWAWAY:
    target = get_char_vis(ch, a, NULL, FIND_CHAR_ROOM);
    spell = named_spell(discard);
    if (!target || target == ch || IS_NPC(target) || !is_player_grouped(ch, target) ||
        (spell != SPELL_BLUR && spell != SPELL_RESIST_ENERGY && spell != SPELL_FLY))
      REJECT("Choose a visible player group ally here and blur, resist energy or fly.");
    if (E3V(ch, E3_STOW_END) > time(NULL))
      REJECT("Your stowaway is already armed.");
    break;
  case E3_RECOVERY:
    choice = atoi(a);
    for (af = ch->affected; af; af = af->next)
      if (recoverable(af))
      {
        index++;
        if (choice == index)
          break;
        send_to_char(ch, "%d) %s (original DC %d)\r\n", index,
                     af->spell > 0 && af->spell < NUM_SPELLS ? spell_info[af->spell].name
                                                             : "condition",
                     af->epic_save_dc);
      }
    if (!af)
      REJECT("Use epic recover <number> from your eligible condition components above; missing "
             "original save data cannot be retried.");
    break;
  }
  if (commands[i].use == E3_RECOVERY)
  {
    copy = *af;
    copy.next = NULL;
  }
  if (cost > GET_PSP(ch))
    REJECT("You do not have enough PSP.");
  if (!is_action_available(ch, commands[i].action, TRUE) ||
      (full && !is_action_available(ch, ACTION_MOVE, TRUE)))
    return TRUE;
  if (commands[i].use == E3_OPPORTUNITY && !reserve(ch, spell, cl, TRUE))
    REJECT("The reserved slot is no longer available.");
  EPIC_THIRD(ch).ready[commands[i].use] = time(NULL) + commands[i].seconds;
  GET_PSP(ch) -= cost;
  if (commands[i].action == ACTION_SWIFT)
    USE_SWIFT_ACTION(ch);
  else
    USE_STANDARD_ACTION(ch);
  if (full)
    USE_MOVE_ACTION(ch);
  switch (commands[i].use)
  {
  case E3_CONTINGENCY:
    E3V(ch, E3_POWER) = spell;
    E3V(ch, E3_POWER_ML) = GET_PSIONIC_LEVEL(ch);
    arm(ch, AFFECT_EPIC_CONTINGENCY, 300);
    break;
  case E3_SENTINEL:
    arm(ch, AFFECT_EPIC_SENTINEL, 60);
    break;
  case E3_INTERCESSION:
    E3V(ch, E3_INTER_TARGET) = GET_IDNUM(target);
    E3V(ch, E3_INTER_END) = time(NULL) + 18;
    arm(ch, AFFECT_EPIC_INTERCESSION, 18);
    break;
  case E3_ASPECT:
    E3V(ch, E3_ASPECT_OLD) = old;
    E3V(ch, E3_ASPECT_NEW) = replacement;
    E3V(ch, E3_ASPECT_END) = time(NULL) + 60;
    break;
  case E3_OPPORTUNITY:
    E3V(ch, E3_OP_TARGET) = GET_ID(target);
    E3V(ch, E3_OP_SPELL) = spell;
    E3V(ch, E3_OP_CLASS) = cl;
    E3V(ch, E3_OP_END) = time(NULL) + 18;
    break;
  case E3_STOWAWAY:
    E3V(ch, E3_STOW_TARGET) = GET_IDNUM(target);
    E3V(ch, E3_STOW_SPELL) = spell;
    E3V(ch, E3_STOW_END) = time(NULL) + 18;
    break;
  }
  if (!save_char_checked(ch, 0))
  {
    epic_third_respec(ch);
    REJECT("The epic commitment could not be saved; its effect was cancelled.");
  }
  if (commands[i].use == E3_RECOVERY)
  {
    for (af = ch->affected; af; af = af->next)
      if (af->spell == copy.spell && af->location == copy.location &&
          af->specific == copy.specific && af->epic_save_dc == copy.epic_save_dc &&
          af->epic_source == copy.epic_source)
        break;
    if (!af)
      REJECT("The selected condition is no longer present.");
    int roll = d20(ch),
        bonus = epic_typed_increment(
            ch, af->epic_save_type == SAVING_POISON ? APPLY_SAVING_POISON : APPLY_SAVING_FORT, 0,
            BONUS_TYPE_CIRCUMSTANCE, 4);
    if (roll == 20 ||
        (roll != 1 && roll + compute_mag_saves(ch, af->epic_save_type, bonus) >= af->epic_save_dc))
    {
      copy = *af;
      copy.next = NULL;
      REMOVE_BIT_AR(copy.bitvector, AFF_POISON);
      REMOVE_BIT_AR(copy.bitvector, AFF_DISEASE);
      REMOVE_BIT_AR(copy.bitvector, AFF_SICKENED);
      REMOVE_BIT_AR(copy.bitvector, AFF_FATIGUED);
      copy.epic_save_dc = 0;
      affect_remove(ch, af);
      /* Keep unrelated numeric/flag components, including ability damage. */
      if (copy.location != APPLY_NONE || copy.bitvector[0] || copy.bitvector[1] ||
          copy.bitvector[2] || copy.bitvector[3] || copy.bitvector2[0] || copy.bitvector2[1] ||
          copy.bitvector2[2] || copy.bitvector2[3])
        affect_to_char(ch, &copy);
      send_to_char(ch, "You overcome the selected condition component.\r\n");
    }
    else
      send_to_char(ch, "Your recovery attempt fails.\r\n");
    save_char(ch, 0);
  }
  else
    send_to_char(ch, "%s activated.\r\n", feat_list[commands[i].feat].name);
  return TRUE;
#undef REJECT
}
