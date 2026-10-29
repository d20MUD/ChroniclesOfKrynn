# Complete psionic spellhelp audit

All 87 active `psiono()` registrations and all other defined IDs in the power
range have reviewed records. This adds 102 records and verifies the existing
Deadly Fear, Mental Barrier, and Psionic Blast records, bringing the catalog to
586 distinct mechanics records.

The range contains 105 definitions (1500–1604): 87 registered powers, fifteen
retired/unregistered definitions, and three perk-only IDs. Boundary markers
1499 and 1605 are not powers. The fifteen retired definitions and three perk IDs
remain unavailable for direct manifestation; documentation does not register or
enable them.

| ID | Spellhelp name | Availability |
| ---: | --- | --- |
| 1500 | broker | Registered |
| 1501 | call to mind | Registered |
| 1502 | catfall | Registered |
| 1503 | crystal shard | Registered |
| 1504 | deceleration | Registered |
| 1505 | demoralize | Registered |
| 1506 | ectoplasmic sheen | Retired/unregistered power ID |
| 1507 | energy ray | Registered |
| 1508 | force screen | Registered |
| 1509 | fortify | Registered |
| 1510 | inertial armor | Registered |
| 1511 | inevitable strike | Registered |
| 1512 | mind thrust | Registered |
| 1513 | defensive precognition | Registered |
| 1514 | offensive precognition | Registered |
| 1515 | offensive prescience | Registered |
| 1516 | slumber | Registered |
| 1517 | psionic vigor | Registered |
| 1518 | bestow power | Registered |
| 1519 | biofeedback | Registered |
| 1520 | body equilibrium | Registered |
| 1521 | breach | Registered; missing manual dispatch |
| 1522 | concealing amorpha | Registered |
| 1523 | concussion blast | Registered; area path works only at 4+ augmentation |
| 1524 | detect hostile intent | Registered |
| 1525 | elfsight | Registered |
| 1526 | specified energy adaptation | Registered |
| 1527 | energy push | Registered |
| 1528 | energy stun | Registered |
| 1529 | inflict pain | Registered |
| 1530 | mental disruption | Registered |
| 1531 | mass missive | Retired/unregistered power ID |
| 1532 | psionic lock | Retired/unregistered power ID |
| 1533 | psychic bodyguard | Registered |
| 1534 | recall agony | Registered |
| 1535 | share pain | Retired/unregistered power ID |
| 1536 | swarm of crystals | Registered |
| 1537 | thought shield | Registered |
| 1538 | body adjustment | Registered |
| 1539 | concussive onslaught | Registered |
| 1540 | dispel psionics | Retired/unregistered power ID |
| 1541 | endorphin surge | Registered |
| 1542 | energy burst | Registered |
| 1543 | energy retort | Registered |
| 1544 | eradicate invisibility | Registered |
| 1545 | heightened vision | Registered |
| 1546 | mental barrier | Registered; existing record verified |
| 1547 | mind trap | Registered |
| 1548 | psionic blast | Registered; existing record verified |
| 1549 | forced shared pain | Retired/unregistered power ID |
| 1550 | sharpened edge | Registered |
| 1551 | ubiquitous vision | Registered |
| 1552 | deadly fear | Registered; existing record verified |
| 1553 | death urge | Registered |
| 1554 | empathetic feedback | Registered; malformed augmentation step/maximum |
| 1555 | energy adaptation | Registered |
| 1556 | incite passion | Registered |
| 1557 | intellect fortress | Registered |
| 1558 | moment of terror | Registered |
| 1559 | power leech | Registered |
| 1560 | slip the bonds | Registered |
| 1561 | wither | Registered |
| 1562 | wall of ectoplasm | Registered |
| 1563 | adapt body | Retired/unregistered power ID |
| 1564 | ectoplasmic shambler | Registered |
| 1565 | pierce the veils | Registered |
| 1566 | planar travel | Registered |
| 1567 | power resistance | Registered |
| 1568 | psychic crush | Registered |
| 1569 | psychoport | Registered |
| 1570 | shatter mind blank | Registered |
| 1571 | shrapnel burst | Registered |
| 1572 | tower of iron will | Registered |
| 1573 | upheaval | Registered |
| 1574 | breath of the black dragon | Registered |
| 1575 | brutalize wounds | Registered |
| 1576 | disintegration | Registered |
| 1577 | remote view trap | Retired/unregistered power ID |
| 1578 | sustained flight | Registered |
| 1579 | barred mind | Retired/unregistered power ID |
| 1580 | cosmic awareness | Registered |
| 1581 | energy conversion | Registered |
| 1582 | energy wave | Retired/unregistered power ID |
| 1583 | evade burst | Registered |
| 1584 | oak body | Registered |
| 1585 | psychosis | Registered |
| 1586 | ultrablast | Registered |
| 1587 | body of iron | Registered |
| 1588 | greater psychoportation | Retired/unregistered power ID |
| 1589 | recall death | Registered |
| 1590 | shadow body | Registered |
| 1591 | apopsi | Retired/unregistered power ID |
| 1592 | assimilate | Registered |
| 1593 | timeless body | Retired/unregistered power ID |
| 1594 | barred mind personal | Retired/unregistered power ID |
| 1595 | greater psychoport | Registered; missing manual dispatch |
| 1596 | true metabolism | Registered |
| 1597 | impale mind | Registered |
| 1598 | ectoplasmic goliath | Retired/unregistered power ID |
| 1599 | razor storm | Registered |
| 1600 | psychokinetic thrashing | Registered |
| 1601 | epic psionic ward | Registered |
| 1602 | absolute geas | Perk effect/command; not separately manifestable |
| 1603 | hive commander mark | Perk effect/command; not separately manifestable |
| 1604 | gravity well | Perk effect/command; not separately manifestable |

Descriptions follow actual psiono metadata and runtime paths in magic.c,
spells.c, psionics.c, spell_parser.c, combat, movement, skill calculations,
immunity/resistance helpers, and relevant perk consumers. They cover damage
formulas/types, separate saves and resistance checks, conditions, durations,
augmentation thresholds, target selection, manual command activation, healing,
PSP transfer, summons, terrain, and travel restrictions.

Augmentation descriptions distinguish additional PSP from total cost, use integer
division where the handlers do, and retain runtime step/max metadata. The
manifestation path clamps augmentation to the manifester limit and available
PSP and rounds down to the registered increment. Character feats and focus can
alter those limits. Listed formulas are base formulas; character-specific previews
and exhaustive cross-cutting feat audits remain outside this phase.

Important implementation discrepancies are documented rather than fixed:

- Breach and Greater Psychoport lack their registered manual dispatch cases.
- Concussion Blast below four additional PSP calls its damage handler with no
  target; at four or more it uses the working area loop.
- Inflict Pain and Psychosis remain single-target despite retained area branches.
- Empathetic Feedback passes its registration arguments in the wrong order,
  yielding a 100-PSP increment and maximum 1; legal augmentation becomes zero.
- Body Adjustment spends in one-point steps but gains dice only per two PSP.
- Body Equilibrium accepts augmentation without changing its affect.
- Energy Adaptation variants have no legal augmentation even though shared
  casting code contains a four-PSP quickening branch.
- Mind Trap drains PSP from psionic attackers; its non-psionic nausea branch
  constructs/messages an affect but never installs it. The trap is not consumed.
- Power Leech saves use level zero in the event callback and target the current
  opponent, not the original selected target.
- Bestow Power's points handler deducts PSP in addition to the ordinary
  manifestation payment.
- Impale Mind has a damage handler but no additional timed-affect case.
- Planar Travel's creation case is excluded in Dragonlance/Forgotten Realms;
  outside those campaigns its random-room loop is not safely bounded when no
  valid destination exists.
- Gravity Well's command halves current movement points and adds a Trip-ID
  marker; it sets neither a speed penalty nor a no-flee flag. Its marker lasts
  30 rounds despite a comment describing three rounds.

Shared effects are accounted for: Oak Body and Body of Iron halve movement speed,
body forms use the actual supported immunity checks, Ubiquitous Vision adds
Perception and prevents flanking, Inflict Pain penalizes general skills as well
as attacks, and Catfall follows both supported falling paths. Energy Conversion
storage and its discharge command are described separately from resistance.
Cosmic Awareness consumes its stored buff through its activation command; natural
1/20 d20 results return before consuming the activated bonus.

Generic psionic damage includes the live global damage bonus, omitted when zero.
Delayed Concussive Onslaught, retaliation, and discharge paths do not advertise a
preserved original casting-class bonus. Energy Push notes that collision damage
can consume casting-class state before its main hit. The three damaging epic
powers explicitly describe the shared 150% epic multiplier. Relevant AC buffs
show the configured cap, and Ectoplasmic Shambler shows live summon scaling.
The generic damage path also handles focus, elemental feats, and defenses; the
base formulas are not a promise of final character-specific damage.

Validation uses the actual production psiono/spello initializers, all 87 literal
registrations, and epic flags. Sanitizer tests render every definition, reject
boundary markers, exercise all new names, verify augmentation and configuration
reporting, check cyan labels and aligned wrapping, and compare registration,
psionic metadata, config, player, and resource state before/after rendering.
The audit verifies all 105 IDs and source paths. The complete CMake build passes.
No gameplay changes, deployment, or server restart are included.
