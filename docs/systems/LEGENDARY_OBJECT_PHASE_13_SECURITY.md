# Phase 13: integration, security, recovery and performance

Implemented across 2026-10-24–25; final security/sanitizer verification completed
2026-10-25. No live world files, production database, deployment or
server restart were changed. SQL schema stays at v5. This phase introduces an
additional durable runtime file format, described below.

## Changes and audit findings

**Combat SQL removed.** Previously, each effective rolling-budget debit opened a
SQL transaction, and participation/dragonlance checks could refresh registries
inside attacks. Budget history now loads at login or activation. Combat uses a
bounded in-memory ledger and checkpoints each actual benefit through the existing
atomic file replacement/fsync helper before granting it. Heartbeat mirrors pending
entries to SQL in a batch, using the existing unique operation IDs for idempotence.
NPC recipients retain their bounded in-memory instance budgets.

Boss/lance metadata refreshes outside attacks, in heartbeat or explicit
activation/staff edits. Stable attack-time checks use the read-only registry
cache. The native post-defense damage path calls `legendary_combat_packet`;
tests execute that exact function extracted from `fight.c`, rather than copying
its effect accounting. Ordinary defenses remain before this finalizer and native
HP loss remains after it. Budget, temporary-HP, offense, source and participation
accounting continue through the production adapters.

**Character deletion now retires ownership.** Permanent deletion tombstones all
of the owner's legendary items and clears the legendary wallet/selection state.
Pending SQL operations, pending accepted awards and a durable player-file
operation marker prevent retirement until recovery succeeds. Self-deletion
checks retirement before deleting inventory or account membership, including
when fast-wipe is disabled. The index/file removal path checks retirement too;
name recreation stops if deferred deletion left the old index entry present.
Unloading, saving, storage and ordinary extraction still do not tombstone items.

**Recovery fixtures now lose real processes.** The test-only checkpoint hook exits
child processes at all 20 purchase checkpoints, both for an existing UUID and
first awakening. The parent reloads real player receipts/object records and
checks exactly one committed or cancelled result. Separate tests interrupt
accepted quest-XP awards before and after ledger commit, then recover/replay the
same source without duplicate XP or claims. These hooks are compiled out of the
production binary.

## Budget journals: operational requirements

- Location: `etc/legendary-budgets/` relative to the running MUD data directory,
  normally `lib/etc/legendary-budgets/`. The directory is created automatically
  with mode 0700. Each numeric player-ID file uses version `LGB1`; files contain
  a row count and SHA-256 digest plus operation UUID, UTC time, budget kind
  and actual amount. They contain no names.
- Journals are authoritative until SQL acknowledges them. **Back up and restore
  this directory with the database/player data. Do not discard pending journals
  or roll back to a binary that ignores them while their windows are active.**
  SQL alone may lag pending combat debits, normally by one heartbeat.
- A per-player lock file prevents two processes sharing this runtime directory
  from spending the same allowance concurrently. Run one world writer per
  runtime/journal directory; independent world instances with separate journal
  directories must not share these characters as concurrent writers.
- Lock descriptors close automatically on copyover exec so the new process image
  can acquire them again. When upgrading from a build without this flag, perform
  one full shutdown/start: copyover cannot retroactively close a lock descriptor
  inherited by the current process. Keep the journal and lock files intact.
- A player ledger holds at most 280 one-point events across the four limits
  (60 offense, 100 prevention, 100 healing, 20 resource). Larger individual debits
  use fewer rows. Expired entries are pruned by their actual window; future
  timestamps still count after clock rollback. File input is bounded to 32 KiB.
- Login merges local and SQL histories by stable operation ID and validates
  totals. Malformed/truncated data (including complete-row truncation), digest failures,
  mismatched duplicate IDs, unavailable initial
  history, or an occupied writer lock fail closed. Ordinary account-menu preview
  cleanup cannot release the active character's budget cache/lock.
- A disk checkpoint failure grants no benefit. A failure after rename may have
  durably consumed allowance; recovery conservatively retains that debit.
  Heartbeat retries blocked checkpoints and SQL mirroring. An already loaded,
  valid journal can safely retain bounded combat debits during a SQL outage;
  economic purchases/awards still require their SQL recovery protocol.
- These are rolling-cap receipts, not replayable effects: a restart never
  replays damage, healing, temporary HP, discounts or prepared powers.

## Acceptance coverage

| Area | Executed coverage / audited native route |
| --- | --- |
| Costs and rank modifiers | Independent cost/parity tests, all power/rank/modifier combinations, rank-V field whitelist, real IV/V smith transitions and free invalidated replacements. |
| Payments and concurrency | Real SQL write rejection, stale/concurrent quotes, duplicate confirmation, gold acknowledgment failures, durable-file faults and all 20 process-exit checkpoints. |
| Earnings | Actual XP/healing entry points, quest daily identity, boss participation/cooldowns, account-character separation, partial allowances, UTC reset, exclusions, overflow and accepted-source process-loss recovery. |
| Character/object persistence | Production player, room/house, pet and sheath writers/parsers; nested estate payloads, both two-handed channels, malformed/duplicate/missing/tombstoned identities and live prototype/clone behavior. |
| Storage/delivery/commerce | Actual house/chest put/get and owner reload; escrow/delivery recovery; auction/stall entry points reject awakened gear even with damaged ordinary binding flags. Existing estate tests exercise ordinary commerce and exactly-once delivery. Mudmail is text-only (`mail_t` has no item attachment); item delivery uses estate framing. |
| Death/corpses | Existing death/logout/extraction hooks cancel leases. Actual native container/corpse retrieval rejects a non-owner, including staff, before ordinary corpse rules, and permits owner recovery. Room-container persistence uses the production object codec. |
| Crafting/reforging | Recursive TRANSFORM barriers reject awakened gear, including nested/sheath identities. Native reforge ordering is checked before any gold deduction; crafting transform/resize/enchant/recycle routes were audited against the shared barrier. Crafted/reforged XP remains excluded from infusion credit. |
| Deletion | Unsettled player-file and SQL receipts block retirement; after recovery retirement is idempotent, tombstones gear and zeroes the wallet. Native self-delete and file/index deletion are checked before destructive operations. |
| Permissions and input | Numeric overflow/sign/trailing-garbage corpus, oversized/unclosed input, 512 deterministic byte-input mutations and inaccessible grant/admin/registry/SQL verbs. No quotes, wallet changes or operation rows are created by rejected inputs. SQL values use validated identities/numbers or the existing escaping helpers. |
| Gameplay and lifecycle | All foundations, techniques and signatures; native final damage accounting, target/group/formation/mount checks, shared limits, redirects, resisted/zero-damage packets, real point payments, source removal and expiry. Sanitizers cover the production adapters and extracted native routes. |
| Budget persistence | Real process loss after local debit, cross-process writer lock, SQL write failure/retry, duplicate flush, disk fault points 1–5, malformed journal rejection, full 280-row reload and all four ceilings. |

## Measurements and validation

The isolated sanitizer run measured:

- 1,000 native legendary combat packets: **0 SQL calls**, approximately **1.52 ms**
  total, including the effective prepared-strike journal write.
- Cold budget login/import: **3 SQL calls**.
- 280 durable one-point debits: **0 SQL calls**, approximately **51.05 ms** total.

These are fixture-machine measurements, not production latency guarantees or
balance results. Activation, login, heartbeat mirroring, economic operations and
boss-death award commits intentionally still access SQL. Stable combat reads and
ordinary per-packet budget accounting do not.

```sh
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 \
LEGENDARY_TEST_CFLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer -g' \
ESTATE_TEST_CFLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer -g' \
python3 tests/run_legendary_security.py
python3 tests/run_legendary_effects.py
cmake --build /tmp/krynn-legendary-phase8-cmake -j4
make -j4
git diff --check
```

The integration runner uses disposable MariaDB and production codecs, economic
operations, effect/catalog code, native XP/healing functions, native damage
finalization and native corpse retrieval. World/dice/stat calculations and
unrelated container side effects have deterministic test adapters. This does
not claim a complete live-server session, real endgame route measurements,
phase 14 balance approval, or phase 15 rollout. No known failing acceptance case
is being deferred under a claim of successful deployment.
