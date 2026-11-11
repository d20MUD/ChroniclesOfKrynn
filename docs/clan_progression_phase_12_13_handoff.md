# Clan progression: Phases 12–13 handoff

Status: complete as disabled readouts and pure buff queries. Required schema
remains version 5. No live database migration, authority switch, restart, award,
buff activation or feature enablement occurred. Phase 14 is next.

## Commands and privacy

- `clan progress [clan-incarnation-uid]` reads the character's SQL membership
  by default. It uses the authenticated descriptor account ID for account-wide
  contribution usage; there is no character-ID fallback.
- `clan favor [after-incarnation-uid]` lists only the requesting character's
  retained Favor, including disbanded clans, in pages of 25. The returned cursor
  is an immutable UID. `clan progress <uid>` opens that personal history.
- `clan inspect <character-id> <account-id> <clan-incarnation-uid>` requires
  `LVL_GRSTAFF` in both dispatcher and handler. These are explicitly separate
  inspection IDs, not a claim that a character belongs to an arbitrary account.
  Clan officer privileges cannot grant inspection access.

Accepted members can see shared balances and buff state. Applicants and former
members see their personal Favor and account usage, with shared data withheld;
an explicit former/deleted incarnation requires an existing personal Favor row.
Only authorized staff inspection bypasses this shared-data restriction. Numeric
clan vnums and playerfile mirrors do not establish identity or membership.

`clan_repository_status_read()` validates version-5 storage and SQL authority,
then reads a repeatable-read consistent snapshot. It finishes with ROLLBACK,
even on success. Missing Favor/windows are represented without creating rows;
storage corruption is not rendered as a zero balance. Output is published only
on success. Favor pagination similarly performs no writes.

`clan_status_render()` produces bounded text with exact eighth-point fractions,
level/next XP threshold, Influence, cumulative Favor/standing, account full/half
bands, source claims, UTC reset, clan weekly usage and last personal committed
award/cap explanations. MUD colors are preserved. No web UI or MSDP report is
introduced here; those remain later phases. Legacy clan points and `clan award`
are unchanged and are not progression Favor.

## Pure buff view

`clan_buff_query()` takes validated configuration, immutable clan UID, status,
level, saved family rows and explicit UTC time. It returns tiers, slots,
category limits, active state, original expiry/next-use timestamps and a free
slot mask. It performs no SQL, resource debit, activation, cancellation or stat
application. The query is independent of game characters and wall-clock reads.

All tiers in a family share its saved timer. Expiry frees the slot but preserves
the family lockout. Cancellation does the same immediately. Configuration
changes do not recompute saved timestamps. Another incarnation's timers do not
apply. Deleted clans have no unlocked or active buffs, while saved cooldowns
remain visible to authorized inspection. Duplicate families/activation IDs,
active slot collisions, locked active tiers and combat overflow fail closed.

Availability is structural only. Phase 14 must additionally validate permissions,
feature gates, eligibility and affordability, returning structured failure
reasons without spending. Later activation transactions must revalidate under
locking before any debit. Do not use the free-slot mask as authorization.

## Verification and operations

`tests/run_clan_status.py` compiles the actual repository, formatter and native
command adapters under ASan/UBSan and runs against a disposable MariaDB schema.
It covers fractional rewards/caps, account IDs, command permission checks before
SQL acquisition, historical privacy, pagination, UTC boundaries, errors and
unchanged persisted rows. `tests/run_clan_buff_queries.py` checks the actual pure
module under strict C99 and ASan/UBSan, including all levels and cooldown edges.
The CMake server build passes. Automake metadata generation passes; this
checkout's `make` cannot configure because `config.status` is absent.

Command help is in `lib/text/help/clan_progression.hlp` and the matching repeatable
`sql/20261111_clan_progression_readouts_help.sql`. The latter updates help only;
it is not a schema migration and has not been applied to the live database.
All progression feature defaults remain OFF. No deployment is part of this work.
