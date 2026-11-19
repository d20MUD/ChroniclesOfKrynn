# Ideas for improving underused classes

Original design proposal for Chronicles of Krynn, 2026-11-18. The Bard, Duelist and Stalwart Defender work is now implemented; see [implementation details and final mechanics](CLASS_IMPROVEMENTS_IMPLEMENTATION.md). The other-class prototypes remain follow-up ideas. Numbers below were proposed starting values, rather than claims about current balance.

The focus is Bard, Duelist, and Stalwart Defender. The user identifies these as underused; this review has no population, damage, or encounter telemetry to establish their relative strength. The proposals address friction and class identity visible in the current implementation.

## Recommended direction

| Class | Role to strengthen | First priorities |
| --- | --- | --- |
| Bard | An active support character who remains useful alone | Reliable performance switching, visible contribution, a personal combat benefit |
| Duelist | A lightly armored specialist who turns an enemy's attacks into openings | Fix riposte ownership, bounded offensive parry, a limited answer to precision immunity |
| Stalwart Defender | A durable front-line character who protects companions | Guard integration, practical repositioning, defenses that reward sustained investment |

Start with reliability fixes and a few class features. Avoid making every improvement a purchased perk: a class should deliver its core play style before optional specialization. Retain existing epic feats and useful perk choices.

## Rules shared by these proposals

- Class levels mean actual ranks in the named class, not total character level. New prestige-class features use actual prestige-class ranks; preferred perk classes do not increase these values.
- A round means the existing combat-round scheduler. Cooldowns stated in seconds use elapsed-time events. Do not translate between combat rounds, affect durations, and world ticks using an assumed universal conversion.
- All benefits require a living, conscious character. Attack-dependent features require a legal target and use the normal attack, range, visibility, equipment, and damage rules.
- Extra attacks cannot trigger another extra attack, parry, riposte, or opportunity-attack chain. They grant no additional actions.
- New effects with the same name use the strongest value; durations refresh without adding together. Existing bonus types and stacking rules still apply.
- Offensive abilities preserve peaceful-room, protected-mob, mission, and PvP restrictions. Passive support never starts combat with neutral bystanders.
- Cooldowns and per-round counters survive target switching and stance toggling. Logoff must not reset an elapsed-time cooldown.

## Bard

### What the code already provides

Bards have spells through sixth circle, Bardic Knowledge, numerous healing, defensive, offensive, and utility performances, and Efficient Performance at Bard 10. Starting a performance currently consumes a standard action, or a move action with Efficient Performance. Existing perks include dual-performance machinery, and the epic expansion already includes Epic Inspiration and Lasting Inspiration.

In `do_perform`, the current performance is stopped before the requested replacement is resolved and validated. A misspelled or unavailable song can therefore remove an otherwise useful active performance. The dual-performance branch also needs an end-to-end audit because performance state is cleared before the later second-song checks.

Sources: [class progression](../src/class.c), [performance command and effects](../src/bardic_performance.c), [bard perks](../src/perks.c), [epic inspiration](../src/epic_feats2.c), [lasting inspiration](../src/epic_feats3.c).

### B1. Reliable performance control — highest priority

**Type:** Fix and interface improvement; available to all bards.

- Resolve the requested song, ownership, instrument requirements, and other prerequisites before stopping or changing any current performance.
- Invalid input leaves all existing performances running and consumes no action or performance resource.
- A successful switch retains the existing standard/move action cost and existing upkeep. It does not reset any once-per-performance ability that has not genuinely recovered.
- With an existing dual-performance perk, switching replaces the explicitly selected slot; it must not silently stop both songs. Starting a second song must remain possible through that perk's intended command flow.
- Add `perform list`, `perform status`, and `perform stop`. Preserve existing song selection syntax. Status reports active songs, eligible group recipients, upkeep, and any remaining duration or resource actually used by the implementation.

**Why:** Makes the existing class more dependable without increasing its numerical power.

### B2. Battle Rhythm

**Type:** Automatic Bard feature at Bard 3.

While maintaining at least one valid performance, gain +1 competence to weapon attack and damage rolls. Increase to +2 at Bard 9 and +3 at Bard 15; cap at +3.

- Applies to melee, projectile, and thrown weapon attacks; does not add spell or performance damage.
- The bard must meet normal performance requirements. A lingering effect after the bard stops performing does not qualify.
- Does not stack with another competence bonus to the same roll. Receiving two songs never doubles the benefit.
- This is a personal benefit, independent of how many allies receive the song.

**Why:** Improves solo and small-group play while letting the bard keep using supportive songs.

### B3. Rallying Verse

**Type:** Automatic Bard feature at Bard 7. Proposed command: `rally <groupmate>`.

While performing, spend a swift action to give one same-room group member, including yourself, a +2 morale bonus on all saving throws for two combat rounds. At Bard 15, increase the bonus to +3. Cooldown: 30 seconds, shared across all targets.

- Requires the recipient to be eligible for the active performance; no benefit through silence, deafness, or other exclusions that already prevent that song from reaching them.
- Does not remove an existing condition, grant another save, or break immunity rules.
- A stronger existing morale save bonus takes precedence.

**Why:** Gives the player a timely support decision without competing with their spellcasting standard action or repeating the existing healing songs.

### B4. Revealing Refrain

**Type:** Improvement to the existing Song of Revelation, rather than another song.

Once each combat round, each hostile creature actually affected by Song of Revelation receives a Will save against `10 + floor(Bard level / 2) + Charisma modifier`. On failure, suppress its concealment miss chance against the bard's same-room group for one round.

- Suppression affects concealment only. It does not defeat total cover, blocked sight, darkness that prevents targeting, incorporeality, spell resistance, or untargetability.
- A creature must already be a legal hostile target. This does not reveal or attack neutral hidden players.
- Successful saves receive no additional rolls from other bards that round; use the highest eligible DC for simultaneous applications.
- Keep existing useful revelation effects; audit overlap before adding this rider. Apply normal mind-affecting immunity if the rider uses that category.

**Why:** Gives the bard a distinct encounter contribution against evasive enemies without universally increasing group damage.

### Bard tuning cautions

Do not hand out another general lingering-song mechanic or unrestricted second song: existing perks and Lasting Inspiration already occupy that space. Check bonus types against Song of Heroism and other common buffs before accepting Battle Rhythm's numbers. Show actual prevented damage, restored health, and successful revelations in optional summaries so support contributions are visible.

## Duelist

### What the code already provides

The ten-level class grants Canny Defense, Precise Strike, Parry, Riposte, mobility features, and Crippling Critical. Precise Strike currently adds Duelist level to damage with light or no armor and a free hand, but fails against critical-immune targets. Several defenses depend on total defense.

The total-defense riposte calculation currently reads `HAS_FEAT(ch, FEAT_RIPOSTE)` even though `ch` is the attacker and `victim` is the defending character. This is a concrete ownership issue to verify and fix before judging the class's performance.

Sources: [class progression](../src/class.c), [feature descriptions](../src/feats.c), [Precise Strike and total defense](../src/fight.c).

### D1. Correct and explain defensive mechanics — highest priority

**Type:** Fix and interface improvement.

- Use the defending character's Riposte feature when calculating their riposte bonus.
- Preserve existing total-defense behavior and limits while fixing ownership; test both an attacking duelist and a defending duelist.
- Add a concise status explanation listing whether Canny Defense, Precise Strike, and parrying are active, with the specific equipment, casting, position, or immunity reason when they are unavailable.
- Audit the current weapon/free-hand conditions against the feat descriptions and make help match the intended rules before adding more restrictions.

**Why:** Fixes lost class value and makes equipment choices understandable.

### D2. Measured Parry

**Type:** Automatic feature at Duelist 5, alongside existing Riposte.

Once per combat round, while fighting normally, attempt to parry one otherwise successful, non-critical melee weapon attack directed at you. Requirements: light or no armor, one wielded melee weapon, a free off hand, and no casting in progress.

- Roll `d20 + Duelist level + Dexterity modifier` against the incoming attack's final attack-roll total, including that attacker's d20. Ties favor the attacker.
- On success, negate that attack. If the defender has Riposte, make one immediate primary-weapon attack at a −2 attack penalty against that attacker, provided they remain in melee range.
- The attempt is automatic and consumes no standard, move, or swift action. The attempt is spent even on failure.
- Uses one shared per-round attempt across all attackers. It does not operate during total defense, which retains its existing separate behavior.
- Cannot parry spells, rays, area effects, ranged attacks, maneuvers, or reflected damage. A riposte cannot itself be parried or generate another reactive attack.

**Why:** Allows a duelist to express their defensive identity while participating in ordinary combat.

**Tuning:** Begin with the −2 riposte penalty and one attempt. Compare damage prevented and added damage against the full cost of losing a shield or second weapon.

### D3. Find the Seam

**Type:** Automatic feature at Duelist 6.

When an otherwise eligible weapon hit loses Precise Strike solely because the enemy is immune to critical hits or precision damage, add `floor(Duelist level / 2)` ordinary weapon damage instead.

- Cap the replacement at +5 for the ten-level class.
- Does not restore critical hits, sneak attack, Crippling Critical, or other precision riders.
- Apply normal physical damage reduction and resistance. Do not multiply this replacement on a critical hit.
- Normal Precise Strike and this replacement are mutually exclusive on an attack.

**Why:** Prevents entire enemy families from erasing a major class feature while preserving precision-immune enemies' defensive advantage.

### D4. Exploit the Opening

**Type:** Automatic feature at Duelist 8. Proposed command: `opening <target>`.

Spend a swift action to designate one enemy you are currently fighting in melee. Your next primary-weapon attack against that enemy within two rounds gains +2 insight to hit and, if it hits, +1d6 ordinary weapon damage. Cooldown: 18 seconds.

- Requires the same equipment as Measured Parry. The benefit is consumed on the first eligible attack roll, including a miss.
- Only one opening may exist per duelist. It ends if either participant leaves the room or dies.
- The extra damage is not precision damage and is not multiplied on critical hits.
- Reactive attacks do not consume or receive the benefit; only an ordinary primary attack does.

**Why:** Adds an active offensive decision that works against both bosses and ordinary opponents without permanent critical escalation.

## Stalwart Defender

### What the code already provides

The ten-level class grants AC increases, Defensive Stance, defensive immunities and damage reduction, Renewed Defense, Mobile Defense at level 9, Last Word, and Smash Defense. Stance gives Strength, Constitution, Will-save, and AC benefits; leaving it applies fatigue. Existing commands already include `guard`, `rescue`, and `taunt`, so improvements should reuse those systems.

Sources: [class progression](../src/class.c), [stance, guard, rescue, and taunt](../src/act.offensive.c), [damage and stance processing](../src/fight.c), [feature descriptions](../src/feats.c).

### S1. Guarded Companion

**Type:** Automatic feature at Stalwart Defender 2; extends `guard`.

While in Defensive Stance and guarding a same-room group member, grant that ally +1 circumstance AC against weapon attacks. Increase to +2 at Defender 6 and +3 at Defender 10.

- Only one ally benefits per defender. Multiple defenders do not stack this named benefit on the same ally; use the highest value.
- Requires the defender to be conscious, able to act, and not helpless. The defender and ally must remain within the game's normal melee reach as well as in the same room.
- Does not protect the defender themself, grant spell defenses, or alter the existing guard interception chance.
- Ends immediately when stance or guard ends, range fails, or group eligibility changes. Never leave a stale AC affect after either character is extracted.

**Why:** Makes the defender's presence valuable even when enemies are attacking someone else.

### S2. Measured Advance

**Type:** New feature at Stalwart Defender 4; preserve and improve Mobile Defense at 9.

Once every three combat rounds, spend a move action to reposition within the current room by up to 10 feet without ending Defensive Stance. This does not allow an exit traversal, charge, flee, or teleport.

- Use the native combat-distance system. Rooting, paralysis, insufficient movement, and other normal movement restrictions still apply.
- Costs the normal movement points for the distance moved. It neither grants another move action nor suppresses attacks of opportunity.
- At Defender 9, remove the three-round interval for this in-room maneuver, retaining the move-action cost. Keep any additional existing Mobile Defense behavior that is intentional.
- Ordinary room-to-room travel remains governed by current stance rules. Keep combat repositioning and exit movement as separate paths.

**Why:** Makes stance practical in encounters where targets move, while keeping level 9's mobility improvement meaningful.

### S3. Intercepting Guard

**Type:** Automatic feature at Stalwart Defender 6. Proposed command: `intercept` toggles this behavior for the current guarded ally.

While in stance, once every three combat rounds redirect one otherwise successful single-target weapon attack from your eligible guarded ally to yourself.

- Requirements are the same as Guarded Companion. Check interception after the ally's normal avoidance; a missed attack does not consume it.
- The attack lands on the defender without a second attack roll. Preserve the original attack's critical result; apply the defender's normal damage mitigation and immunities once.
- Intercept only attacks that have not already been redirected by the existing guard system. Never process the same attack through both systems.
- Cannot intercept area effects, spells, combat maneuvers, damage over time, self-damage, or reflected damage. Cannot redirect again, trigger reactive attacks, or start an interception chain.
- Interception remains optional because it can be lethal. Display its enabled state and recovery in guard status.

**Why:** Creates a reliable protective moment rather than requiring enemies to cooperate with the tank's preferred target selection.

### S4. Shielded Recovery

**Type:** Improvement to Renewed Defense at Stalwart Defender 8.

Keep its existing recovery and usage limits. After a successful activation, gain a temporary damage buffer equal to `2 × Stalwart Defender level`, capped at 20, for three combat rounds.

- The buffer absorbs final incoming weapon damage after ordinary mitigation. It does not absorb spell damage, damage over time, or health costs.
- It cannot stack or refresh by toggling stance. Renewed Defense must genuinely activate and spend its existing use.
- It does not increase maximum health, restore health, revive the defender, or trigger healing-based effects.
- Apply it once to intercepted damage just as to any other eligible weapon hit.

**Why:** Adds a short window to recover under pressure without giving unlimited healing or a large permanent damage-reduction increase.

## Other classes worth reviewing next

These are follow-up candidates, not findings that the classes are currently weak. Audit their existing perks, feats, resource systems, and usage before introducing new abilities.

| Candidate | Direction | Initial bounded prototype |
| --- | --- | --- |
| Ranger | Make quarry and terrain expertise useful across more zones | Once per 30 seconds, designate one visible hostile as quarry with a swift action: +1 insight to weapon attacks against it for three rounds; +2 at Ranger 12. One quarry per ranger. Compare with existing mark features before proceeding. |
| Monk | Improve approach against ranged enemies | Once per 30 seconds, spend a move action to advance up to 20 feet within the room and gain +2 dodge AC against ranged weapon attacks until the next round. Normal movement costs and restrictions apply; no free attack. |
| Rogue | Reward successful setup without requiring repeated hiding | After a successful feint, add +2 insight to the next eligible primary-weapon attack against that target within one round. Once per round; no added sneak-attack eligibility against immune targets. Audit existing feint perks first. |
| Assassin | Give preparation a benefit when a target survives | After the existing assassination attempt resolves without killing its target, apply −2 attack for two rounds on a failed Fortitude save, DC `10 + Assassin level + Intelligence modifier`. Once per target per 60 seconds; no new instant-kill chance. Confirm current assassination behavior first. |

Do not add these prototypes merely to keep numerical parity with the three main proposals. Their value depends on an identified gap in those classes' current play.

## Implementation order

1. **Reliability:** performance validation before stopping, dual-song state audit, Riposte defender ownership, clearer class status messages.
2. **Identity:** Battle Rhythm, Measured Parry, Guarded Companion, and Measured Advance. Trial these before adding all remaining abilities.
3. **Encounter coverage:** Find the Seam and Revealing Refrain, focusing on enemies that currently invalidate a class contribution.
4. **Active choices:** Rallying Verse, Exploit the Opening, Intercepting Guard, and Shielded Recovery if the earlier changes leave room for them.
5. Update feat descriptions, command help, study previews, and relevant web-client displays together with each implementation batch.

## Validation and balance review

Compare existing behavior with each trial independently. Use similarly equipped characters of equal total level, and include the actual opportunity cost of prestige-class entry and the preferred perk allocations.

- **Bard:** solo time to defeat a standard enemy, standard actions retained after song setup, resource use per encounter, effective group healing, and damage prevented. Include instruments, silence/deafness, invalid song names, switching, and both dual-performance slots.
- **Duelist:** ordinary damage, damage against precision-immune enemies, successful parries per round, and total extra attacks. Include multiple attackers, total defense, ranged enemies, critical hits, casting, and off-hand equipment changes.
- **Defender:** self damage taken, ally damage prevented, movement actions spent, stance availability across consecutive fights, and death risk when intercepting. Include moving allies, room exits, rescue/guard overlap, fatigue, and logout/extraction cleanup.
- **All three:** test ordinary and epic levels, multiclass entry, solo play, groups with duplicate class members, PvP where permitted, and common boss immunities. New base features must coexist with existing perks and epic feats.

For the first trial, aim for visibly better reliability and role fulfillment without making any single class mandatory. If a defensive ability adds more offense through counters than through ordinary attacks, or multiple support characters multiply the same benefit, reduce or cap that interaction before increasing other classes to match it.
