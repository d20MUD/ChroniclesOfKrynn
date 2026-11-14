# Weapon special ability mechanics review

Reviewed all 40 `WEAPON_SPECAB_*` definitions against the current source.
The player-facing reference is `docs/help/weapon_effects.hlp`. This change
publishes documentation; it does not change mechanics or quartermaster rolls.

The ordinary-item dispatcher tests each item's stored activation mask. Clan
items synthesize that mask from `special_ability_info`, and separately gate
all effects on their saved active state. Consequently an ordinary customized
item can run triggers that the current clan/default registration omits.

## Findings that matter when choosing an effect

| Effect | Current behavior or limitation | Source evidence |
| --- | --- | --- |
| Adaptive | Positive non-thrown ranged Strength damage is 1.5x, not merely full Strength. | `fight.c:compute_damage_bonus` |
| Agile | Dexterity contribution is capped by highest wielded Agile normal enhancement, then compared with Strength. No minimum cap for clan items. | `assign_wpn_armor.c:get_agile_weapon_dex_bonus`, `fight.c:compute_damage_bonus` |
| Defending | Clan uses max(1, enhancement); ordinary uses floor(base damage dice count / 2). Adds armor protection and subtracts from enhancement attack bonus. | `assign_wpn_armor.c:get_defending_weapon_bonus`, `fight.c:compute_armor_class`, attack bonus calculation |
| Disruption | Default/clan mask lacks ON_CRIT, despite a critical handler existing. Default result is 2d6 holy versus undead on hit. | `spec_abilities.c:initialize_special_abilities`, `weapon_specab_disruption` |
| Vicious | Default/clan mask lacks ON_CRIT. Normal active hit is 3d6 negative versus target, 1d4 backlash. | `spec_abilities.c:initialize_special_abilities`, `weapon_specab_vicious` |
| Wounding | Default/clan mask is ACTMTD_NONE, but its procedure only handles ON_HIT/ON_CRIT. Clan Wounding currently gives no bleeding. Custom world masks can enable it. | `spec_abilities.c:initialize_special_abilities`, `weapon_specab_wounding`, `clan_enchant.c:clan_enchant_ability` |
| Dancing / Mighty Cleaving | Combat hooks explicitly check an active clan enchantment, not ordinary ability-list presence. | `fight.c:perform_attacks`, `perform_violence`, `handle_cleave`; `clan_enchant.c:clan_enchant_wielded` |
| Ki Focus | Techniques test ITEM_KI_FOCUS; a bare ordinary special-ability record does not set it. Clan activation does. | `fight.c` monk technique gates, `clan_enchant.c:effect_flag` |
| Lucky | 10% chance in shared d20 routine, after natural 1/20 early returns. Applies to many attacks and other checks as well as saves. Max across wielded items, not sum. | `utils.c:d20`, `assign_wpn_armor.c:get_lucky_weapon_bonus`; callers in `fight.c`, `magic.c` |
| Vorpal | Separate 5% roll on a critical; exclusions are undead, constructs, ooze and MOB_NOCHARM. Attempts HP+100 negative damage. No natural-20 attack requirement. | `spec_abilities.c:weapon_specab_vorpal` |
| Seeking | Concealment bypass uses `is_using_ranged_weapon`; its THROWING branch follows `get_throwing_weapon`, including the active thrown source/profile. | `fight.c` concealment branch; `assign_wpn_armor.c:is_using_ranged_weapon`, `get_throwing_weapon` |
| Throwing | `thrown_weapon_type` recognizes the ability, but `is_throwing_weapon` still tests native WEAPON_FLAG_THROWN. Newly throwable melee weapons may miss native-thrown Strength damage routing. | `assign_wpn_armor.c:thrown_weapon_type`, `is_throwing_weapon`; `fight.c:compute_damage_bonus` |
| Keen / Ghost Touch | Helper searches wielded slots rather than only the particular attacking weapon; do not imply strict attack-local behavior. | `assign_wpn_armor.c:is_using_keen_weapon`, `is_using_ghost_touch_weapon`; `fight.c:determine_threat_range`, damage handling |
| Returning | Individual non-returning weapons are retained but moved to inventory. Returning keeps them ready and permits full throwing without Quick Draw. | `fight.c:finish_thrown_weapon`, `assign_wpn_armor.c:throwing_allows_full_attack`, `ammunition_is_unlimited` |
| Brilliant Energy / Distance / Merciful / Spell Storing | Definitions/registrations exist, but no connected weapon combat implementation was found. Spell Storing has editor values but no release hook. | Repository-wide symbol searches; `spec_abilities.c:initialize_special_abilities`; `oedit.c:OEDIT_SPECAB_VALUE_1/2` |

## Other reviewed paths

- Anarchic/Axiomatic/Holy/Unholy/Bane add 2d6 after the base critical multiplier
  in `fight.c:compute_hit_damage`. Matching alignment subraces also qualify.
- `weapon_specab_flaming`, `frost`, `shock`, `corrosive` deal 1d6 while their
  property flags are active. Burst critical dice use base weapon-type critMult,
  not the final multiplier modified by feats. Ordinary burst critical branches
  ignore the coating flag; clan inactive state suppresses the whole procedure.
- Icy Burst and Invigorating have their recently added live callback wiring.
  Invigorating restores floor(H/2) on hit and floor(3H/2) on crit, using
  max(2, normal enhancement) for clan weapons. Hit and crit events can both run.
- Blinding: susceptible target, Reflex save, 1 round, two -4 modifiers.
  Bewildering: confusion immunity/MOB_NOCONFUSE, Will save, 1 round.
  Exhausting: Fortitude save, 2-round fatigue, immediate 20 movement loss.
  Thundering: 2d10 sonic regardless of deafness save, then Fortitude versus
  2-round deafness if susceptible.
- Vampiric: living targets only, 1d8 hit / 2d8+2 crit negative damage, healing
  based on positive damage return. Wounding custom triggers create separate
  bleeding affects; `limits.c:point_update` processes each bleeding modifier.
- Speed shares the Haste/Blinding Speed condition; it is not a separate stacking
  grant. Dancing uses bonus main-hand attacks and follows the full-routine gate.
- Affect durations advance through `affect_update` at `PULSE_VIOLENCE` in comm.c.

The earlier quartermaster overview is corrected and links to the detailed
reference. Missing hooks are documented, not repaired or removed from rolls;
those balance/behavior changes should be a separate implementation task.
