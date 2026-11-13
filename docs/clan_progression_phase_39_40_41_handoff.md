# Clan progression: Phases 39–41 handoff

Implemented behind the existing disabled quartermaster/live/pilot gates. No game
database migration, help publication, enablement, deployment or restart occurred.
No web client changes or personal services are included.

## Initial catalog and budgets

`lib/clan/quartermaster.json` contains seven approved existing indexed templates.
The catalog and live adapter reject absent references, unsupported versions,
duplicate IDs/prototypes, malformed fields, price-policy violations, invalid gates,
and mismatches in approved type, weight, character level or all sixteen values.
These entries permit no enhancement, stat affects, permanent character effects,
weapon/channel spells, activated spells, special abilities, scripts or C special
procedures. Thus activated utility has a zero budget; adding any requires a new
explicit power review and validation policy. Gates never purchase extra stats.

| Catalog ID | Prototype | Character level | Clan level | Favor | Approved value | Gold price | Ordinary power/coverage |
| --- | --- | --- | --- | --- | --- | --- | --- |
| field-map | 3006 | 1 | 2 | 100 | 15 | 15 | Existing readable map; all classes |
| field-knife | 3001 | 1 | 7 | 500 | 70 | 140 | 1d4 knife; normal knife restrictions |
| staff-commission | 9600 | 1 | 8 | 1500 | 100 | 300 | 1d6 quarterstaff; simple weapons |
| sword-commission | 9603 | 1 | 8 | 1500 | 490 | 1470 | 1d8 longsword; martial weapons |
| bow-commission | 2210 | 5 | 8 | 1500 | 10 | 30 | Existing 1d9 longbow; ammo separate |
| hide-commission | 3004 | 1 | 8 | 1500 | 30 | 90 | Existing armor value 3 hide tunic |
| shield-commission | 309 | 1 | 8 | 1500 | 480 | 1440 | Existing armor value 4 shield |

Values are the approved ordinary template values, including the existing low
longbow value; price multipliers are 1x supplies, 2x utility, 3x commissions.
These are ordinary alternatives, not PvE-best-in-slot upgrades. The initial slice
does not stock higher standings or prestige appearances. Favor is cumulative
standing for the character in the acquiring clan incarnation and is never spent.
No discounts, Influence charges, clan XP changes or repeat discounts apply.

## Commands and commit protocol

`clan quartermaster list` shows gates, coverage and budgets. `buy <id>` and
`commission <id>` validate the actor and produce an exact-price, 60-second quote
with `confirm <id> <purchase-id>`. Quote terms and clan must still match at commit;
expired, changed or missing quotes require another preview. Uncommitted quotes
are intentionally ephemeral. Replaying a committed confirmation returns its own
recorded result, even after departure, without creating another purchase.

New acceptance checks authenticated identity, current SQL accepted membership,
clan incarnation/status/derived level, that character's Favor, character level,
normal alignment/class/item restrictions, quartermaster presence, combat/crafting,
carry space, gold, content readiness and feature/pilot gates. The SQL service
rechecks durable acquisition gates under the core store lock. Failed acceptance
touches neither gold nor inventory. A committed intent is a pending purchase,
not a failed purchase to be retried under a new identity.

The explicit optional `sql/20261112_clan_quartermaster_v001.sql` extension requires
core v5. It retains immutable original recipient/account/clan/time/terms and a
serialized bound item, with one pending operation per character. SQL identity,
unique pending slots and monotone acknowledgement prevent competing operations.
Readiness requires InnoDB, the expected version, keys, generated slot and guards.
Schema installation is never automatic. Optional help publication is separate.

Recovery uses two independent durable stores in order:

1. Checkpoint gold debit and original operation/paid marker atomically in the
   character file using `save_char_checked`.
2. Deliver the bound serialized item with its original `CQm` identity, checkpoint
   inventory and its own operation marker with `Crash_crashsave_checked`.
3. Acknowledge SQL only after both checkpoints succeed, then durably clear the
   character lock. No commands run while recovery remains unresolved.

Retries retain the in-memory receipt after uncertain saves. After process death,
each file's own marker determines whether its step already happened; inventory
markers without their exact item or conflicting identities fail closed. Login,
copyover's common enter path and command recovery find pending SQL intents even
if the first file checkpoint never happened. No catalog/current-clan gate is
reapplied to already accepted delivery. Recovery also runs with purchases disabled.
SQL acknowledgement does not itself mutate gold or create an item; it is an
internal API called only after both durable checkpoints.

The item codec persists `CQm`, existing binding and zero resale cost in both file
and SQL readers/writers, including the estate frozen-item codec. The inventory
header appends its marker after existing delivery fields; old six/eight-field
headers still load. `ClQM` records the paid currency marker in the player file.
Recover complete authoritative SQL, player and inventory stores together; never
clear locks or delete journal history to resolve inconsistent storage. Drain
pending purchases before a downgrade to a binary unaware of these markers.

## Ownership and departure

Copies bind through existing `bound_id` to the acquiring character, whose ID must
fit that field. They retain ordinary weapon, armor, level, alignment and class
rules. No equip/use path checks clan membership or Favor. Departure does not
remove gear, change binding or invalidate use by its owner. Giving it away does
not change its existing binding or make another character its owner. Setting
copy cost to zero prevents shop resale profit; existing bound-item exclusions
continue to apply to auctions and player stalls. New purchases/services still
require current accepted membership, regardless of retained historical Favor.
A purchase committed before departure delivers only to its original recipient.

## Verification

- `python3 tests/run_clan_quartermaster.py`: strict catalog and indexed prototypes;
  private MariaDB gates, insertion rollback, concurrent intents, idempotent receipt
  reads/replays, immutable history and schema guards; production game coordinator
  with separate persistent checkpoint stores, crashes after each save/SQL ack,
  uncertain writes, confirmation replay, quote expiry/change, acquisition item
  restrictions, template drift, and pending delivery after departure/feature disable.
- `python3 tests/run_clan_delivery_header.py`: actual production header writer and
  parser, both receipt types, old headers and malformed marker refusal.
- `python3 tests/run_estate_integration.py`: actual object codecs round-trip `CQm`
  through file and SQL, alongside existing estate/market recovery regressions.
- `python3 tests/run_clan_delivery_recovery.py`: existing independent file/SQL
  delivery recovery remains valid. Full Make and CMake builds pass.

Database checks use disposable socket-only MariaDB, not the game database. MySQL
8 compatibility follows the existing schema approach but was not independently
exercised. Indexed world files must be audited against the intended deployment;
boot/list/purchase perform live template checks as well.
