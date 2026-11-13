# Clan progression operator notes

Phases 1–50 are complete as disabled foundations and isolated validation.
The full clan/server/web checks and migration/recovery rehearsals pass. Phase 51
uses the user-approved replacement of the staffed pilot with mathematical
pacing and automated checks; rules-2 pacing and the startup policy loader are implemented and validated. Phase
52 has a review candidate and checklist, not deployment approval. See the
[Phases 50–52 handoff](clan_progression_phase_50_52_handoff.md),
[pilot review](clan_progression_pilot_review.md) and
[release checklist](clan_progression_release_checklist.md). No live schema,
authority, service, feature-enable or deployment changes occurred.

## Authority and configuration

`CLAN_STORE_MODE` is the existing persistence selector (`legacy`, `shadow`,
`mysql`). It is not an earnings or UI enable switch. Core schema v6 and a checked,
completed import/cutover are prerequisites for SQL authority. Never switch back
to legacy files after SQL writes without the documented authority recovery
procedure; stale files cannot replace committed SQL state.

The release policy is `struct clan_progression_config` in
`src/clan_progression.{c,h}`, installed through the trusted server API
`clan_buff_runtime_configure`. Defaults select legacy storage, progression OFF,
all five feature controls false, no public release and an empty pilot allowlist.
The server reads `lib/clan/progression.json` after SQL persistence hydration and
before clan recovery. This is a server-owned startup file, not a player command.
A missing file leaves gameplay OFF; malformed, oversized, wrongly typed or
inconsistent settings abort startup. A live policy requires checked SQL authority.
The shipped file is OFF. Explicit settings are `version: 1`, `mode` (`off`,
`shadow` or `live`), booleans `buffs`, `contracts`, `expeditions`, `quartermaster`,
`web`, `public_release`, and a `pilot_clans` array of stable incarnation UIDs.
All nine settings are required. To enable all clans on dev, use live mode,
feature booleans true, public_release true and an empty allowlist in DEV's file
only. Runtime changes require a cold restart/copyover that runs boot; there is
no player-facing reload or automatic enablement. Feature-off never means
switching SQL storage back to legacy after cutover.

Storage readiness, feature controls and pilot eligibility are independent gates.
Absent/invalid configuration fails closed. Configuring SQL persistence alone
does not enable awards, buffs, contracts, expeditions, quartermaster or the web
report. Disabling a feature does not erase accepted receipts or progression;
recovery paths must remain available for already accepted economic operations.

## Optional SQL extensions

Apply scripts explicitly to a backed-up, offline staging database first:

1. Core `20261110_clan_progression_v001.sql` through `v005.sql`, in order,
   then the unused-economy cutover `20261113_clan_progression_v006.sql`;
   use the existing importer, verification and authority-cutover procedure.
2. `20261111_clan_buff_activations_v001.sql`, then
   `20261111_clan_buff_lifecycle_v001.sql` for saved activations/lifecycle.
3. `20261111_clan_kill_sources_v001.sql` for durable kill contribution facts.
4. `20261111_clan_delivery_v001.sql` and
   `20261111_clan_objectives_v001.sql` for contract families.
5. `20261112_clan_expeditions_v001.sql`, then `v002.sql` for enrollment,
   combat progress, completion proofs and claims.
6. `20261112_clan_quartermaster_v001.sql` for bound-item purchases.

There is no new Phase 46 data migration. The web extension reads these existing
stores; missing optional schemas leave their sections unavailable while core
status remains usable. Publish help separately with
`20261112_clan_progression_services_help.sql` after the help schema exists.
That script updates entries and aliases only; it does not enable gameplay.

Retain SQL receipts, operation journals, roster invalidations and weekly claims.
Back up SQL together with player and inventory files before rehearsals. Goods,
gold and item delivery use independent saved files and recoverable SQL proofs;
a SQL-only restore is not a coherent economic rollback. Do not delete pending
rows or clear player locks to make a command proceed. Preserve operation IDs,
absolute deadlines and original clan/account/week identities through restart,
copyover and recovery. Collect inconsistent retained proofs for review.

## Player and panel workflows

An accepted member begins with `clan progress` and `clan favor`. In the Krynn web client, select Clan and open the relevant
service section. Gateway subscriptions are Krynn-only. Current private reports
are bounded to 49152 bytes and refresh at most once per two seconds per
subscribed descriptor. Old/absent extension data leaves core status usable.
Disconnected or reports older than ten seconds disable panel actions.

For buffs, an authorized manager chooses a tier and slot, reviews the price and
replacement, then confirms. The client persists a receipt identity before
sending. Retry that same operation after uncertain delivery. Cancellation gives
no refund and never clears a family's saved lockout.

For delivery, list/accept at a quartermaster, acquire exact-grade materials or
eligible finite ammunition, then preview turn-in. Review the exact capped
credit and confirm within 60 seconds. Changes in goods, membership or caps are
revalidated. Retry recovery preserves accepted identity; refreshing cannot
consume goods. For gathering and bounty, accept first, then perform fresh base
harvests or qualified combat. Their displays show saved event progress; stock
already owned or purchased cannot advance gathering.

For expeditions, create a listed outing and share its UID. Two to six distinct
accounts opt in at a quartermaster. All enrolled members gather there, out of
combat/crafting; the creator uses `clan expeditions start`. The panel's
`Check roster / manage in text` button runs `clan expeditions active`; start,
leave and abandon remain explicit text commands. Complete the listed objectives
with qualified participation. Each personal finisher needs half the counted
events (rounded up) and the final event; at least two accounts must finish.
Then inspect claims and claim personal Favor at a quartermaster. The claim
button uses the saved outing UID. The server alone creates completion proofs.

For quartermaster goods, visit a quartermaster, inspect current gold, Favor,
clan/character level, carrying and normal item restriction gates, then preview
purchase. Confirm the frozen item and gold price within 60 seconds. Favor is a
standing gate and is not spent. Accepted purchases deliver once to the original
recipient and remain bound with zero resale value. `clan progress` or reconnect
runs recovery; don't invent a new purchase UID after an uncertain result.

The panel shows provisional eligibility. Every command rechecks authoritative
state; another member's action or cap change can invalidate a displayed choice.
No browser packet contains a reward amount, combat event or completion claim
constructed by the client.

## Timers, categories and currencies

All five initial buffs last 24 hours with 12 hours actual downtime after their
scheduled expiry (36 hours from activation). Early cancellation/replacement
retains the original lock. Tier costs are 13/19/29 Influence; unlock levels
are 1/5/20, except Artisan 3/5/20. Scholar is growth; Arcane and Psionic are
recovery; Vitality is combat; Artisan is craft. Two slots become three at clan
level 10, with one combat buff maximum. Refer to player help and the defaults
for exact effect percentages and stacking/source restrictions.

Lifetime rank Influence, spendable Influence, personal Favor, character XP,
gold and legacy clan points are distinct. Buffs spend Influence; purchases spend gold, never Favor.
Earned currencies retain eighth-point precision. Account contribution weights
receive full credit through 600 and half for the next 600; weekly Favor caps
at 150. Earned Influence caps at 625/week and 2500 stored. Each actually
credited Influence eighth also advances the separate lifetime rank counter. These caps compose and can reduce
credit to zero without removing source-specific claim limits.

Daily windows begin 00:00 UTC; weeks begin Monday 00:00 UTC. Reset never wipes
accumulated lifetime Influence, spendable Influence, Favor, durable receipts or absolute buff timers.
Uncommitted contract/claim deadlines remain their original boundaries; accepted
pending receipts recover in their original week. Started outings last at most
two hours and never cross the weekly boundary. Browser-local deadline rendering
does not change the UTC reset policy.


### Rules-2 pacing cutover

After core v005 apply `sql/20261113_clan_progression_v006.sql` offline. This
semantic migration requires unused progression: no nonzero balances, contribution
windows, pending awards or buff history. It refuses used economies rather than
invent lifetime Influence from Clan XP or rewrite receipts. The new binary
requires core v6. The historical `xp_eighths` column/wire field stores lifetime
credited Influence under rules 2. Keep spendable `influence_eighths` independent.
Optional activation/lifecycle migrations accept both core v5 and v6.

The approved six Favor and twenty clan thresholds, and the exact smaller rewards,
are listed in [pacing](clan_progression_pacing.md). Edit the independent percentages
in `src/clan_pacing.h`, rebuild, validate and restart to shorten progression later.
A lower Favor percentage also shortens quartermaster standing gates. Frozen
catalog terms and receipts keep their original base values. Initial percentages
are 100; the staffed pilot is explicitly waived. The old release archive must be
rebuilt before any deployment review. No runtime flags were enabled here.


### Dev startup rollout, 2026-11-13

Dev's checked import and cutover contain eight clans and 38 memberships in
`krynn_dev`, store mode 2/revision 2. Dev is stopped pending operator installation.
The live world passes all ten expedition definitions and matches the seven
quartermaster prototype records; the older dev world does not. Use a validated
snapshot of live's `lib/world` on dev so both use the same catalogs. Do not copy
live player files, clan data, credentials or database settings over dev.

`/home/krynn/dev/install-clan-progression.py --check --sync-live-world` verifies
the prepared payload and cutover without installing. `--apply --sync-live-world`
backs up dev library/previous binaries and copies only the world, server binary,
clan catalogs, and explicit startup settings. It preserves repaired player and
clan files and does not start the server or modify live. The install report
records the backup and installed binary hash. Keep the stopped state until
installation succeeds; then use dev's existing `checkmud.sh` launcher on 4301.
The gateway/browser deployment remains a separate operation; no shared/live web
service is restarted by this dev installer.
