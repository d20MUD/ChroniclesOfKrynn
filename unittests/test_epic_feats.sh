#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
epic_test_flags=${EPIC_TEST_FLAGS:-}
epic_test_dir=$(mktemp -d)
trap 'rm -rf "$epic_test_dir"' EXIT HUP INT TERM
for epic_source in src/epic_feats.c src/epic_feats2.c src/epic_feats3.c src/legendary_techniques.c unittests/test_epic_feats.c; do
  cc $epic_test_flags -std=gnu90 -g -Wall -ffunction-sections -fdata-sections -I src -I /usr/include/mariadb \
    -c "$epic_source" -o "$epic_test_dir/$(basename "$epic_source" .c).o"
done
cc $epic_test_flags -Wl,--gc-sections "$epic_test_dir"/*.o -o "$epic_test_dir/test_epic_feats"
"$epic_test_dir/test_epic_feats"
