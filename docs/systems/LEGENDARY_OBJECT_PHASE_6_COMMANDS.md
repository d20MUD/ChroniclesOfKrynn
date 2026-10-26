# Phase 6: smith service and player commands

Implemented on 2026-10-24. Phase 6 adds the `infuse` player command, safe
quotes and persistent configuration. It does not activate equipment bonuses or
powers; equipment enforcement/cache and gameplay effects remain Phases 7–12.

## Player flow

Read-only help, inspect, materials, paths, choices and loadout work away from the
smith. Inspection includes channel rank/path/family, stable selection IDs,
subtypes, exact resolved selected-power values, remaining first choices,
investment and next-rank costs. Choices lists all 47 definitions, availability
reasons, trained skill subtypes and selected-power modifier compatibility.
Loadout displays persisted priorities/major selections and character cooldowns;
it explicitly identifies these as configuration rather than active bonuses.

Changes require a conscious, connected level 30 character out of combat, at an
explicitly registered smith. Recognition uses the NPC prototype and live runtime
identity, rejecting pets, charmed NPCs and test instances. The same smith must
still be present at confirmation. No name-based recognition is used.

Awaken, advance, select, modify, path/family reshape, exchanges and free
prioritize/attune/unattune all require an exact single-use quote and confirmation.
An item quote identifies the target channel, rank, path, family and selections,
shows every resource change, and states that the other channel will not advance.
Awakening warns about permanent binding before confirmation. Standard keyword
matching, quoted multiword names and numbered targets are supported; ambiguous
unqualified targets fail without choosing the first object.

Quotes expire after 60 seconds. Changed inventory/equipment, item eligibility or
fingerprint, smith, combat/consciousness, selected skill/resource/dragonlance
requirements, gold, ledger or persisted item versions reject confirmation.
Cancel, logout/free-character and opportunistic expiry release pending contexts.
Reconnecting requires a new quote, while completed ranks and free choices persist.

The default essence allocator is deterministic. `infuse mix` accepts the six
exact payment amounts for a pending item quote, enforces its matching minimum
and total, and issues a new quote ID. Catalyst exchanges display their exact
inputs; exchange outputs share the existing daily acquisition limits. Materials
shows balances, remaining acquisitions and the next midnight UTC reset.

## Persistence and transaction boundaries

All economic mutations use the Phase 4 transaction service. Exchanges delegate
to Phase 5's existing capped exchange participant. No command directly adjusts
wallet balances or character gold. Permanent identity/binding and crash recovery
continue to use the existing checked checkpoints and operation journal.

`first_choices` is a persisted selected-choice bitmask: foundation=1,
technique=2, signature=4, modifier=8. Rank purchases do not consume these bits.
The first choice is free; replacements use the approved reshape price. Path or
family changes clear invalidated selections and their bits, including an
incompatible modifier, permitting free replacement without a second reshape fee.
Rank V automatically resolves improved values from the pure catalog.

Two-handed supporting channels require an actually wielded weapon in the
explicit two-hand slot, or an actually wielded mandatory two-handed ranged
weapon, with no occupied offhand. Supporting assault/guard/focus uses the
weapon/shield/held pools respectively. Each channel advances independently and
pays full rank costs. Primary equipment may be carried or equipped; supporting
configuration requires maintaining the verified two-handed grip.

Saved priorities and major selections use the existing character TEXT columns,
with canonical UUID/channel references and an additional technique/signature
kind for major selections. They are updated inside a zero-cost journal
transaction under the character wallet version lock, with the target item
version locked/rechecked as well. There are at most three attuned majors and
64 saved priority references; runtime passive budget evaluation remains Phase 7.
Missing/deleted selections display as unavailable and may be unattuned.

An existing Phase 4 mismatch was fixed: ordinary unbound objects use `bound_id`
-1 as well as 0. First-awakening quotes now accept both, retaining the original
binding in the rollback journal.

## Offline deployment preparation

Apply existing migrations v1–v3, then
[`20261024_legendary_smiths.sql`](../../sql/20261024_legendary_smiths.sql).
Schema v4 adds `legendary_smiths`; readiness checks all thirteen InnoDB tables.
The migration is repeatable and never downgrades a newer schema. Missing schema
fails closed; ordinary characters without pending awards/operations retain
normal play. Runtime does not auto-create or migrate tables.

Register a chosen existing NPC prototype explicitly, substituting its real vnum:

```sql
INSERT INTO legendary_smiths(mob_vnum, enabled) VALUES (<smith_vnum>, 1);
```

No live SQL migration, NPC placement, world file edits or server restart was
performed. Content placement remains Phase 15. Player help is provided in
`lib/text/help/infuse.hlp` and included in the repository help index. Sites using
database-only help can import the same text through their normal help workflow.

## Validation

```sh
python3 tests/run_legendary_commands.py
ASAN_OPTIONS=detect_leaks=0 \
ESTATE_TEST_CFLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer -g' \
python3 tests/run_legendary_commands.py
python3 tests/run_legendary_rules.py
```

The command runner uses disposable MariaDB and the real transaction, earnings,
identity, persistence and object checkpoint implementations. World/ordinary wear
checks use the shared fixture adapter. It reruns estate/housing/auction/stalls,
legendary persistence, transaction fault/replay and earnings regressions.

The ordinary-player script covers awakening an initially unbound object,
ambiguity, cancellation, inventory invalidation, all five ranks, free first and
paid replacement choices, modifiers, invalidated free replacements after a path
reshape, exact essence mixing, independently purchased supporting ranks, saved
priorities and three-major limits, exchanges/caps, no-smith/combat restrictions,
expired quotes and insufficient funds. No live database is used by tests.

Validation completed successfully: the scripted flow and all included regressions,
AddressSanitizer/UndefinedBehaviorSanitizer (fixture leak detection disabled),
pure-rule tests, and full Make/CMake builds. The changed command module compiles
without warnings. No production migration or restart was performed.
