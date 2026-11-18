#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
perk_test_flags=${PERK_TEST_FLAGS:-}
perk_test_dir=$(mktemp -d)
trap 'rm -rf "$perk_test_dir"' EXIT HUP INT TERM
for perk_source in src/perks.c unittests/test_preferred_perks.c; do
  cc $perk_test_flags -std=gnu90 -g -Wall -ffunction-sections -fdata-sections -I src -I /usr/include/mariadb \
    -c "$perk_source" -o "$perk_test_dir/$(basename "$perk_source" .c).o"
done
cc $perk_test_flags -Wl,--gc-sections "$perk_test_dir"/*.o -o "$perk_test_dir/test_preferred_perks"
"$perk_test_dir/test_preferred_perks"
