# Clan progression: Phase 10 handoff

Status: complete as disabled personal reward policy and receipt integration.
Next is Phase 11 membership/lifecycle eligibility. No gameplay hook, migration,
live award, deployment or feature enablement occurred; schema stays at version 5.

## Personal reward policy

`clan_award_personal_project()` is a pure version-1 decision using accepted event
facts and the same account, clan, Favor and occurrence-week snapshot supplied by
the locked receipt coordinator. Invalid input leaves its output unchanged.
`clan_award_complete_personal()` invokes this decision inside the existing atomic
receipt transaction, with earnings/campaign/schema/pilot gates. The shared-only
Phase 9 endpoint remains restricted to shared sources.

The order follows the Phase 2 specification:

1. Validate the frozen source reward tuple and account/character/incarnation/week
   keys. Enforce the source's account-global weekly claim limit: kills 200,
   supply orders 3, deliveries/gathering/bounties 2 each, personal expeditions 2.
2. For positive-XP personal sources, pay the first 600 base XP fully, the next
   600 at half credit, and subsequent contributions at zero. Split crossing
   events exactly; apply the same reduction to Influence and Favor. A supply
   order after 550 base XP credits 75 XP and 37.5 Influence/Favor, charging
   100 base XP and one claim. Source-eligible events still charge their claim
   and original base XP when another cap truncates the payout. An already
   exhausted source charges neither again.
3. Feed account-adjusted incoming XP/Influence into Phase 9's clan projection.
   Clan XP/Influence reductions do not reduce Favor. Influence and Favor caps
   do not reduce otherwise allowed XP.
4. Limit realized personal Favor to 600 per account/week, across all characters
   and clans. Charge only credited Favor to that allowance. Expedition personal
   Favor skips the base-XP bands while retaining its source limit and Favor cap.
5. Commit all balances, counters and the terminal decision together, including
   zero outcomes. Replay returns that exact decision without new awards.

Ordinary kills may carry the full, half, or zero configured tuple frozen by a
trusted source adapter. Half credit is for the specified prototype farming band,
not an invented level multiplier. A zero-source kill gets a terminal receipt
without charging a source claim or base-XP usage. Daily per-prototype tracking,
actual combat eligibility and accepted contract/expedition identity/family quotas
remain later source phases; amount validation does not establish eligibility.
No client/officer command can submit these trusted reward facts.

Version-1 eligible base XP is always a whole point (including a half-credit
2-XP kill). The policy rejects fractional base-usage corruption, preserving
exact eighth-point arithmetic through source/account/clan reductions rather
than silently rounding Favor. Positive cumulative Favor addition is checked
against INT64_MAX; any failure rolls back the whole SQL operation.

## Standings, explanations and previews

`clan_award_standing()` derives the six configured standings from cumulative
Favor, including fractional progress and values above Exalted. It neither
spends Favor nor checks current membership or clan-level stock gates. The
existing character/incarnation Favor repository supplies distinct current and
former-clan records; Phase 11 governs access, and later quartermaster phases
combine standing with acquisition eligibility.

Receipt explanation bits 1–8 retain Phase 9 meanings. Added version-1 bits are
16 (account bands), 32 (source claim limit), 64 (weekly Favor allowance), and
128 (source reduction). `clan_award_explanation()` returns a stable text reason
for each individual flag; the receipt preserves requested amounts, actual
credit, usage, before/after balances and all applicable flags. No text-command
or MSDP/UI readout is connected until its planned phase.

The pure decision supports exact advisory previews. A preview does not reserve
allowance; commit always recalculates against locked counters. Future manual
resource turn-ins must confirm reduced amounts, reject a completely zero payout
without consuming goods, and enforce that decision atomically with their
recoverable escrow/outbox protocol. The current SQL-only endpoint is not a
resource-consumption authorization. Those adapters remain disconnected pending
their source/file-recovery phases.

## Recovery and runtime boundaries

Rules and occurrence weeks remain frozen on acceptance. Recovery after a weekly
reset uses the original retained counters, not a fresh allowance. COMMIT_UNKNOWN
requires loading/retrying the same operation/event IDs; terminal receipts replay
even after earnings is disabled. No compensating awards or file mutations are
introduced. Runtime callers must refresh the Phase 9 progression cache after OK
or an unknown acknowledgement before serving cached progression. Ordinary clan
saves retain their automatic refresh and revision checks.

Membership/account ownership at occurrence, departure/expulsion/deletion policy,
source eligibility and source-specific definition uniqueness remain trusted
adapter responsibilities. Phase 10 does not enable rewards or claim that these
later integrations already exist.

## Verification and next entry point

- `tests/run_clan_awards.py`: strict C99/ASan/UBSan checks of production clan and
  personal policy; exact full/half/exhausted boundaries, each source allowance,
  expedition independence, source reductions combined with account/clan bands,
  all Favor standing thresholds, explanation bits, malformed identities/rules,
  fractional-base rejection and cumulative/counter overflow.
- `tests/run_clan_personal_awards.py`: real production SQL composition in private
  MariaDB; all staged rollbacks, lost acknowledgement and replay, alternate
  characters/clans sharing limits, original-week delayed recovery, exact
  fractional rewards, zero payouts, independent caps, concurrent last eligible
  encounter, bad tuple refusal and atomic Favor-overflow rollback.
- Prior receipt/process-crash/concurrency, persistence/cache/save and campaign
  gate regressions; CMake and Automake builds.

Next: Phase 11 integrates joining/leaving/expulsion/deletion and access queries,
retaining old Favor and immutable-incarnation isolation without transferring
benefits or accepted claims to another clan. Keep all default gates OFF.
