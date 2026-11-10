# Clan progression — Phase 3 handoff

Status: complete. Foundations only; no deployment or database changes.

The server now builds `src/clan_progression.c`, with its public interface in `src/clan_progression.h`. The module has no world, character, database, or event side effects. Existing clan structures and persistence remain unchanged.

## Contracts for subsequent phases

- Persistent enum IDs are explicit. Preserve them across migrations; look up buff families and reward sources by ID rather than array position.
- Clan incarnation IDs are nonzero, 32-character lowercase hexadecimal strings. Numeric clan vnums remain separate. UID generation and database uniqueness belong to later persistence work.
- Balance fields use signed 64-bit eighth-points. Configuration uses whole points; use `clan_progression_points_to_eighths()` for checked conversion. Negative or overflowing conversions fail without modifying the output.
- Data types cover clan identity/progression, family cooldown/activation state, character Favor, account contribution windows, and clan weekly windows. These are contracts for future repository work, not serialized records today.
- `clan_progression_defaults` contains the Phase 2 version-1 levels, slots, buff tiers/prices/timing, Favor standings, source budgets, contribution caps, participation thresholds, and contract quotas.
- `clan_progression_config_validate()` returns a structured error and an optional static reason. It checks IDs, thresholds, units/overflow, slots, categories, tier ordering, cooldowns, reward ratios, capacities, reset rules, and contract limits. Future configuration loaders must validate before use.
- `clan_progression_feature_available()` rejects disabled/shadow modes, unsupported campaigns, invalid configuration/identities, absent or mismatched schema readiness, non-MySQL authority, and clans outside the pilot/public release. Content-dependent features require additional readiness.
- Readiness must come from trusted repository/content checks. A caller-supplied readiness structure is not schema detection. Feature availability also does not replace membership, permissions, balances, participation, or claim eligibility checks.
- Rules/schema version constants are expected contracts only. No SQL schema or configuration loader exists yet. The injectable clock type is available for future time-dependent logic; this phase runs no timers.

## Default behavior

Progression OFF, storage LEGACY, every feature flag false, public release false, and an empty pilot allowlist. No reward, buff effect, command, startup migration, or web output is connected. Forgotten Realms and default campaign builds reject live feature availability, even if explicitly configured on.

## Verification

- `python3 tests/run_clan_progression_config.py`: strict C99 warnings plus AddressSanitizer/UndefinedBehaviorSanitizer, compiling the production module for Dragonlance, default, and Forgotten Realms campaigns. Covers defaults, malformed/duplicate IDs, invalid configurations, overflow boundaries, lookup after table reordering, and fail-closed release gates.
- `python3 tests/run_clan_login_notifications.py`: existing clan regression passed.
- `cmake --build /tmp/luminari-move-action-build -j4`: full server build passed.

Next: implementation plan Phase 4, versioned MySQL schema migrations with repeatable validation in a disposable database. Do not enable progression before the persistence, recovery, and gameplay prerequisites are implemented.
