# Religion and church progression proposal

**Status:** Phases 1, 2 and 3 implemented; see the [phase 1 guide](RELIGION_PHASE1.md) and [phase 2 guide](RELIGION_PHASE2.md). Two authored pilot churches are staged for test-server installation; live rollout/playtesting remains a deployment step. See the [phase 3 guide](RELIGION_PHASE3.md) for appearances, blessings and the optional content extension. The expanded privilege catalogue and privilege-choice system are deferred. The separate vestment appearance catalogue remains in scope.

## Recommended shape

Let characters formally devote themselves to one deity, receive a few immediate
benefits, and earn permanent **Church Standing** through service. Standing unlocks
church ranks, temple access, deity-specific gear appearances, conveniences, and
one cumulative 24-hour blessing with a different small benefit at each rank.

Keep the first version small:

- One deity per character, using the existing deity selection.
- One progression number: Church Standing. It is earned, never spent.
- Eight ranks, with deity-specific church rank names; player-customizable titles are not rewards.
- One active church task at a time, using the existing quest framework.
- Slow advancement: two Standing per task, at most six per week.
- Deity and temple rank settings for mobs, with faith and rank directional blocker flags.
- Temples and clergy built by you in game, connected through simple builder settings.
- A private `religion` channel for members of the same faith.
- One cumulative church blessing lasting 24 real hours; each rank adds a different modest benefit.
- No additional spell circles, perk points, or mandatory daily upkeep.

The appeal should be **belonging, access, choice, and participation**, supported
by a modest amount of power progression. A veteran gains a broader wardrobe,
more church opportunities, and a richer cumulative blessing, without accumulating
a permanent stack of combat bonuses.

## What already exists

The code already stores a character's deity through `GET_DEITY`, and has deity
names, portfolios, symbols, descriptive material, favored weapons and domains.
It also has quest systems, communication channels, and directional blocker mobs.
There is currently a dedicated access-check pattern for clan blockers that can
inform a separate faith access check.

Use the existing deity IDs and descriptions rather than maintaining a second
pantheon. Church membership should remain independent of clan membership.
The existing deity definition includes descriptive worshipper-alignment text;
that text should not be parsed as a programmatic eligibility rule.

Code references: [deities](../src/deities.h), [character deity](../src/utils.h),
[blocker movement checks](../src/movement.c), [quests](../src/quest.h), and
[communication commands](../src/act.comm.c).

## 1. Joining a faith

### Proposed player flow

1. `religion list` shows a compact list of supported gods.
2. `religion info <deity>` shows the deity's portfolio, expectations, rank names,
   benefits, known temples, and any actual membership restrictions.
3. At an appropriate temple priest, `religion devote <deity>` presents those
   benefits and the commitment rules before asking for confirmation.
4. On confirmation, the character becomes a **Devotee** with zero Standing.

All classes can participate. A warrior, rogue, or bard should have as much reason
to belong to a church as a divine caster. Church rank does not grant Cleric,
Paladin, Inquisitor, or other class abilities.

### Existing worshippers

Treat an existing non-None deity choice as membership at Devotee rank, with zero
Standing. Existing Cleric domains and class choices continue to work. Characters
with no deity remain unaffiliated until they devote themselves.

Before implementation, audit every existing deity-selection path so character
creation, respec, staff edits, and the new command cannot create conflicting
membership records or bypass devotion rules.

### Alignment and class compatibility

For the first version, avoid a second alignment-policing system. Retain any
existing enforced deity/class eligibility rules. Put additional expectations in
the deity's description for roleplay; do not silently remove Standing because
an alignment number changes.

Divine classes should not be able to change patron through this command if that
would invalidate existing class or domain choices. Explain the incompatibility
and direct them to the existing supported change process. Church progression
must not become an alternative way to obtain another deity's domains.

## 2. Benefits for devoting yourself

Every church should offer the same useful baseline, with different flavor.
This prevents choosing a god primarily because its reward package is stronger.

| Immediate benefit | Suggested behavior |
| --- | --- |
| Faith identity | Show deity, Standing and earned church rank in `religion` status. This is a membership record, not a reward that edits the player's customizable title. |
| Religion channel | Access to communication with the same faith. |
| Temple hospitality | Once per 24 hours, an affiliated temple's service NPC provides one random food item and one random drink, each with an existing consumable buff. Effects apply on consumption, not on claiming. Hunger and thirst are unchanged. |
| Rank vestments and holy symbol | Restring eligible currently worn items using the deity-, rank-, armor-category- and slot-specific catalogue described below. Cosmetic only. |
| Church blessing | Access the first blessing at Devotee rank; one self-only blessing lasts 24 real hours. Each rank adds a different benefit and strengthens earlier bonuses on a capped schedule. |
| Church tasks | Access to available service work and Standing progression. |

Hospitality is claimed on site and produces ordinary transferable consumables.
Its cooldown belongs to the character and is shared across all temples and
faith changes. The cooldown and both rewards commit together in the estate
delivery journal; the character must have capacity for both provisions. Devotion offers one explicitly chosen temporary blessing; it does
not automatically apply permanent stat increases.

A church without a completed temple can still provide identity and the channel,
but should be marked **temple services not yet available** before devotion.
Do not require membership in an unfinished church to unlock basic character play.

## 3. Church hierarchy

### Eight earned ranks, earned slowly

Standing never decays through inactivity. Rank follows Standing automatically;
there is no separate promotion fee or approval queue for ordinary advancement.
Every task awards **2 Standing**, with at most **three rewarded completions per
week**. The resulting cap is **6 Standing per week across all churches**.

| Rank | Standing | Example generic rank name | Earliest at weekly cap | Main unlocks |
| --- | ---: | --- | --- | --- |
| 1 | 0 | Devotee | On devotion | Membership, hospitality, rank-one vestments/symbols, first blessing and public tasks. |
| 2 | 12 | Faithful | 2 rewarded weeks | Members' social areas and second blessing benefit. |
| 3 | 36 | Acolyte | 6 rewarded weeks | Rank-three appearances and church lore. |
| 4 | 72 | Votary | 12 rewarded weeks | Rank-four appearances and fourth blessing benefit. |
| 5 | 120 | Steward | 20 rewarded weeks | Inner social areas and senior service stories. |
| 6 | 180 | Preceptor | 30 rewarded weeks | Rank-six appearances and sixth blessing benefit. |
| 7 | 252 | Elder | 42 rewarded weeks | Rank-seven appearances and eligibility for optional appointed offices. |
| 8 | 336 | Luminary | 56 rewarded weeks | Inner sanctuary access, rank-eight appearances and the eighth cumulative blessing benefit. |

These are cumulative thresholds. Each later promotion takes longer than the
previous one. The final rank requires 168 rewarded task completions and at least
56 weekly reward allowances—roughly thirteen months at the cap. Someone averaging
one task per week would take 168 weeks; two per week would take 84 weeks.

The timetable measures rewarded weeks, not an exact minimum number of elapsed
days: a character who starts shortly before a weekly reset can use two weekly
allowances close together. Joining must not grant retroactive weekly allowances.

The intention is a long-term association with a church, not a short leveling
track. Early participation is useful immediately, and the second rank arrives
after two fully rewarded weeks. Later ranks mainly add identity, content access,
and broader temple access. The blessing gains one different small component at every rank, but remains
one package with a shared expiry and bounded rank-based increases to earlier components.

Stop Standing at **336** in this version. Do not create an endless post-Luminary
ladder, rank decay, promotion failure rolls, or an upkeep requirement. No bonus
Standing from donations, rank, account purchases, festivals, or rare task drops.
Special events may replace a normal rewarded task, but never bypass the weekly cap.

Titles can differ by deity, but access checks use the eight underlying rank
numbers. These ranks describe service within the church. They do not make a lay
warrior into an ordained Cleric or establish authority over another player's class.

### Optional appointed offices — later, if players want them

Add offices only after the earned-rank system is working:

- **Temple Keeper:** helps organize gatherings at a particular temple.
- **Faith Representative:** helps coordinate church events and introductions.

Staff appoint and remove these offices. Office responsibilities are separate from earned
rank, confer no additional combat or service benefits, and cannot be purchased
with Standing. Start with no elections, player-controlled treasury, tax system,
or power to expel or silence other worshippers.

The eight earned ranks already provide a usable hierarchy; appointed offices
are optional roleplay support, not a prerequisite for launch.

## 4. Rank rewards and appearances

The expanded privilege catalogue and its selection, replacement, cooldown and
access systems are deferred. They are not dependencies of this implementation.
Membership, rank-based temple access, appearances, church tasks and the cumulative
blessing remain in scope.

### Rank vestments and holy symbols: a separate automatic unlock

Every earned rank unlocks its own appearance collection for the character's
specific deity. These appearances are **automatic rank benefits**, not free equipment or a character-title reward.

The catalogue has four armor categories:

- Clothing
- Light armor
- Medium armor
- Heavy armor

For **each deity, rank and armor category**, author one appearance entry for each
of these seven worn locations:

| Location | Item represented |
| --- | --- |
| Neck | Holy symbol worn as a neck item |
| Body | Robe, tunic, cuirass, coat, or other appropriate torso garment/armor |
| Arms | Sleeves, bracers, armguards, or pauldrons as appropriate to that slot |
| Legs | Trousers, leggings, greaves, or other appropriate leg covering |
| Head | Hood, circlet, cap, helm, or other appropriate headwear |
| Hands | Gloves, handwraps, or gauntlets |
| Feet | Slippers, shoes, boots, or armored footwear |

That is **28 appearance entries per rank**, **224 per deity**, and **224 × the
number of supported deities** overall. For illustration, 21 supported deities
would require 4,704 entries. This counts appearance records, not individual text
fields: a record may contain a short description, room description and an
inspection description if the restring system supports those fields.

The holy symbol is included in every armor-category collection, even though it
is always worn at the neck. Its chain, cord, mounting, or other presentation can
match the collection. Do not silently collapse the four neck entries into a
single generic symbol or reduce the requested content count.

#### Deity identity and rank progression

Every collection must read as belonging to **that god**, using the existing
pantheon's symbols, colors, materials, imagery and religious character. Rank
changes should be visually meaningful: humble practical pieces at early ranks,
then increasingly elaborate craftsmanship and religious detail. More elaborate
does not mean better armor statistics or a claim that the item is an actual
powerful relic.

Use shared authoring templates and word limits to help produce the large
catalogue, but review the actual final descriptions per deity/rank/category/slot.
Changing only a god's name in otherwise identical generic text is insufficient.
Keep descriptions suitable for different character bodies; avoid assigning a
player an unchosen identity or writing a character title into item text.

A builder/editor view should show an eight-rank × four-category × seven-slot
completion checklist for each deity, with previews and missing entries clearly
marked. This checklist is for builders. Player appearance menus omit any rank
or wear slot with no eligible published entry; do not show empty placeholders
or invent a fallback. Missing appearances do not block rank advancement.
Store entries under stable deity and rank IDs plus category and slot,
not by parsing item names. This is a content catalogue, not thousands of new
object prototypes.

#### Shared menu editor and item restring service

Use the [Shared Restring Editor proposal](SHARED_RESTRING_EDITOR_PROPOSAL.md)
for both clans and religion. Its top menu is `1) Clans`, `2) Religion`; each
category lists its supported reward ranks, and each rank lists existing entries
with add/edit/delete actions. Entries belong to a specific clan or deity.

On creation, choose wear position, then clothing/light/medium/heavy armor,
shield subtype, or weapon family as appropriate. The deity-specific eight-rank,
four-category, seven-slot wardrobe remains the initial religion content target;
the shared catalogue can also support shield and weapon appearances.

Eligible players use `restringapply <item> religion <rank>` or
`restringapply <item> clan <rank>` at the appropriate service NPC. It matches the
owned carried/worn item's position and category, checks membership and earned
Standing/Favor, previews the exact text and gold fee, and asks for confirmation.
Multiple valid styles receive a selection menu; no match causes no change.

The recommended price is **100 gold × max(1, item level)**, charged only after a
successful persisted application. No Standing or Favor is spent.
The selected item keeps its identity and statistics, changes appearance, and
**loses all existing extra descriptions**, with that clearing surviving reload.
Other items and the object prototype remain untouched. See the shared proposal
for exact matching, editor menus, preview, failure handling and save rules.

Appearance eligibility is checked when the restring is performed. Applying a
religion description also stamps its deity and minimum temple rank on the item;
a clan description stamps its clan and minimum Clan Favor rank. Existing clan
officer rank requirements remain separate. Purchases and wear require matching
membership and at least the stamped rank, and both categories apply when present.
Appearance and requirements survive transfer and membership changes. The text
grants no membership, rank, blessing or access rights. Further use of the service
requires current eligibility.

Audit the native restring and object-save paths before implementing this
interface; the new service should reuse instance customization rather than
altering the shared object prototype or generating replacement equipment.

### Church blessing: one cumulative package, a different benefit at every rank

At an affiliated temple priest, receive **one self-only church blessing lasting
24 real hours (86,400 seconds)**. The blessing includes **every benefit unlocked
by the character's earned church rank**. Players do not choose individual buffs.

Each rank introduces a different benefit, and earlier benefits grow on the
rank schedule below. The overall blessing gains both breadth and strength.
Individual amounts have explicit caps so the combined package stays bounded.
A Luminary receives all eight benefits together, with one application and expiry.

The blessing is separate from appearance collections. No Standing is spent to receive it. All faiths
share the same mechanics, with deity-specific names, descriptions and ceremony
messages so patron choice remains primarily a matter of identity and roleplay.

#### Proposed cumulative benefits

These are initial balance values for discussion, not implemented mechanics.

| Unlock rank | New benefit | Scaling and maximum | Rules |
| --- | --- | --- | --- |
| 1 — Devotee | Resolve | +1 saves initially, increasing to +4 at rank 7. | Resistance bonus; stronger resistance bonuses take precedence for each save. |
| 2 — Faithful | Shelter | +1 AC initially, increasing to +4 at rank 8. | Deflection bonus; normal stacking and AC caps apply. |
| 3 — Acolyte | Endurance | +15 maximum HP initially, then +5 per rank, capped at +40. | No current HP granted on application or renewal. One named church HP bonus, not temporary HP. |
| 4 — Votary | Sacred Magic | +5% spell potency and +5% spell duration initially; capped at +10% potency and +15% duration at rank 8. | Uses the native spell-potency and spell-duration calculations; no spell DC, caster-level, skill-rank or prerequisite bonus. Both effects form the rank-four component of the same blessing. |
| 5 — Steward | Guided Hand | +1 weapon attack initially, increasing to +2 at rank 7. | Morale bonus for melee, projectile and thrown weapons; not spells or rays. Stronger morale bonuses and the normal attack cap apply. |
| 6 — Preceptor | Warding | DR 1/– initially, increasing by one per rank, capped at DR 3/–. | Normal DR comparison; no addition to a stronger DR source, energy immunity or spell resistance. |
| 7 — Elder | Pilgrim's Stamina | +50 maximum movement initially, increasing to +75 at rank 8. | No movement restored on application or renewal. Does not increase speed, actions or weapon range. |
| 8 — Luminary | Readiness | +2 initiative. | Final-rank unlock arrives at its cap; no further rank exists to scale it. One named church initiative bonus; normal initiative rules apply. |

#### Exact package at each rank

All listed numbers are bonuses. A dash means that component is not unlocked.
The row for the application rank determines the entire 24-hour blessing.

| Rank | All saves | AC | Max HP | Spell potency / duration | Weapon attack | DR | Max movement | Initiative |
| --- | ---: | ---: | ---: | ---: | ---: | --- | ---: | ---: |
| 1 — Devotee | +1 | — | — | — | — | — | — | — |
| 2 — Faithful | +1 | +1 | — | — | — | — | — | — |
| 3 — Acolyte | +2 | +1 | +15 | — | — | — | — | — |
| 4 — Votary | +2 | +2 | +20 | +5% / +5% | — | — | — | — |
| 5 — Steward | +3 | +2 | +25 | +5% / +5% | +1 | — | — | — |
| 6 — Preceptor | +3 | +3 | +30 | +7% / +10% | +1 | 1/– | — | — |
| 7 — Elder | +4 | +3 | +35 | +7% / +10% | +2 | 2/– | +50 | — |
| 8 — Luminary | +4 | +4 | +40 | +10% / +15% | +2 | 3/– | +75 | +2 |

**Example:** a Votary receives +2 saves, +2 AC, +20 maximum HP, +5% spell
potency and +5% spell duration as one blessing. A Luminary receives +4 saves,
+4 AC, +40 maximum HP, +10% spell potency, +15% spell duration, +2 weapon
attack, DR 3/–, +75 maximum movement and +2 initiative.
These are cumulative unlocked components at the row's values, not a sum of
all previous rows: a Luminary's save bonus is +4, not +20.

**Sacred Magic calculation:** add the listed potency percentage points once to
its native potency modifier, affecting only the spell outcomes that already use
that calculation. It does not independently add damage dice, saving-throw DC,
spell penetration or caster levels. Integrate duration once into the existing
spell-duration bonus calculation, with ordinary engine rounding and minimums;
only positive finite spell durations qualify. Neither effect retroactively
changes spells already cast. Do not extend cooldowns, charge counts, permanent
effects, item buffs or the church blessing's own fixed 86,400-second expiry.
The two bonuses are one combined rank unlock, not separate blessing selections.

Because equipment and spells can supersede typed bonuses, show both the package
and its **effective benefit with current equipment and buffs** where practical.
For example, a Luminary's +4 resistance saves do not add another +4 to an
existing +5 resistance bonus. Do not replace superseded bonuses with extra
untyped bonuses to guarantee an increase.

Eight scaling benefits have a meaningful combined value. Trial the complete
Luminary package, not only each component in isolation. Keep the listed caps;
no extra effect ranks, bonus Standing conversions, or post-Luminary scaling.

#### Application, renewal and persistence

- `religion blessing` shows the cumulative package for the current rank,
  locked future benefits, active application rank, remaining real-time duration
  and next availability.
- `religion bless` at an affiliated priest previews the **whole package** and
  expiry, then asks for confirmation. Use it outside combat while able to act.
  There is no buff-name argument, individual selection or per-component charge.
- Recommended simple rule: one application per 24 real hours, shared across
  temples and faiths. After expiry, a new application bestows the full package
  for the character's current rank for another 24 hours.
- Only one church-blessing package can exist on a character. Repeated requests,
  other priests and other deities cannot add a second copy or independently
  refresh one component. Every component shares one expiry.
- Voluntary removal or a supported dispel removes the entire package and does
  not reset application availability. Failed applications consume no cooldown.
  Do not add a separate prayer currency or daily charge pool.
- Logout does not pause the clock. Persist an absolute UTC expiry, then remove
  expired effects before calculating login stats. Reconnection never grants
  another 24 hours or separately restores individual components.
- Recommended death policy: retain the package only until its original expiry;
  death neither refreshes it nor grants a new application. Implement that policy
  consistently with the native death/affect lifecycle and document it in help.
- Rank-up adds its new component and increases earlier values on the **next application**, not by extending
  or reapplying the current package. Status shows the next application's upgrade.
- Leaving or changing deity removes the whole old-faith package immediately.
  The shared application cooldown remains; returning cannot duplicate benefits.
- HP and movement bonuses change maximums only. Application, renewal and rank-up
  do not refill either resource. Expiration/removal clamps current values to
  their new maximums without killing the character solely because the HP maximum
  changed. Never heal, refill temporary HP or restore movement by cycling buffs.
- Persist source deity, application rank and one expiry under a dedicated church
  blessing identity. Internal affects may be needed for different stats, but
  they must behave as one package for display, removal, dispelling, expiry and
  save/load. Generic affect refresh must not overwrite the real-time expiry.
- Respec or staff rank correction must never leave benefits above the eligible
  rank: trim unavailable components without extending duration. Faith changes
  remove the package, rather than transferring another deity's active blessing.

This supplies a modest amount of power progression alongside the slower church
hierarchy. Higher rank means **one richer 24-hour blessing**, while appearance
collections and temple access continue to supply the horizontal rewards.

### Other rewards worth adding later

- Additional seasonal variants of the deity/rank vestment collections and ceremony emotes.
- Access to church lore, sermons, and stories you place in temple rooms.
- Optional pilgrimage routes and records of discovered affiliated shrines.
- Invitations to church events or alternate quest dialogue.
- Different story solutions to the same quest, with equivalent mechanical value.

Keep these as content additions. Avoid introducing a collectible currency or
separate progress meter for each reward.

## 5. Tasks for the church

### Simple weekly service board

Each church offers up to four distinct service task choices per calendar week.
Every task is worth **2 Standing**. A character may earn Standing from at most
**three** completions per week, for a maximum of **6 Standing per week across all
churches**. Four offerings give players a choice; they do not provide four rewards.

- One active church task per character, separate from unrelated ordinary quests.
- The player chooses from the visible weekly board; accepting a task previews
  the objective, destination, return priest, requirements, and reward.
- Weekly offerings are stable. Abandoning a task does not reroll the board.
- An active task remains completable after the week changes. Its completion
  counts toward the current week's cap; the same task template cannot award
  Standing twice in that completion week.
- Refresh at one documented weekly boundary, recommended Monday 00:00 UTC.
  Show the next reset in status. No daily task reset is needed.
- After three rewarded completions, clearly mark remaining tasks as story-only
  before acceptance; they grant neither Standing nor a later banked reward.
- At Luminary, tasks can still be played for story participation, but grant no
  additional progression or repeatable currency.
- Unused weekly allowances do not accumulate. Progress already earned is never
  lost, and there are no streaks, daily requirements or catch-up multipliers.

Three rewarded completions per week is a cap, not an expectation of daily play.
Where possible, provide short errands and longer adventure alternatives so the
system accommodates different play sessions.

### Task types for the first release

Reuse authored quest objectives and turn-ins, with a small church reward hook.
Do not create a parallel universal objective engine or automatically reward
all healing, crafting, or killing everywhere in the game.

| Type | Example | How completion is verified |
| --- | --- | --- |
| Delivery | Bring sealed correspondence to another affiliated priest. | The assigned quest recognizes the intended recipient and advances normally. |
| Recovery | Retrieve a stolen temple object from a particular location. | The assigned quest verifies and consumes its intended quest item. |
| Pilgrimage | Visit an authored shrine and report back. | Quest visit credit is recorded for that character, not inferred from a traded item. |
| Church service | Help with a ceremony, infirmary, archive, or workshop. | An authored interaction or quest sequence completes; generic command spam does not count. |
| Adventure | Defeat an assigned threat or recover evidence from its area. | Existing quest credit rules verify the objective; the church accepts the completed quest. |

Church rewards add Standing only. Existing quests can retain intentional normal
rewards, but the repeatable service board should not become a new source of XP,
perk points, gold, resurrection discounts, or valuable sellable items.

### Theme examples for Krynn

These are suggestions for stories you author, not new mandatory deity rules.

- **Mishakal:** deliver medical supplies, prepare an infirmary, recover healing texts.
- **Paladine / Kiri-Jolith:** escort correspondence, investigate a threat, honor an oath.
- **Gilean:** recover a manuscript, verify an inscription, deliver archival records.
- **Reorx:** obtain workshop materials, restore a ceremonial object, visit a forge shrine.
- **Chislev:** investigate blight, assist an injured animal through an authored quest, protect a sacred grove.
- **Shinare:** settle an authored trade dispute, recover a ledger, deliver contract documents.
- **Takhisis / Sargonnas:** deliver orders, reclaim an order's property, complete an authored trial of resolve.
- **Hiddukel:** recover incriminating records or carry out an authored intrigue; avoid enabling arbitrary theft from other players.

A weekly offering should be possible at its stated difficulty. Do not require
random player availability, player deaths, real-time staff approval, or a rare
auction purchase for ordinary rank advancement.

## 6. Temples you build in game

### Keep buildings separate from the church record

A deity can have several temples, all sharing the same membership and Standing.
A small shrine can provide only hospitality, while a larger temple has a priest,
service board, archive and inner rooms. No automated temple construction or
resource upkeep is needed.

### Minimal builder configuration

| Entity | Proposed settings |
| --- | --- |
| Temple registration | Stable temple ID, deity ID, display name, public entrance room, priest mob vnum, enabled service options. |
| Priest | Link to one registered temple; invokes devotion, tasks and configured services. |
| Task | Existing quest ID, deity ID, minimum rank, weekly-board eligibility, enabled/disabled. Standing reward is fixed at two. |
| Mob faith | Deity ID and temple rank (0 for none, 1–8 for a church rank), independently editable on any mob. |
| Faith blocker | Mob deity and temple rank, `BLOCK_RELIGION` and/or `BLOCK_TEMPLE_RANK`, and existing blocked-direction flags. |

A menu should show deity names and rank labels, while persistence uses IDs.
Do not encode deity affiliation in mob names, infer temples from room names, or
make every temple a separate player organization.

One priest can handle the whole initial interaction. Decorative clergy and
additional service NPCs can be added later without changing membership rules.

### Purchase requirements

Pet shops check the pet's deity and temple rank before taking payment. When both
are set, the buyer must actively follow that deity and have that rank or higher.
This uses the mob's metadata without requiring blocker flags. The display mob
and the delivered prototype must both allow the purchase.

Objects have the same deity/minimum-temple-rank purchase settings in OEDIT.
Both must be nonzero to activate the requirement. Checks apply to NPC shops,
player stalls and auctions before payment; clan requirements still apply.
Pet-token objects also check the resulting pet prototype. Requirements survive
object saves and resale, including sales inside bags or sheaths. Religion
requirements also gate wear, wield and hold through both player commands and the
central equipment path; they do not prevent carrying an item. The shared restring
service remains planned and must stamp the selected entry's requirements when
implemented.

### Mob faith and blocker behavior

Expose a deity and temple rank on mob prototypes in MEDIT, save and load both,
and copy them to spawned mobs. A mob may have a deity and rank without guarding
anything; these fields describe the mob as well as supplying guard requirements.
Use the same deity IDs and rank numbers as player churches: 1 is Devotee and 8
is Luminary. Rank 0 means no temple rank; deity None means unaffiliated. Do not
use player Standing to calculate an NPC's builder-assigned rank. Display both
fields and flag names in the builder and mob inspection views.

Add independent mob flags `BLOCK_RELIGION` and `BLOCK_TEMPLE_RANK`. They apply
only to directions marked by the existing `MOB_BLOCK_*` direction flags:

| Flags | Who may pass the guarded direction |
| --- | --- |
| Neither | Existing directional blocker behavior applies unchanged. |
| `BLOCK_RELIGION` | A character whose active deity matches the mob's deity, at any rank. |
| `BLOCK_TEMPLE_RANK` | A character of the mob's deity whose rank is at least the mob's temple rank. Rank comparisons only make sense within the same faith. |
| Both | The character must match the mob's deity and meet the mob's rank. |

For rank checks, calculate player rank from Standing for the currently active
deity, including the existing-worshipper Devotee migration. Apply the existing
movement policy for NPC movers and staff bypass; do not silently promise that
NPCs are blocked when the current movement code only blocks players. The
faith flags impose requirements, not exemptions from other configured guards.
If a guard also has `MOB_BLOCK_CLAN`, both the clan and faith requirements must
pass. Refactor the current clan branch, whose successful check exits early, so
it cannot skip a faith check; preserve existing behavior for clan-only guards.
Other exemption flags, Ghost, and stealth cannot bypass a faith requirement.

Reject enabling either faith flag without a valid non-None mob deity, and reject
`BLOCK_TEMPLE_RANK` without a rank from 1 through 8. Permit rank 0 on ordinary
non-rank-blocking mobs. Validate on save and load; malformed legacy data must
fail closed for the guarded direction, log a builder-facing error, and leave
outward exits available. Old mobs default to deity None, rank 0, and neither
flag, preserving their existing behavior. Do not infer a mob's identity from its
name or from a temple registration. Test that prototype save, reload, editing,
and spawning preserve both fields and flags.

Example denial: `The temple guardian admits only Stewards of Reorx or higher.`
Keep public worship areas open, do not attack visitors merely for failing a
check, and provide outward exits. Directional blockers are not a security
boundary against teleportation, portals, summons or scripted movement; build
restricted rooms with existing restrictions on those routes and test them.

## 7. Religion channel

Proposed syntax:

- `religion say <message>` — speak to online members of your deity.
- `religion channel on|off` — personal listening toggle.
- `religion who` — list visible online worshippers who allow channel participation.

Every rank has equal access; higher ranks do not gain louder speech or moderation
powers. Display the faith name and the sender's ordinary permitted identity.
Respect existing silence, communication restrictions, ignore rules, visibility,
and staff monitoring policies. This is a normal player channel, not a divine
ability that works around restrictions.

No cross-faith channel or inter-church diplomacy system is needed initially.
For secretive faiths, channel membership must not appear in public `who` output.
`religion who` must not reveal hidden staff or bypass normal visibility rules.

To keep version one simple, omit message history. If history is added later,
partition it by deity and never expose a previous faith's backlog after a change.

## 8. Leaving or changing faith

Avoid using heavy punishment to keep players in a church they no longer enjoy.

- `religion leave` previews lost access, active blessing, services, and channel membership,
  then asks for confirmation.
- Leaving immediately ends membership benefits and clears active church tasks.
  It does not change class ranks, remove ordinary items, undo completed cosmetic
  restrings, edit the player's title, or delete character XP.
- Store previously earned Standing for each deity, so returning does not require
  repeating the whole progression.
- A new devotion after leaving or changing is allowed once every seven days.
  The first devotion has no cooldown. Show the rule before departure.
- Weekly Standing limits, completed-task records and hospitality cooldowns are character-wide and survive faith changes.
- Only the currently devoted deity grants benefits. Old ranks cannot open doors
  or provide services while the character follows someone else.
- Switching never grants class domains or makes an incompatible divine build valid.

This requires a small Standing history per character/deity, not a second active
faith slot. No automatic apostate enemies, divine curses, or reputation wars in
the initial release.

## 9. Minimal persistence and integration

A practical implementation needs only a few records:

- Existing character deity field remains the authoritative active affiliation.
- Per-character/per-deity Standing; rank is derived from Standing.
- A shared clan/religion rank-and-item-category appearance catalogue with
  supported restring text fields, publication states and revisions.
- One active cumulative church blessing, source deity, application rank, absolute
  expiry and shared next-application time per character.
- Per-character faith-change and service cooldowns, weekly completions/cap, and
  one active church task reference through the quest system.
- Small temple and task configuration tables or equivalent supported OLC data.
- One saved channel preference; rank is derived from Standing rather than stored twice.

When an eligible church quest completes within the shared weekly cap, award its
two Standing once using a unique
character/task/week completion record and an atomic update. Disconnects, repeated
turn-ins or failed saves must not duplicate rewards. Failed devotion or service
requests leave the old state intact and consume no cooldown.

Validate quest ownership and turn-ins using the quest system's rules. Purchasing
an ordinary shop item, trading a proof item between alts, or repeatedly accepting
and abandoning a task should not independently create Standing.

Use deity and character IDs, not names, as keys. Unknown or removed deity/temple
references should disable the affected church service safely, without breaking
existing class spell access or stranding the character.

## 10. Implementation plan

Implement in dependency order. Each phase should include migration, builder/player
help, and focused tests before the next is enabled. The privilege catalogue and
choice system are excluded; the appearance catalogue is still required.

### Phase 1: data, membership and mob authoring

- Audit existing deity selection, respec and save paths; keep `GET_DEITY` as the
  authoritative active faith. Add per-deity Standing history, character-wide
  cooldowns, and safe defaults/migration for existing worshippers.
- Add deity/rank display, list/info, devotion and departure previews and
  confirmations; protect existing class/domain eligibility and faith-change rules.
- Add mob deity and temple rank fields, MEDIT controls, persistence, prototype
  copying, validation and inspection. Add the two blocker flags, but enable
  rank-gated areas only after rank and movement checks are ready. The implementation
  includes these checks with the phase 1 flags; authored area rollout remains phase 2.
- Add the faith channel, visibility/privacy and preference handling, and temple
  registration, priest configuration and shared hospitality cooldown.
- Verify old character and mob data load unchanged; exercise reload, faith changes,
  class restrictions, channel visibility, and mob editor save/load/spawn.

### Phase 2: service, hierarchy and access

- Integrate the existing quest framework with temple task configuration, one
  active task, fixed Standing awards, weekly cap and atomic completion records.
- Derive eight ranks from Standing and display deity-specific names without
  changing character titles. Add the two directional faith checks with the exact
  combined-flag and clan interaction rules above.
- Test duplicate turn-ins, failures, weekly reset boundaries, switching faiths,
  and each flag combination with matching, different and unaffiliated players at
  ranks below, equal to and above the mob's rank. Test multiple guards, direction
  flags, clan combinations, stealth/Ghost, staff policy and legacy mobs.
- Pilot two authored churches, verifying restricted rooms against non-walking
  entry and ensuring an outward route before enabling progression widely.

### Phase 3: appearances and blessings

- Implement the shared clan/religion appearance editor and restring service per
  the linked proposal, including rank/slot/category matching, fees, confirmation,
  persistence and extra-description clearing. Author and review the requested
  deity/rank collections; mark incomplete content unavailable.
- Implement the single cumulative 24-hour blessing using the exact rank table,
  native stacking, shared application cooldown and persisted expiry. Test rank
  changes, dispels, death, relog, faith changes and native bonus interactions.
- Add authored temple archives, inner areas and more tasks. Optional appointed
  roleplay offices remain a later addition if players want them.

The player-facing loop is **devote, serve, gain Standing, and unlock rank rewards**.

## 11. Things to leave out initially

- The expanded privilege catalogue and privilege selection system.
- Permanent combat-stat ladders, exclusive best-in-slot equipment, multiple
  simultaneous church blessings, extra spell slots, perk points or new class features.
- Character titles as gated rewards: players can already customize their titles.
- Separate favor, piety, influence and donation currencies.
- Gold donations that buy rank or accelerate the weekly cap.
- Guild-style skill trees, temple budgets, taxes, upkeep and decay.
- Automated punishment for nuanced roleplay or every alignment fluctuation.
- Player-controlled excommunication, treasury access, elections and church wars.
- Daily login streaks or obligations to stay online for ceremonies.

## 12. Decisions to discuss before coding

1. Is the six-Standing weekly cap and 56 rewarded weeks to Luminary the right pace?
2. Are the eight scaling blessing benefits and 24-hour renewal rule appropriate?
3. Which deity should receive the first complete 224-entry appearance catalogue,
   and should completed restrings remain on traded items as proposed?
4. Which two deities should receive the pilot temples and weekly task choices?
5. Should the religion channel be in-character or ordinary faith-community chat?

The mob blocker rules above use the mob's own faith and rank as requirements;
temple ranks increase from 1 to 8, unlike clan ranks.
