#!/usr/bin/env bash
# Guarded Phase-16 staging rehearsal.  It is deliberately unusable without
# explicit confirmation that the database is sanitized and non-production.
set -euo pipefail

usage() {
  cat <<'EOF'
Usage:
  LEGENDARY_PHASE16_STAGING=YES LEGENDARY_PHASE16_SANITIZED=YES \
  LEGENDARY_PHASE16_DB=<name ending _stage or _staging> \
  LEGENDARY_PHASE16_SNAPSHOT_DIR=<absolute empty directory> \
  ./scripts/legendary_phase16_rehearsal.sh [--defaults-file /path/to/mysql.cnf]

The script snapshots the named staging database and the configured save roots,
applies legendary migrations through schema v10, proves all gates remain disabled,
runs read-only diagnostics, then executes the automated regression suite.
It refuses database names that do not end in _stage or _staging.

Optional roots to include in the coherent snapshot:
  LEGENDARY_PHASE16_PLAYER_ROOT=/absolute/player-save-root
  LEGENDARY_PHASE16_OBJECT_ROOT=/absolute/object-store-root
EOF
}

defaults_file=""
if [[ "${1:-}" == "--defaults-file" && -n "${2:-}" ]]; then
  defaults_file="$2"
  shift 2
elif [[ $# -ne 0 ]]; then
  usage >&2
  exit 64
fi

if [[ "${LEGENDARY_PHASE16_STAGING:-}" != "YES" || "${LEGENDARY_PHASE16_SANITIZED:-}" != "YES" ]]; then
  echo "Refusing to run: set LEGENDARY_PHASE16_STAGING=YES and LEGENDARY_PHASE16_SANITIZED=YES." >&2
  exit 64
fi

db="${LEGENDARY_PHASE16_DB:-}"
snapshot="${LEGENDARY_PHASE16_SNAPSHOT_DIR:-}"
case "$db" in
  *_stage|*_staging) ;;
  *) echo "Refusing non-staging database name: ${db:-<unset>}" >&2; exit 64 ;;
esac
if [[ "$snapshot" != /* || -e "$snapshot" ]]; then
  echo "LEGENDARY_PHASE16_SNAPSHOT_DIR must be a new absolute path." >&2
  exit 64
fi
if [[ -n "$defaults_file" && ! -r "$defaults_file" ]]; then
  echo "Cannot read MySQL defaults file: $defaults_file" >&2
  exit 66
fi

repo=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
mysql_args=()
dump_args=()
if [[ -n "$defaults_file" ]]; then
  mysql_args+=("--defaults-extra-file=$defaults_file")
  dump_args+=("--defaults-extra-file=$defaults_file")
fi

mkdir -p "$snapshot"
trap 'echo "Phase-16 rehearsal stopped; snapshot retained at $snapshot" >&2' ERR

actual_db=$(mysql "${mysql_args[@]}" --batch --skip-column-names "$db" -e 'SELECT DATABASE()')
if [[ "$actual_db" != "$db" ]]; then
  echo "Connected database did not match requested staging database." >&2
  exit 65
fi

mkdir -p "$snapshot/files"
for root_var in LEGENDARY_PHASE16_PLAYER_ROOT LEGENDARY_PHASE16_OBJECT_ROOT; do
  root=${!root_var:-}
  if [[ -n "$root" ]]; then
    if [[ "$root" != /* || ! -d "$root" ]]; then
      echo "$root_var must name an existing absolute directory." >&2
      exit 66
    fi
    tar -C "$(dirname "$root")" -cf "$snapshot/files/${root_var}.tar" "$(basename "$root")"
  fi
done

mysqldump "${dump_args[@]}" --single-transaction --routines --events "$db" > "$snapshot/${db}.sql"
printf '%s\n' "$(date -u +%FT%TZ) phase16 staging snapshot for $db" > "$snapshot/manifest.txt"

migrations=(
  20261023_legendary_objects.sql
  20261024_legendary_transactions.sql
  20261024_legendary_earnings.sql
  20261024_legendary_smiths.sql
  20261024_legendary_effects.sql
  20261025_legendary_item_passives.sql
  20261025_legendary_release_controls.sql
  20261026_legendary_mob_bosses.sql
  20261026_legendary_smith_specproc.sql
  20261026_legendary_daily_limits.sql
)
for migration in "${migrations[@]}"; do
  mysql "${mysql_args[@]}" "$db" < "$repo/sql/$migration"
done

features=$(mysql "${mysql_args[@]}" --batch --skip-column-names "$db" -e \
  "SELECT COUNT(*) FROM legendary_features WHERE enabled <> 0")
version=$(mysql "${mysql_args[@]}" --batch --skip-column-names "$db" -e \
  "SELECT version FROM legendary_schema WHERE singleton=1")
legacy=$(mysql "${mysql_args[@]}" --batch --skip-column-names "$db" -e \
  "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='legendary_boss_registry'")
legacy_smith=$(mysql "${mysql_args[@]}" --batch --skip-column-names "$db" -e \
  "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='legendary_smiths'")
if [[ "$version" != "10" || "$features" != "0" || "$legacy" != "0" || "$legacy_smith" != "0" ]]; then
  echo "Post-migration safety check failed (schema=$version enabled_features=$features legacy_boss_table=$legacy legacy_smith_table=$legacy_smith)." >&2
  exit 1
fi

mysql "${mysql_args[@]}" "$db" < "$repo/sql/20261026_legendary_phase16_diagnostics.sql" | tee "$snapshot/diagnostics.txt"
python3 "$repo/tests/verify_legendary_release_content.py"
python3 "$repo/tests/run_legendary_security.py"
python3 "$repo/tests/run_legendary_flow.py"
cmake --build "$repo/build" -j2
trap - ERR
echo "PASS: Phase-16 staging rehearsal completed. Snapshot: $snapshot"
