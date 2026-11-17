# Stealth passage through directional blockers

Builders can enable `Block-Stealth` in a mobile's `medit` flags. Set at least
one existing directional blocking flag as well (`Mob-Block-North`, for example).
The new flag is `MOB_BLOCK_STEALTH`, appended at bit 110 without renumbering
existing flags. Existing mob-file flag serialization and OLC saving handle it.
No existing mobile is opted in automatically. Rebuild the server and enable the
flag on intended blockers in `medit`. Apply
`sql/20261117_block_stealth_help.sql` to install the idempotent database help
entry; the matching source help is `lib/text/help/block_stealth.hlp`. The help
migration has not been applied to the live database.

A player affected by Sneak or Hide may attempt to pass an opted-in blocker.
The game rolls the player's Stealth against that guard's Perception using the
normal skill-roll function. Stealth must exceed Perception; ties favor the
guard. Failure blocks movement. Invisibility alone does not qualify. Without
`Block-Stealth`, hiding and sneaking do not bypass directional blocking.

Both success and failure are fixed for 300 real seconds from the first check.
Repeated attempts, different blocked directions, toggling Sneak/Hide, and
respawns or copies of the same mobile prototype reuse the result. The key is
player plus blocker mob vnum; another player or another prototype gets its own
check. A cached success still requires Sneak or Hide on each attempt. The timer
is not extended by retries. Every blocking guard must allow passage.

`No-Block-Bypass` disables this exception. `Block-Clan` retains its existing
mandatory membership/rank/Favor access rules. Existing race, class, level,
alignment, Ghost and staff exemptions retain their normal behavior; this flag
does not change them.

Live check results are written to the normal player file as repeated records:

```text
BStl: <guard mob vnum> <absolute expiry timestamp> <passed: 0 or 1>
```

This preserves the remaining timer across normal logout, login and copyover.
Offline time counts toward expiry. Expired results are discarded on checks
and saves; invalid, expired or excessively future-dated records are ignored
on load. Records without this tag remain compatible. Memory is released when
the character is freed.

Validation:

```sh
python3 tests/run_blocker_stealth.py
python3 tests/run_clan_guard.py
```

The tests execute the production guard loop, cache helpers and player-file
save/load blocks with address and undefined-behavior sanitizers. They cover
opt-in behavior, Hide and Sneak, invisibility, ties, success and failure reuse,
exact expiry, per-player/prototype isolation, multiple guards, clan and hard
block precedence, persistence, invalid records and cleanup.
