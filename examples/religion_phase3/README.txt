PHASE THREE KRYNN CONTENT

Use this complete zone 699 version INSTEAD OF examples/religion_phase2, never
install both versions. Check all vnums before adding its wld/mob/qst/zon files
to the corresponding test-server indexes. The setup.txt commands configure the
two registered churches and optional task rotations. Existing week snapshots
remain stable; new tasks appear in the next week's snapshot.

Archives require temple rank 3; deeper chambers require rank 6; sanctums require
rank 8. Inward keeper checks do not block outward exits. All task targets are
public rooms. These rooms add roleplay space, not new economic rewards.

After the phase 3 schema migration, import wardrobes.sql for 4,704 published
Krynn appearances (21 deities x 8 ranks x 4 styles x 7 positions). Stable IDs use
300000..329999. INSERT IGNORE leaves existing builder revisions untouched. The
service checks deity names too, so these records cannot match different deities
at the same numeric IDs in another campaign. They do not create equipment.

Authored source: wardrobe_themes.json plus build_wardrobes.py. Rebuild with:
  python3 examples/religion_phase3/build_wardrobes.py

Builder review: restringedit -> Religion -> a rank -> C <deity-id> for coverage,
then E <entry-id> to preview/edit a published appearance. Player-visible options
only include valid published matches. See docs/RELIGION_PHASE3.md.
