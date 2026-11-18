# Epic feat expansion: second batch

Implemented from the approved catalogue, following the first 20 feats. This batch adds 16 feats: 15 new stable IDs (1348–1362) and the existing Great Smiting ID (306). Former sentinel 1347 stays reserved. No extra feat points are granted.

| Group | Feats |
| --- | --- |
| Psionicist | P06 Epic Resistance Rebuttal; P09 Shared Psionic Mantle |
| Artificer | A05 Epic Charge-Fed Repair; A07 Epic Golem Aegis; A08 Epic Constructbane Infusion |
| Summoner | S03 Epic Resilient Bond; S04 Epic Spell Sharing; S07 Epic Conjuration Command; S09 Epic Planar Triage; S10 Epic Evolved Summons |
| General | G06 Epic Trailmaster |
| Other classes | O01 Epic Inspiration; O03 Positive Energy Bulwark; O04 Great Smiting; O07 Legendary Companion: Elemental Bond; O08 Epic Augmented Alchemy |

All require total level 21 and the catalogue's permanent attribute, trained skill and class prerequisites. They use one eligible class or general epic point. Study displays full mechanics before accepting or rejecting. Evolved Summons chooses Aquatic or Watchful; Elemental Bond chooses acid, cold, electricity or fire. Inspiration allows two ranks, with Bard 23 and total level 27 required for the second. Great Smiting now permits only one effective rank, including old saved ranks.

## Commands

| Command | Action and payment | Recovery / duration |
| --- | --- | --- |
| `epic psi-rebuttal` | Swift; 5 PSP | 60s / 18s arm |
| `epic psi-mantle <ally> <power>` | Standard; unaugmented base PSP +8; inertial armor, biofeedback or vigor | 60s / at most 60s |
| `epic charge-repair <golem> <wand>` | Standard; 3 charges from a carried ordinary circle 1–3 wand | 300s / immediate repair, min(60, 10 + twice Artificer level) before penalties |
| `epic golem-aegis` | Standard; directly deduct 30 HP from own golem with at least 31 HP | 120s / 30 temporary HP for 18s |
| `epic constructbane <wielded weapon>` | Standard | 60s / 18s infusion; first eligible hit per 6s adds 2d6 physical |
| `epic shared-cast <spell>` | Native casting action/time; Summoner slot one circle higher, final circle ≤6 | 60s / native spell duration |
| `epic conjuration-command <summon>` | Swift; own normal conjured creature | 30s / next ordinary attack within 12s: +4 morale to hit, +8 damage after critical multiplication |
| `epic planar-triage` | Standard; sixth-circle Summoner slot | 300s / primary eidolon heals min(100, max HP / 5), minimum 1 before native healing pipeline |
| `epic positive-bulwark` | Standard; native Channel Energy use | 300s / 2d6 holy to hostile undead at 0, 6, 12s, native Will half against snapshotted DC |
| `epic great-smite` | Swift; next manual smite still pays its native use/action | 300s / 18s arm; one eligible hit adds supplying class level as physical damage |
| `epic augment-bomb <slot> <sacrifice>` | Swift; two distinct normal prepared fire bombs, sacrificial slot consumed immediately | 60s / 18s arm; bound manual direct hit adds 4d6 fire with the original save/evasion outcome |

Slots are numbered as in the native bomb menu. Passive Resilient Bond spends its owner's 300s recovery before its DC 20 Will save, after failed native Life Link on lethal damage. Elemental Bond adds a separate 1d6 energy packet on the primary animal companion's first ordinary melee hit per six seconds. Inspiration changes only the four approved native song contributions.

## Native adaptations and persistence

- Native Vigor raises maximum HP and current HP. Shared Vigor instead supplies a finite five-point absorb pool. It and Aegis compete with existing true legendary temporary HP: stronger pools replace weaker pools, never add. Ordinary healing does not refill these pools. Aegis ends immediately when owner or golem leaves the room, and validates ownership/identity before absorbing damage.
- Charge-Fed Repair is a construct repair, so it bypasses the golem's penalty for healing spells and Empowered Healing. Native Blackmantle and feeding adjustments remain. It does not cast the wand spell.
- Shared casting runs through the native casting path and charges a higher Summoner slot. Its second target is resolved directly through the affect routine, without another cast or cast-trigger chain. Both targets are checked before starting; losing the primary eidolon before completion aborts the shared cast. Native Resist Energy currently protects against all energies, with no selectable element. No new element selector is introduced.
- Native tracking currently uses its existing target lookup and has no recorded room trails or classified terrain penalties. Trailmaster supplies its +6 competence check bonus without extending range, revealing exits or adding a new locator. There is currently no terrain penalty for its conditional waiver to reduce.
- Cooldowns, arms, selected doctrines/elements and bomb sacrifice are saved in explicit player-file fields. Bomb sacrifice and arm share one player save. Native spell slots and channel use retain native recovery. Respec clears temporary arms, preserves cooldowns and owner proc recovery, and grants no resource refill.
- Pet rows gain `epic_doctrine`, `epic_expiry` and `epic_identity`. Only new normal summon-creature spells assign doctrine provenance. Restore retains the absolute original expiry and stable identity; it cannot renew a summon. Existing rows default to no doctrine. Mirrors and purchased/charmed pets cannot gain a doctrine from restoration.
- Ordinary infused weapons use their own `EpID` object tag, separate from legendary identity/registration. Unwielding clears the infusion; serialization does not. Owner proc recovery cannot be reset by swapping weapons.
- Secondary feat packets use the no-trigger bonus origin. They retain damage reduction/resistance and death resolution, avoid another attack/concealment/image roll, and do not trigger offensive damage, item or spell chains. Nested attacks and attacks of opportunity receive no new ordinary-attack riders.
- Bulwark rechecks room legality and hostile undead each pulse. Its DC is captured once, using the higher qualifying class level. Offline pulses are skipped, never replayed. Death, unconsciousness or expiry cancels the aura.

## Validation and deployment

The complete CMake build and the focused epic component tests pass, including address/undefined-behavior sanitizer coverage. Tests exercise prerequisites and rank gates, positive-resistance-only consumption, finite shields, room/ownership loss, doctrine provenance/expiry, single-attack command consumption, failed/successful Resilient Bond saves and recovery, bomb binding and serialization-safe infusion removal. Earlier batch tests remain included.

The help bundle includes all 36 feat entries, and individual feat help resolves to the registered mechanics. No help database import or live database migration was run. The pet columns are added by the existing core-table upgrade at server startup. No running server was restarted; room/combat command smoke tests against a running game remain a deployment check.
