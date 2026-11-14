# Runtime clan schema validation cache

SQL-authoritative clan startup enables a process-local cache of successful core
and optional-store schema validation. Offline tools and tests retain uncached validation by default.

A proof belongs to a MYSQL handle, server connection ID, and selected database.
It expires after 60 seconds. Each call still disables implicit reconnect, checks
the connection, and reads the actual selected database, SQL mode, autocommit,
and core schema version. These session/version checks are never cached. Existing
transactions, receipt checks, balances, timers and revision comparisons remain
uncached. Optional-store metadata proofs have independent cache entries; each
validator still runs its full contract on a miss. Optional version-table checks
are part of those expiring proofs.

Only successful full metadata proofs are cached. Failures invalidate the entry.
The connection pool invalidates entries before closing/replacing handles; server
connection IDs also protect against reused handle addresses. Copyover or restart
starts with an empty cache. Cache bookkeeping is mutex-protected; callers still
must exclusively own each database connection as before.

Intentional migrations should run with game writers stopped and a subsequent
cold startup. Code performing an intentional schema change must first call
`clan_repository_schema_cache_invalidate(db)` (NULL clears all entries). An
unannounced trigger/constraint change that leaves the version unchanged can take
up to 60 seconds to be detected; changing the schema version is detected on the
next check. The cache does not grant permission for online schema changes.

Verification: `python3 tests/run_clan_schema_cache.py` uses an isolated MariaDB,
ASan/UBSan, and a one-second test TTL. It covers disabled-cache defaults, warm
hits, strict SQL mode, autocommit, version/database changes, expiration, explicit
invalidation, failed validation, connection loss and reused MYSQL storage. The
measured core-check workload fell from 607 queries to 4 for three validations.
This is a query-count measurement, not a claim about total server CPU reduction.

Optional-store coverage includes buff activation/lifecycle, deliveries, objectives,
expedition enrollment/progress, and quartermaster receipts. Their proofs share the
connection identity and invalidation lifecycle of the core proof. Expiring or
invalidating the core proof discards its optional proofs too. Failed optional
proofs are never cached. Nested validators do not hold the cache mutex during SQL.

`python3 tests/run_clan_extension_cache.py` tests all seven real validators and
independent proof keys in isolated MariaDB under ASan/UBSan. Its measured workload
fell from 466 queries to 8 for one pass over the seven validators.
`python3 tests/run_clan_cached_acceptance.py` runs integrated economic and recovery
acceptance with the runtime cache enabled; the original integrated acceptance
continues to exercise default uncached behavior.
