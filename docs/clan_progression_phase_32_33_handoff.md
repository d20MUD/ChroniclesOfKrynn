# Clan progression Phases 32–33

Implemented as disabled foundations. No live migration, progression enablement,
installation, deployment or process restart was performed. No new SQL schema is
required; use the existing optional delivery and objective extensions over core
v5. Deploy all three `lib/clan/*.json` catalogs with the game library.

## Phase 32: rotation, expiry and repeat limits

Server catalogs rotate at Monday 00:00 UTC using the existing half-open weekly
period calculation. Stable definition IDs determine the order, independent of
file order, process timezone, account or clan. Each cohort offers all but one
choice when it has more than two entries; pairs remain available and singleton
higher-level unlocks remain available. Delivery cohorts use clan unlock level.
Gathering/bounty cohorts use family, clan unlock and character eligibility, so
rotation preserves at least two choices wherever the underlying supported band
has two choices. Level changes can change the eligible pool; they never reset
account claims. Additional immutable copper-harvest, sewer-spider, saber-tiger
and Aurak-guard IDs provide rotation without repurposing prior definitions.

Both list and accept use the same validated, rotated catalog. Lists expose the
next UTC reset. Already accepted terms remain frozen and do not depend on the
current pool. Commands recover saved source occurrences and delivery checkpoints
before applying a lazy per-character SQL reset under the existing control lock.
At the exact expiry, unfinished objectives/deliveries enter their retained
expired state and release their slot. No goods are automatically consumed and
partial progress earns no reward. An expired objective cannot accept a newly
captured event. Offline characters need no sweep: capture/prepare already check
expiry; commands and acceptance release their old slot on next use.

Reset never deletes history, claims, event deduplication keys or receipts. Ready
objectives and prepared delivery operations remain recoverable in their original
week, including across restart, membership changes and lost acknowledgements.
An unresolved saved occurrence blocks command reset until it is safely retried.
The ordinary kill (200), supply-order (three), delivery/gathering/bounty (two
*different definitions per family*) account/week limits remain in the existing
atomic award policy and retained ledgers. Alts, clan changes, abandonment and
rotation cannot replenish them. Shared bounty limits remain two definitions
per clan/week. There is no administrative command that clears claims or rewards.

## Phase 33: expedition definitions and world validation

`clan_expeditions.[ch]` defines a bounded version-1 catalog for Stronghold Assault
and Regional Hunt. Parsing rejects unknown/extra fields, malformed or duplicate
IDs, unbounded arrays, invalid bands, unknown reward IDs and overlapping target
prototypes. Output is published only after the entire catalog parses.

Stronghold Assault requires one 20-defender objective and two one-kill named
objectives, each with at least two designated alternatives, all in one area.
Regional Hunt requires three ten-kill categories. The validator rejects any
hunt whose alternatives permit completing all three categories in one area.
No objective requires a whole-zone clear, new instance or quest. Target lists
and fixed policy reward IDs are explicit; definitions contain no reward amounts.

The separate world-validation API requires server-owned callbacks for area
existence, target/reset existence in that area and alternative availability.
Every reference must validate and every objective must have an available
alternative. Diagnostics identify the definition, objective and bad area/mob.
Future enrollment must revalidate against the selected dataset and current
availability; boot validation alone is not admission authorization.

After world loading/renumbering, `clan_expedition_world.c` validates the optional
catalog and logs failures without enabling enrollment. Its adapter checks loaded
zones/prototypes and positive-probability mobile resets into the specified area;
prototype number prefixes are not treated as area membership. It treats valid
spawn definitions as dataset availability, not a promise that a named NPC is
currently alive. Missing catalogs or references produce explicit diagnostics.

`lib/clan/expeditions.json` supplies disabled level-24–30 Icewall examples for
both patterns using existing indexed areas 140/141. These are definition
fixtures; the broader staging roster and playability/balance review remain
Phase 38. Enrollment, stable outing IDs, progress and claims remain Phases 34–37.

## Validation

- `tests/run_clan_expeditions.py`: strict C99 ASan/UBSan parser and world validator,
  actual indexed mob/reset references, missing references/availability, quotas,
  alternatives, reward IDs, bands, atomic parse failure and multi-year rotation.
- `tests/run_clan_objectives.py`: production catalog rotation over every character
  level and twelve weeks, file-order independence, exact-boundary/idempotent SQL
  expiry, retained claims, pending old-week payout and all existing transaction,
  crash/replay, concurrent-alt and shared-bounty tests in private MariaDB.
- `tests/run_clan_deliveries.py`: exact-boundary reset, prepared-operation
  preservation, old-week recovery and prior atomic receipt/claim tests.
- Actual gathering/bounty and delivery command/checkpoint recovery regressions,
  personal source-limit regressions, Make and CMake builds.

Next: Phase 34 expedition enrollment and roster eligibility. Defaults remain
unchanged and disabled.
