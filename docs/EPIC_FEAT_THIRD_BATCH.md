# Epic feat completion — final twelve entries

All 48 catalogue entries are implemented in source, Study and the epic help bundle. This release completes P02, P05, P10, S06, S08, G07, G08, G09, G11, G12, O02 and O06. Each costs one general or eligible class epic feat point. The descriptions below give the approved mechanics; native adaptations and persistence details here take precedence over proposal wording.

## Runtime details and native adaptations

- P02 chooses one of the six native disciplines before Study confirmation. Both ranks retain that choice; rank two requires total level 27. Permanent prerequisite loss suspends the bonus. Only ordinary manifested powers gain +1/+2 save DC; items, epic powers and resistance checks receive none.
- P05: `epic psi-contingency <inertial armor|biofeedback|vigor>`, full-round, base PSP plus 5, 300-second cooldown and arm. Triggers after surviving hostile damage at 25% HP or less; defensive effect lasts at most 60 seconds, using the lesser of armed/current manifester level without augmentation. Native Vigor grants a finite five-point temporary shield, never healing.
- P10: `epic sentinel-mind`, swift, 15 PSP, 300-second cooldown; 60-second arm. One failed qualifying mental save rerolls with +2 insight, using the same original DC and fixed modifiers; consumed on the retry regardless of outcome.
- S06: `epic eidolon-intercession <ally>`, standard, 60-second cooldown, 18-second arm. Primary eidolon directly pays up to 20 HP to absorb that much hostile damage; it must survive payment. No damage/proc redirection.
- S08: `epic adaptive-aspect <old ID> <gills|swim|scent|flight>`, full-round out of combat, 300-second cooldown, 60-second overlay. No arguments lists eligible IDs. Replacement cost cannot exceed the removed aspect; dependencies and duplicate selections are checked. Saved aspect points are unchanged.
- G07: `epic spell-opportunity <enemy> <magic missile|shocking grasp|ray of enfeeblement>`, swift, 60-second cooldown, 18-second arm. Prepays one known eligible ordinary slot. Reacts once after that enemy completes a hostile ordinary cast, including a resisted or missed cast. No reaction chains, metamagic or duplicate manual-cast procs. Reconnecting forfeits this runtime-target reservation without refund; cooldown remains.
- G08: `epic spell-stowaway <ally>`, swift, 300-second cooldown, 18-second arm. Copies newly applied components of the ally's next self-cast blur, resist energy or fly at half resulting duration, rounded down with a one-round minimum. Native Resist Energy protects against all native energy types; there is no element selector. No copied ward bonus or repeated cast proc.
- G09: Tenacious Wards records original source caster level and +4 hostile-dispel defense only on its qualifying manually cast wards. It survives saving; item/proc/copy casts receive no bonus. Self/friendly dispelling is unchanged.
- G11: Epic Dodge is enabled as one rank. First eligible noncritical ordinary weapon/natural hit is avoided after normal avoidance checks, before on-hit effects, once per 30 seconds. Critical hits, spell packets, incapacitation and restraint do not qualify.
- G12: `epic recover` lists eligible condition components; `epic recover <number>` spends a standard action and starts a 300-second cooldown. Retries against the stored original Fortitude/poison DC with +4 circumstance. Removes only selected poison/disease/sickened/fatigued condition bits on success, preserving unrelated modifiers and damage. Older affects without original-save metadata cannot be selected.
- O02: Lasting Inspiration adds at most 18 seconds to native Dance of Protection and Heroism components after voluntary stopping. Forced stop, separation or lost eligibility removes only this extra tail, preserving any remaining native duration. No stacking by repeated toggles.
- O06: Study chooses a permanently owned Weapon Focus type. A qualifying otherwise valid sneak hit has one 50% chance per six seconds to bypass native Essence of Undeath anatomical sneak immunity. Swarms, legendary bosses and no-kill mobs are hard exclusions. Critical immunity by itself is not sneak immunity; fortification, concealment and positioning remain independent checks. The native impromptu/AoO paths now respect this anatomical restriction too.

## P02 balance audit

`get_power_penetrate_mod()` adds +6 for existing Epic Power Penetration and is used by power-resistance checks. No save-DC path reads that feat. Its historical description claimed +6 manifestation DC; that description did not match runtime and has been corrected. This release preserves the existing resistance benefit and does not introduce an additional +6 DC. P02 adds once in the ordinary-power save challenge, alongside native Epic Psionics and Telepathy DC features.

Enumerating all twenty native d20 results, including natural-one failure and natural-twenty success, gives these representative save-success rates with a fixed +15 save modifier. These are deterministic check probabilities, not live encounter measurements; other native rerolls/automatic defenses are excluded.

| Base DC | No P02 | Rank 1 | Rank 2 |
| --- | --- | --- | --- |
| 20 | 80% | 75% | 70% |
| 25 | 55% | 50% | 45% |
| 30 | 30% | 25% | 20% |
| 35 | 5% | 5% | 5% |

The feat reduces save success by at most five percentage points per rank, bounded by native natural-roll rules. It does not change power penetration.

## Persistence and deployment

New IDs preserve previous sentinel values; Epic Dodge reuses its existing ID. The optional player `EpCh` row appends discipline selection after previous choices. Older rows default missing choices to zero. New cooldown/state rows and appended affect provenance preserve cooldowns, finite uses, original source caster level and original save DC. Respec clears arms/choices while retaining spent cooldowns. Legacy affect rows remain readable.

No SQL schema changes, live help imports, live game commands or server restart were performed. Install the updated executable and load the help bundle through the project's normal deployment procedure. Focused component tests cover prerequisites, rank gates, persistence-facing state, paid reactions, finite shields, aspect overlays, ward provenance, condition metadata, precision immunity and performance tails; the full executable is also built.

## Approved mechanics reference


## Player build planner

`epicfeats` lists learnable epic feats in compact columns with IDs and independent colored markers for current-class choices (C), known feats (K), maximum ranks (M), and next-rank eligibility (+). Names are shortened with ~ to fit the screen; IDs open full details. Prerequisites and mechanics are shown only on request with `epicfeats info`. The paged list is available before level 21 and is not restricted to currently eligible feats. Use `epicfeats class <full class name>`, `search <name text>`, `known`, or `available` to filter, and `epicfeats info <name or ID>` for full mechanics, rank gates and suboptions. Availability excludes unspent-point checks; purchases remain in Study.
