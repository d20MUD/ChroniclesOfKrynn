# Spellhelp batch 501–550

Ordinary spell definitions end at 540 (`NUM_SPELLS` is 541). This batch reviews
40 defined entries, adding 39 records and verifying the existing Solid Fog
entry. IDs 541–550 contain no ordinary spells and are not populated with invented
entries. The catalog now has 484 distinct mechanics records.

Of this batch's 40 entries, 33 have public registrations, three are disabled
Dragonlance recalls, three are disabled rapid-buff effects, and one is an internal
Aurak death effect. No gameplay handlers were changed or casting enabled.

| ID | Spellhelp name | Availability |
| ---: | --- | --- |
| 501 | recall to palanthas | Dragonlance registration; normal casting disabled |
| 502 | recall to sanction | Dragonlance registration; normal casting disabled |
| 503 | recall to solace | Dragonlance registration; normal casting disabled |
| 504 | minor rapid buff | Normal casting explicitly disabled |
| 505 | rapid buff | Normal casting explicitly disabled |
| 506 | greater rapid buff | Normal casting explicitly disabled |
| 507 | power word silence | Registered |
| 508 | holy aura | Registered |
| 509 | flame arrow | Registered |
| 510 | mass identify | Registered |
| 511 | fire bolt | Registered |
| 512 | jolt | Registered |
| 513 | disrupt undead | Registered |
| 514 | arcane mark | Registered |
| 515 | solid fog | Registered; existing entry verified |
| 516 | enhanced diplomacy | Registered |
| 517 | flare | Registered |
| 518 | grasp | Registered |
| 519 | guidance | Registered |
| 520 | lullaby | Registered |
| 521 | root | Registered |
| 522 | stabilize | Registered |
| 523 | summon instrument | Registered |
| 524 | virtue | Registered |
| 525 | splinter storm | Registered |
| 526 | shockwave | Registered |
| 527 | poison breath | Registered |
| 528 | summon solar | Registered |
| 529 | blink | Registered |
| 530 | hurl through hell | Registered |
| 531 | planar anchor | Registered |
| 532 | bane | Registered |
| 533 | disguise self | Registered |
| 534 | alter self | Registered |
| 535 | litany of damnation | Registered |
| 536 | aurak energy ray | Registered |
| 537 | aurak breath | Registered |
| 538 | aurak death throes | Internal automatic effect; player casting disabled |
| 539 | protection from poison | Registered |
| 540 | panacea | Registered |
| 541–550 | No ordinary spell definitions | Outside the ordinary spell list |

Mechanics were traced through registration dispatch, generic damage and affects,
group recipients, manual spells, combat hooks, skill rolls, movement, summons,
object creation, and race/perk access paths. Existing live school, class, domain,
and target metadata remains in use. Cyan labels, aligned text, wrapping,
zero global damage bonus suppression, and relevant live AC/summon settings are
preserved.

Notable behavior documented:

- Holy Aura grants group defenses, charm immunity, and SR 25 against evil
  casters. Its critical retaliation currently applies blindness to the defender,
  despite checking the attacker and messaging that the attacker is blinded.
- Poison Breath is registered for area dispatch but has no area switch case;
  its retained damage and Constitution-poison handlers are not reached by normal
  casting. It is distinct from the older internal Poison Breathe ID.
- Minor/ordinary Rapid Buff check the caster's level, not the recipient's level;
  all three variants are disabled for normal player casting.
- Guidance grants 1d4 on the next `skill_roll` call and is consumed. Its stored
  +1 marker is not the actual roll bonus.
- Enhanced Diplomacy modifies Persuasion and Intimidate, not Diplomacy. Grasp's
  marker is consumed by the legendary tracking route check, but ordinary climbing
  has no general skill-bonus consumer.
- Mass Identify reports the target's worn equipment to that target; it does not
  identify every carried item or set identification flags.
- Hurl Through Hell deals 5d10 mental damage plus the perk helper's actual
  `10 + Warlock class level` bonus when applicable, with no planar travel.
- Alter Self changes the active race used by race checks and grants its
  size-dependent racial ability bonus; Disguise Self grants a skill bonus only.
- Panacea removes individual qualifying poison/disease records and grants
  temporary immunity, preserving unrelated records sharing the same spell ID.

Validation: sanitizer tests load production registrations through 550, including
campaign conditions and disabled-casting setters, render every defined entry,
reject undefined slots, exercise exact command names, and verify configuration,
colors, alignment, audit records/source paths, and unchanged player state. The
full CMake build passes. No server restart was performed.
