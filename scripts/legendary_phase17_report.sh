#!/usr/bin/env bash
# Read-only Phase-17 aggregate report. Use a SELECT-only MySQL account.
set -euo pipefail

usage() {
  cat <<'EOF'
Usage:
  LEGENDARY_PHASE17_READONLY=YES LEGENDARY_PHASE17_DB=<database> \
  LEGENDARY_PHASE17_OUTPUT=/absolute/report-directory \
  ./scripts/legendary_phase17_report.sh [--defaults-file /path/to/mysql.cnf]

The report contains aggregate operational metrics only. It does not select
character names, player IDs, item UUIDs, targets, or recovery payloads.
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
if [[ "${LEGENDARY_PHASE17_READONLY:-}" != "YES" ]]; then
  echo "Refusing to run: set LEGENDARY_PHASE17_READONLY=YES." >&2
  exit 64
fi
db="${LEGENDARY_PHASE17_DB:-}"
out="${LEGENDARY_PHASE17_OUTPUT:-}"
if [[ ! "$db" =~ ^[A-Za-z0-9_]+$ || "$out" != /* ]]; then
  echo "Specify a simple database name and an absolute output directory." >&2
  exit 64
fi
if [[ -n "$defaults_file" && ! -r "$defaults_file" ]]; then
  echo "Cannot read MySQL defaults file: $defaults_file" >&2
  exit 66
fi

repo=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
args=()
if [[ -n "$defaults_file" ]]; then args+=("--defaults-extra-file=$defaults_file"); fi
mkdir -p "$out"
test -d "$out"
actual_db=$(mysql "${args[@]}" --batch --skip-column-names "$db" -e 'SELECT DATABASE()')
if [[ "$actual_db" != "$db" ]]; then
  echo "Connected database did not match requested database." >&2
  exit 65
fi
stamp=$(date -u +%Y%m%dT%H%M%SZ)
report="$out/legendary-phase17-$stamp.txt"
mysql "${args[@]}" --batch --raw "$db" < "$repo/sql/20261026_legendary_phase17_observability.sql" > "$report"
printf 'Wrote aggregate Phase-17 report: %s\n' "$report"
