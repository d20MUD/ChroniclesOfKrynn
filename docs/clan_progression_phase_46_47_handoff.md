# Clan progression Phases 46–47

Phase 46 adds optional service details to the existing private `CLAN_PROGRESS`
report and renders them in the Krynn Clan tab. Phase 47 documents the actual
commands, policy and operator boundaries. Defaults remain disabled. Neither
repository has been deployed, restarted, migrated or enabled.

## Server read model

`clan_web_content.c` composes bounded contract, expedition and quartermaster
reads inside the core report's consistent SQL transaction. It never accepts,
expires, reconciles, claims or writes receipts. Optional storage failures mark
only the affected service unavailable. Active contract terms are parsed by the
same strict parsers as the command services; weekly definition claims use the
existing service readers. Expedition counters use the saved events and only
the requesting character's participation/entitlements. Other roster member IDs
and account identities are not published.

`clan_web_content_runtime.c` enriches the snapshot after releasing the SQL pool
connection. Shared production catalog loaders, exact delivery goods predicates,
live world references, current character restrictions, gold/carrying state,
quartermaster presence and feature/pilot gates explain current eligibility.
Delivery inventory counts are distinguished from saved fresh gathering and
qualified bounty events. Expedition details include target alternatives,
counters, personal participation requirements, roster size and claim gates.
The five buff families now expose their actual growth/recovery/combat/craft
categories, preserving the old version-1 report shape.

Existing ephemeral delivery and purchase quotes can be displayed with their
server-issued IDs, exact credit or fixed gold price and original deadlines.
Polling never creates those quotes or mints operation identities. Text commands
continue to perform all authoritative revalidation and economic mutations.
Oversized content is removed while preserving core status within the existing
49152-byte report limit.

## Client actions and compatibility

`shared/clan-content.ts` bounds and normalizes the optional extension. Missing,
old or malformed service data leaves the valid core report usable. It constructs
only fixed command verbs with validated catalog slugs and immutable UIDs;
labels and descriptions never become command input. `ClanServices.tsx` renders
collapsible service sections with plain escaped text, saved progress, reward
notes, preview deadlines and lock reasons. All actions use the existing
`ClanPanel` → `App.sendInputLine` route. Disconnected/stale state disables writes.
Turn-in confirmation, purchase confirmation and personal claims require review
and a second click, then use the same server receipt/outing identity on retry.
There are no client reward amounts, combat event submissions or completion
assertions. Expedition start/leave/abandon remain explicit text workflows;
active-roster output supplies their context.

## Help and operator handoff

The player help set now covers currencies, caps, UTC windows, actual buff
categories/effects/costs/timers and text/panel workflows. The repeatable
`20261112_clan_progression_services_help.sql` publishes matching entries and
aliases without enabling services. `clan_progression_operator_notes.md` names
real schema prerequisites, the persistence authority selector, trusted policy
API, disabled defaults and recovery/backup boundaries. There is no new data
schema or invented feature environment variable.

## Verification and next work

`tests/run_clan_web.py` exercises the production SQL model with optional schemas,
saved contract and expedition events, privacy, no-write polling, missing-schema
failure, and the production runtime eligibility composer with isolated world
adapters. Generated core wire and enriched service fixtures feed the web
regressions. Client tests cover fixed command builders, malformed-extension
fallback, current/stale controls, saved counts, target/participation rendering
and safe text. CMake and TypeScript/Vite builds validate integration. The quartermaster and delivery recovery regressions pass against the reused
catalogs and goods predicates. The client suite passes 130 tests; lint has zero
errors and the existing `App.tsx` hook-dependency warning. No interactive browser
session was used; component output and command callbacks are covered by tests.

Reproduce from the MUD checkout:

```sh
python3 tests/run_clan_web.py
python3 tests/run_clan_quartermaster.py
python3 tests/run_clan_delivery_recovery.py
python3 tests/run_clan_objective_runtime.py
cmake --build build -j4
```

From `/home/krynn/LuminariWebClient`:

```sh
node --test tests/*.test.mjs
npm run build
npm run lint
```

After deliberately changing the report, copy the generated core JSON/MSDP and
service JSON fixtures from `code/tests/fixtures` into the web client's
`tests/fixtures`, preserving its hyphenated names, before running web tests.

Next is Phase 48 integrated buff lifecycle acceptance, then Phase 49 integrated
progression acceptance. Pilot enablement, staging migration rehearsal and
release remain later phases.
