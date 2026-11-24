# Religion phase 1

Phase 1 provides membership, saved Standing history and cooldowns, the religion
channel, mob faith/rank authoring, and registered temple hospitality. Task rewards
and deity-specific rank labels are implemented in [phase 2](RELIGION_PHASE2.md).
Blessings and the shared appearance service remain phase 3. The expanded privilege catalogue remains deferred.

## Player use

Use `religion list`, `religion info <deity>` and `religion status`. `religion devote
<deity>` and `religion leave` create two-minute previews; `religion confirm` commits
and `religion cancel` discards them. Confirmation rechecks current affiliation,
class/domain eligibility, cooldown, temple configuration and priest presence.

A completed temple requires devotion at its priest, stationed at the configured
public entrance. A faith with no available temple may still be joined for identity
and channel access; the preview states that services are unavailable. A malformed
registry blocks new devotions until repaired. Departure is available without a
priest, outside combat. It starts a seven-day character-wide devotion deadline.
The first devotion has no deadline. Historical Standing is never spent or deleted.

Existing deity choices automatically count as Devotee membership at zero Standing.
`GET_DEITY` remains authoritative. Changing or leaving an established patron with
Cleric, Paladin, Blackguard, Inquisitor or Sacred Fist levels, or selected domains,
requires the supported staff process. First devotion cannot invalidate selected
domains. Neither membership nor rank grants class abilities or domains.

The old `devote choose`/`deity choose` commands use the same preview flow. The login
roleplay menu permits only first-time creation at level zero, assigns the deity
only on confirmation, and cannot bypass a departure deadline. Respec retains
Standing, cooldowns and affiliation. Explicit staff deity changes clear any
outstanding preview and retain Standing and service cooldowns.

`religion hospitality` gives one random food item and one random drink at an
affiliated configured priest, once per 86,400 seconds across all temples and
faiths. Both have a random existing forage effect and matching flavour; the
normal `eat`/`drink` commands apply the corresponding food/drink bonus when
consumed. Bonuses use fresh forage strength (base +2, with the existing
per-effect scaling) and last 100 game ticks. Hunger and thirst are untouched,
even when disabled. Claiming does not immediately apply the buffs.

The player must have room to carry both items. The SQL `religion_hospitality`
deadline and both estate deliveries commit together; failed transactions grant
neither items nor a cooldown. Committed deliveries remain recoverable across
reboots and failed player saves. The native player deadline is still saved and
honoured; SQL also prevents repeats if that character file is stale. The table
is created by `estate_init`; installations without DDL privileges can preinstall
`sql/20261123_religion_hospitality.sql`.

`religion say`, `religion who` and `religion channel on|off` provide a private
same-faith channel with saved listening preference. Native visibility, silence,
NOSHOUT, minimum communication level, deafness and soundproof-room rules apply.
Messages use per-recipient identity and the web client's ordinary chat output
route; they have no history or Discord forwarding. Staff monitoring follows the
existing `PRF_NOCLANTALK` opt-out. Switched descriptors are excluded. The native
server has no player ignore-list API; no separate ignore system was introduced.

## Building

Set `deity` and `templerank` in MEDIT. Menus list supported deity IDs and the eight
rank names. `None`/rank zero are legacy defaults. NPC identity uses dedicated mob
fields rather than the shared dummy player data. Prototypes persist `MobDeity`
and `TempleRank` e-specs and copy them to spawned/copy-edited mobs. Inspection shows
both. Malformed numeric values remain invalid rather than clamping to a different
faith or rank.

`Block-Religion` and `Block-Temple-Rank` combine with existing directional flags.
The first requires the mob's faith. The second requires the same faith and at least
the mob's rank, whether or not the first flag is set. Clan requirements also apply.
Neither Ghost nor stealth nor ordinary guard exemptions skip these checks. NPC
movement and staff NOHASSLE retain the native bypass policy. MEDIT rejects invalid
requirements; loaded invalid gates fail closed and log a boot warning. These checks
were included with the phase 1 flags so they can be used safely. See the phase 2
guide for progression and the optional authored pilot churches.

Pet shops also enforce a mob's deity and temple rank as purchase conditions
when both are nonzero; blocker flags are not needed. The buyer must currently
follow that deity and have that rank or higher. A stale shop display cannot
bypass stricter requirements on the prototype being delivered.

OEDIT provides `deity` and `templerank` for the same requirement on objects.
Zero in either field disables the religion requirement; malformed data denies
purchase and wear until corrected. Requirements are checked before spending any purchase
currency in NPC shops, player stalls and auctions, and combine with existing
clan requirements, including restricted items inside bags or sheaths. Pet tokens must satisfy both their object settings and those
of the pet they create. Wear, wield and hold commands and the central equipment
path enforce the object's religion requirements alongside existing clan checks.
Selling, giving and carrying property do not remove its requirements. Stock with different requirements is never grouped together or
mistaken for unrestricted manufactured stock.

Object prototypes persist a two-value `F` record. Object instances always save
an `RReq` record, including zeros, across file/SQL saves and estate transfers.
No SQL schema change is needed. The shared rank-based restring service remains
phase 3; its player menus must omit ranks and wear slots without published
matching entries, while staff can still see gaps for authoring. Its application
transaction must also stamp the entry's clan/Favor or deity/temple-rank requirement
and persist it together with the appearance and payment.

Use `religionadmin` at builder level. Example (replace IDs/vnums with actual content):

```text
religionadmin create 1 1 Temple of the Healer
religionadmin set 1 entrance 100
religionadmin set 1 priest 200
religionadmin set 1 hospitality 1
religionadmin set 1 enabled 1
religionadmin show 1
```

A new record is a disabled draft. The priest prototype must already have matching
faith and rank 1..8. Spawn/reset it after changing its prototype. It must be awake,
visible and outside combat at the configured entrance. A priest prototype belongs
to only one temple. Several temples may share a deity. Display names do not carry
identity; IDs do. `religionadmin list` shows all editable fields, `delete <id>
confirm` removes a record, and `reload` rereads the registry. Disable an active
temple before rebuilding its configuration.

## Persistence and verification

Player files retain `God` and add `RlSt` records for nonzero per-deity Standing,
plus `RlCd` for the devotion deadline, hospitality deadline and channel-off flag.
Historical slots are preserved even when unavailable in the current campaign.
Missing fields default to zero/on. Malformed religion fields are rejected and
logged. Rank is derived, and confirmations are session-only. Membership and
preference changes use checked native player saves; failed writes restore the
previous values and retry saving that restored state. Hospitality additionally
uses the durable SQL claim and delivery journal described above.

The versioned `etc/religion.json` registry uses the native durable file writer
(atomic replacement and file/directory fsync). A missing file starts empty; an
invalid file disables services and edits until corrected and reloaded. Runtime
world-reference checks disable unavailable services without changing membership or
class abilities. A failed registry save rereads the authoritative file, including
the possibility that rename succeeded before directory fsync failed. There is no
additional SQL dependency for the original membership registry. Hospitality,
chests and phase 2 progression use the SQL migrations described in their guides.
The optional help SQL patch installs the same
help entries for deployments using database help.

Run `python3 tests/run_religion.py` for the production module's membership,
confirmation expiry/revalidation, save failure rollback, pfile compatibility and
round-trip, temple persistence, hospitality, native visibility and channel privacy,
creation-menu protection, mob field copying/tag parsing, and combined guard rules.
The test uses native structures and visibility macros with isolated world/save
adapters; it does not start a live server or require a database.

## Persistent temple chests

In the desired permanent public room, a builder with zone editing permission
runs `religionadmin chest <deity-id> <minimum-temple-rank> <item-limit>`.
Rank must be 1–8 and capacity 1–10000. The command creates or updates one
immovable `temple chest` in that room. It does not need a temple registry entry,
a priest, an object prototype or a zone reset. Player housing is excluded.

Players use `look in chest`, `examine chest`, `put <item> chest` and
`get <item> chest`, including the existing numbered and `all` selectors.
Viewing and transfers require the same active faith and at least the configured
rank, without a staff or NPC bypass. Nested objects and sheathed weapons each
consume capacity; capacity cannot be reduced below current usage. Containers
remain intact on withdrawal. Existing persistent-storage exclusions and
legendary ownership checks apply. The chest itself cannot be picked up.

`temple_chests` stores room configuration and `temple_chest_items` stores native
serialized items. The existing estate escrow and delivery journal provide
transfer recovery. Chests materialize after zone resets at boot and their
contents remain in SQL. `estate_init` creates missing tables; installations
without DDL privileges can preinstall `sql/20261123_temple_chests.sql`.
