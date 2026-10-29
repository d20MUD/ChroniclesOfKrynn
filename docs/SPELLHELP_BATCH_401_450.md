# Spellhelp batch 401–450

All 50 defined IDs have reviewed mechanics records: 49 new records and the
existing Remove Paralysis entry. The catalog now contains 398 distinct records.
This batch has 44 ordinarily registered spells, four disabled campaign recalls,
one unregistered room-effect ID, and one internal breath ID. No gameplay
handlers were changed.

| ID | Spellhelp name | Availability |
| ---: | --- | --- |
| 401 | divine favor | Registered |
| 402 | silverymoon recall | Forgotten Realms only; normal player casting disabled |
| 403 | heal mount | Registered |
| 404 | resistance | Registered |
| 405 | lesser restoration | Registered |
| 406 | hedging weapons | Registered |
| 407 | honeyed tongue | Registered |
| 408 | shield of fortification | Registered |
| 409 | stunning barrier | Registered |
| 410 | sun metal | Registered |
| 411 | tactical acumen | Registered |
| 412 | veil of positive energy | Registered |
| 413 | bestow weapon proficiency | Registered |
| 414 | effortless armor | Registered |
| 415 | fire of entanglement | Registered |
| 416 | life shield | Registered |
| 417 | litany of defense | Registered |
| 418 | litany of righteousness | Registered |
| 419 | righteous vigor | Registered |
| 420 | sacred space | Unregistered; retained room handler |
| 421 | remove paralysis | Registered; existing record verified |
| 422 | dancing weapon | Registered |
| 423 | spiritual weapon | Registered |
| 424 | undetectable alignment | Registered |
| 425 | weapon of awe | Registered |
| 426 | blinding ray | Registered |
| 427 | holy javelin | Registered |
| 428 | invisibility purge | Registered |
| 429 | keen edge | Registered |
| 430 | weapon of impact | Registered |
| 431 | greater magic weapon | Registered |
| 432 | magic vestment | Registered |
| 433 | protection from energy | Registered |
| 434 | communal protection from energy | Registered |
| 435 | searing light | Registered |
| 436 | divine power | Registered |
| 437 | wind wall | Registered |
| 438 | air walk | Registered |
| 439 | gaseous form | Registered |
| 440 | restoration | Registered |
| 441 | gas breathe | Internal breath; not registered for player casting |
| 442 | minor illusion | Registered |
| 443 | moonbeam | Registered |
| 444 | hellish rebuke | Registered |
| 445 | triboar recall | Forgotten Realms only; normal player casting disabled |
| 446 | luskan recall | Forgotten Realms only; normal player casting disabled |
| 447 | mirabar recall | Forgotten Realms only; normal player casting disabled |
| 448 | ant haul | Registered |
| 449 | mass ant haul | Registered |
| 450 | corrosive touch | Registered |

Descriptions follow actual dispatch and shared combat/equipment consumers,
including weapon events, group recipients, damage types, bonus stacking,
condition gates, and live configuration. Delayed and triggered damage does not
advertise a preserved casting-class global damage bonus; applicable direct
spells retain the existing live bonus reporting and zero-value suppression.
Relevant AC buffs report the configured AC cap.

Implementation discrepancies are explicitly documented:

- Lesser Restoration and Restoration test their own IDs against the restorable
  affliction list and currently remove nothing.
- Fire of Entanglement and Life Shield have retained combat consumers but no
  normal-cast affect case.
- Stunning Barrier applies an AC penalty without the Stunned flag.
- Litany of Defense doubles the shield contribution rather than all armor.
- Righteous Vigor mutates its recorded bonus without recalculating cached attack
  modifiers; its hit healing does not clamp current HP to maximum HP.
- Spiritual/Dancing Weapon deal bludgeoning damage through their event handlers.
- Moonbeam events deal more damage on a successful save than a failed save.
- Minor Illusion alone does not satisfy the attack-interception spell-ID check.
- Corrosive Touch does not have its touch-attack flag enabled.

Validation: the sanitizer harness covers every defined ID through 450, exact
command lookup for all new entries, unavailable status, colors, aligned labels,
read-only state, direct versus delayed configuration reporting, and audit source
coverage. The full CMake build also passes. No server restart was performed.
