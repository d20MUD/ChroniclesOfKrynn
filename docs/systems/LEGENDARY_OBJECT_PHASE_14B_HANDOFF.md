# Interim phase 14B — native combat balance completion

Status: resumed; native combat encounter validation remains open. Do not mark the full phase-14 combat
balance gate complete from screening results.

## Completed and authorized (do not ask again)

- Phase 13 security, durable local journal with count/SHA-256 integrity,
  process-interruption recovery, corpse/storage/commerce/deletion fixes and
  sanitizer verification passed. Phase-13 SQL was v5; the additional item-passive migration now requires v6. Journal format remains LGB1.
- The owner authorized economy modeling at **20 level-30 kills/hour**, plus
  **five total level-30 items/hour sold for 10% of their value**. No route
  measurements are required in place of that explicit model.
- The owner selected option 1 for both review questions: **reduce purchase and
  reshape gold costs to 10%**, and **accept the scripted first-use walkthrough
  as this phase's usability review**. Do not repeat either approval question.
- Approved rank gold costs are 2,500 / 5,000 / 10,000 / 20,000 / 40,000;
  cumulative V = 77,500. Reshape gold fees are 1,000 / 1,500 / 2,500 / 1,000 /
  5,000. XP, materials, acquisition caps and independent channel parity are
  unchanged. Definition version is now 3 (additional item passives). Existing recorded investment is not
  rewritten/refunded, and durable operations recover their exact saved deltas.
- `infuse next <item> [channel]` and a short no-argument prompt are implemented.
  Exact prices and walkthrough are tested through real command/SQL fixtures.
- Economy scanner and fixed-seed screening exporter are implemented. At median
  rewards with a 50/50 random/regular item mix, estimated XP/hour is 450,000 and
  gold/hour is 8,357.5. New V time is 8.56 XP hours and 9.27 gold hours/channel.

## Files and reproducible entry points

- `tests/run_legendary_balance.py`: reads live world/config files without edits,
  compiles `tests/legendary_catalog_export.c` against production definitions,
  exports economy and synthetic sensitivity CSV/JSON.
- `tests/capture_legendary_native_profiles.py` and
  `tests/legendary_native_capture.c`: link actual CMake server objects, boot
  private copies of world/player/object data, force all MySQL connections to
  disposable MariaDB, block application Internet sockets and cURL, never open a
  game port/start a game loop. No credentials or player names are exported.
- `docs/systems/legendary_balance_results/native_snapshots.json`: 38 native
  pseudonymized level-30 snapshots with baseline/control/augmented probes.
- `tests/run_legendary_flow.py`: full rank/modifier regression chain plus the
  guided first-use walkthrough. Writes `legendary_balance_results/player_flow.txt`.
- `tests/run_legendary_security.py`: complete isolated persistence/economics,
  actual process deaths, source awards, native packet/corpse routes, fuzz,
  journal faults, budgets and performance checks.
- `LEGENDARY_OBJECT_PHASE_14_REVIEW.md`: review context and owner-approved prices.

## Work completed after resuming

- Damage inspection consumed Dark Revelation against the player's own sentinel
  mobile ID, explaining the first/control drift. Display calculations now never
  consume Dark Revelation or Blackguard Reprisal, and Dark Revelation requires
  an actual NPC target with a non-sentinel mobile ID.
- The native fixture requires identical same-seed baseline/control damage, and
  explicitly checks that self/real-target display probes leave Dark Revelation
  unconsumed. Original 38 snapshots recaptured with repeatable probes.
- Owner explicitly chose **an additional passive alongside the foundation**.
  Implemented 16 rank-scaling choices, different slot menus, shared outfit caps,
  single primary-track selection with paid supporting-rank contributions, and
  quote/confirm replacement (first free, then 1,000 gold/5 fragments).
  See [full specification](LEGENDARY_ITEM_PASSIVES.md). Existing foundations,
  powers, prices and earnings limits remain. Schema migration v5→v6 is explicit;
  no production database migration has been applied.
- Integration coverage exercises all 16 passives at I–V, native adapters, trained
  skills/resource capability, caps, removal, SQL persistence, invalid supporting
  selections and two-handed parity. Include this layer in future balance runs.

## Remaining work, in order

1. **Retain paired controls in every encounter fixture.** Display-probe drift
   is fixed and repeatability is asserted. Display probes remain diagnostics,
   not substitutes for genuine 30-second/600-second combat and caster rotations.
2. Select or construct **seven legal representative level-30 builds**: two-handed
   martial, sword/shield, dual wield, ranged, offensive caster, psionic and
   healer/support. Validate gear/grips/class capabilities and saved snapshot
   completeness. Raw archived snapshots can contain missing or overlapping
   hand equipment; class labels alone are not valid role coverage.
3. Run genuinely paired **30-second and 600-second native encounters**, including
   full nine-channel outfits, two-handed supporting channels, passive-only
   allocations, actual active rotations and three-source group stacking.
   Respect native action recovery, criticals, saves, defenses, caster resources,
   legitimate targets, ordinary caps and all rolling budgets. Actual spell
   rotations cannot be replaced by weapon-only caster tests or free spell calls.
4. Check **<=5% sustained outgoing increase and <=8% incoming reduction** on each
   approved build. The bundled Python sensitivity profiles are synthetic and
   flag 20/28 screens; this is an unresolved finding, not native-playtest proof.
   Uncapped +1 attack can exceed the relative 5% threshold; existing native caps
   and typed deflection may suppress some effects, but this needs measurement.
5. If a real balance rule must change, update production definitions, independent
   tests, the proposal and player guide together; explain the change for review.
   Do not silently nerf bonuses or relax the thresholds to make tests pass.
6. Rerun the appropriate native integration/security/flow suites and supported
   builds, record numerical evidence, then close phase 14 only when the combat
   gate actually passes. User approval of pricing/usability is already recorded.

## Constraints and environment

- Repo `/home/krynn/code`, cwd `/home/krynn/code/src`.
- Preserve the large dirty/untracked working tree from all prior phases.
- Never edit live world/zone/quest files under `/home/krynn/live/mud/lib/world`.
- No production DB writes, deploy, commit, server restart or live player actions
  have been performed. Native captures operate only on copies and isolated SQL.
- CMake build directory: `/tmp/krynn-legendary-phase8-cmake`.
- Phase 13 validation logs: `/tmp/legendary-phase13-sanitizer-final.log`,
  `/tmp/legendary-phase13-engine-final.log`, `/tmp/legendary-phase13-build-final.log`,
  `/tmp/legendary-phase13-make-final.log`.
- Phase 14 current logs: `/tmp/legendary-phase14-final-tests.log`,
  `/tmp/legendary-phase14-cmake-final.log`, `/tmp/legendary-phase14-make.log`,
  `/tmp/legendary-native-capture.log`.
- No sub-agents were authorized; continue without delegation.

## Latest verification for the additional passive layer

- `/tmp/legendary-item-passive-flow-final.log`: first-use flow now includes the
  extra free passive selection; full foundation/technique/signature/rank chain passed.
- `/tmp/legendary-item-passive-sanitizers.log`: ASan/UBSan security suite passed,
  including all sixteen new passives at I–V and existing crash/recovery checks.
- `/tmp/legendary-item-passive-build-final.log`: CMake build passed.
- `/tmp/legendary-item-passive-make-final.log`: Make build passed without warnings.
- `/tmp/legendary-item-passive-native-final.log`: native display/actual-
  attack regression capture passed for 38 copied level-30 snapshots.
- Economy artifacts include the authoritative new passive catalog. Their combat
  screen explicitly excludes additional passives and remains nonauthoritative.
  No full native encounter result or completed phase-14B gate is claimed.

## Native combat progress after the request to finish 14B before phase 15

See [native probe protocol and results](LEGENDARY_OBJECT_PHASE_14B_NATIVE_PROBES.md).
Two independently controlled melee snapshots passed preliminary outgoing checks.
A provisional low-damage bow setup increased from 1,142 to 1,714 HP damage in
600 seconds (+50.09%, but only +0.953 DPS). This is not an approved endgame
archer and not a substitute for the seven-role suite. The owner was asked whether
the 5% gate is for representative geared role builds or every legal level-30
build. Do not silently change the fixed bonuses or accepted percentages.

The fixture now supports `--probe`, `--candidate`, `--role` and `--debug`, enforces
legal hands/proficiency for melee and native bow proficiency/Point Blank Shot
for its equipped ranged setup, prioritizes the actual 12-point foundation plan,
and checks baseline/control equality. An earlier target-flat-footed fixture flaw
was corrected before recording these results. No phase-15 content edits,
production SQL changes, deployment or restart have been performed.

Logs: `/tmp/legendary-phase14b-warrior-aware.log`,
`/tmp/legendary-phase14b-assassin-aware.log`,
`/tmp/legendary-phase14b-ranged-probe.log`,
`/tmp/legendary-phase14b-probe-build.log`.

## Owner decision on benchmark scope

The owner selected **representative geared role builds; retain fixed bonuses**.
The 5% target is not a universal cap for weak fallback weapon setups. Do not
repeat this question, nerf the fixed bonuses to fit the fallback result, or
raise the agreed 5%/8% targets for the representative suite. The provisional
low-damage bow result remains a diagnostic case, not a failed representative
endgame-archer gate. Build the remaining fixtures with ordinary native endgame
gear and real class/role capabilities.

## Credit-limit checkpoint — stop requested by owner

The owner explicitly requested wrapping up because credits are nearly exhausted.
**Phase 14B is NOT complete. Phase 15 is deferred and has not been implemented.**
Do not spend the next turn rechecking approvals or treating these probes as the
full acceptance suite.

### Decisions that persist

- Use **representative geared role builds** for the 5%/8% gates; keep the fixed
  bonuses. Weak fallback-weapon setups are diagnostics, not the representative
  gate. Do not re-ask that question or silently nerf Accuracy/Force to fit them.
- The owner requested finishing 14B before 15, then explicitly deferred phase 15
  for now. Continue only 14B when asked to resume unless they change that scope.
- All earlier pricing, usability, daily-reward and additional-passive decisions
  remain in force. No production migration or world edit is authorized/performed
  by these preparation and testing tasks.

### Completed this session

- Added a test-only `LEGENDARY_NATIVE_PROBE` observer in `fight.c`; the ordinary
  build excludes it. Native outgoing probes execute real attack phases, HP
  damage, mitigation, crits/procs, timers and NPC deaths in isolated copies/SQL.
- Corrected the target-awareness fixture, initialized native PC queues/lookup
  entries, enforced legal melee hands/proficiency, prioritized the actual
  12-point foundation allocation, and checked independent baseline/control
  equality. See the native-probe report for scope and corrected numbers.
- Completed two corrected melee probes at 30/600 seconds. They stay within the
  outgoing target, but do not cover full outfits, active rotations or incoming
  damage. The lower-damage bow/weapon-only-upgrade probes are explicitly
  provisional and do not validate a representative endgame archer.
- Built **seven draft native role blueprints** through native advancement,
  premade level-1–20 selections and common grade-six generated gear. All seven
  generate successfully. `eligible_items=8` on two-hand/ranged roles means eight
  objects; adding their independently paid supporting track should produce nine
  channels. Other roles have nine eligible objects.
- `tests/legendary_native_builds.h` and `legendary_native_blueprints.c` are fixture
  code only. Blueprints still need a feat/prerequisite audit (especially the
  ranged feat substitutions and campaign-human elective budget), caster slot/
  power preparation, complete legendary outfitting and the actual encounter
  suite. Do not claim those checks are complete just because generation succeeds.
- Native generation must use `DL_RACE_HUMAN` under this campaign; classic
  `RACE_HUMAN` becomes undefined during native initialization. This is handled.
- Ordinary CMake build and the existing full flow/integration chain passed after
  adding the observer. No production balance numbers were changed this session.

### Current files/artifacts

- `tests/capture_legendary_native_profiles.py`: `--probe`, `--candidate`,
  `--role`, `--geared` (weapon-only upgrade, not a validated geared role),
  `--blueprints`, `--debug`. Every run uses disposable MariaDB and private data.
- `tests/legendary_native_probe.c`: outgoing-only actor scheduling; existing
  targets/roles are diagnostics. All fixture time/observer hooks are test-only.
- `tests/legendary_native_builds.h`: successful draft generation for all seven
  roles. Keep source ordinary caps, feat budgets and wear restrictions intact.
- `docs/systems/legendary_balance_results/native_blueprints.json`: draft stat/
  eligible-object captures, explicitly not encounter results.
- `native_gate_probe.json` / `native_assassin_gate_probe.json`: corrected aware-
  target melee results. The report records exact values and trajectory caveats.
- `native_ranged_gate_probe.json` / `native_geared_ranged_probe.json`: fallback
  bow and weapon-only upgrade diagnostics; neither is the approved ranged build.

### Resume next

1. Audit and finish the seven representative blueprints, including legal role-
   specific feats and ordinary gear; do not relabel the fallback rogue as a
   completed dedicated ranged build.
2. Equip all nine legendary channels, including two-handed supporting ranks,
   new per-item passives, and three valid selected powers. Verify effective
   bonuses/eligibility so default priority order cannot silently suppress a
   foundation intended for the test.
3. Implement and run the 30-second/600-second native **full** suite: passive-only
   and active rotations, actual prepared/spontaneous/psionic resource expenditure,
   healer/support actions, incoming reductions and three-source group stacking.
   Define controlled encounter health/AI conditions explicitly; outgoing-only
   stationary targets are not a substitute for that suite.
4. Use reproducible paired controls; account for crit/proc trajectory noise after
   a changed hit/kill consumes different random draws. Do not silently waive
   the agreed representative-build limits or nerf fixed bonuses without review.
5. Run relevant regressions/builds, publish the complete numerical results and
   close 14B only if its actual gate passes. Phase 15 remains deferred.

Latest logs: `/tmp/legendary-phase14b-blueprints.log` (seven roles generated),
`/tmp/legendary-phase14b-probe-build.log` (CMake passed),
`/tmp/legendary-phase14b-probe-regression.log` (full flow/integration passed).
The older sanitizer run predates these new fixture files; do not claim the new
fixture suite itself was sanitizer-validated.

## Phase 15 preparation resumed after the checkpoint

The owner later authorized resuming phase 14B and phase 15, with the instruction
to use the first recommendation rather than request a choice. Phase 14B remains
open: no full seven-role 30/600-second suite has yet established the 5% outgoing
and 8% incoming gates.

Phase 15 now has a reviewed, non-live release package:

- Schema v7 adds `legendary_features` with independent fail-closed `earnings`,
  `purchases`, and `major_effects` controls, all disabled on migration. Awards,
  smith writes/exchanges and major activation respectively check the relevant
  control; read-only inspection remains possible while purchases are disabled.
- Schema v8 removes the obsolete MySQL boss registry. The existing smith mobile
  10500 (Lord Orelor, room 8795) remains SQL-configured; each approved boss is
  instead opt-in via its native `Legendary-Boss` MEDIT flag. Eligible deaths roll
  a durably recorded essence type. The release SQL does not mutate world files
  or enable any feature gate.
- `tests/verify_legendary_release_content.py` is a read-only preflight that
  validates every selected identity and reset room against the live world tree.
  It passed for the smith and all twelve bosses.
- No true-dragonlance prototype is verifiably present in existing object text.
  The prepared registry has no lance entry; dragonlance-only powers stay locked
  rather than granting them to ordinary lances. This is an explicit unresolved
  content prerequisite, not a placeholder assignment.
- In-game help and Markdown guide now describe independent release controls,
  schema v8, Lord Orelor and the verified flag-based candidate package.

Verification after these additions: `git diff --check`, Python compilation,
CMake build and `LEGENDARY_SECURITY_TEST=1 python3 tests/run_legendary_security.py`
passed. The latter exercises v7 migration through the full persistence,
transaction, earning, equipment, power and command stack. No production
migration, world edit, deployment or restart occurred.
