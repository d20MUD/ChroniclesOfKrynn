# Phases 24–25: buff commands and shared combat eligibility

Both phases are implemented without enabling live progression or issuing rewards.
Existing core-v5 and buff activation/lifecycle schema extensions are reused. No
live migration, deployment or restart occurred.

## Phase 24: player commands

`clan buffs` / `clan buffs list` shows authoritative accepted-membership progression,
slot/category state, unlocked tiers, prices, active expiration and family next-use
times. Active entries include the activation UID used for cancellation/replacement.
Family IDs 1–5 follow the pilot library order.

Commands:

```
clan buffs activate <family 1-5> <tier 1-3> <slot 1-3>
clan buffs replace <family> <tier> <slot> <current-active-uid>
clan buffs cancel <activation-uid>
clan buffs permission <maximum-rank>
```

Zero permission means leader-only; positive N permits accepted ranks 1 through N.
Only the current SQL-recorded leader, with current accepted membership and enabled
buff policy, can change this setting. The rank must exist. Management remains the
separate stable privilege ID 21 in `clan_buff_management`; legacy privilege IDs and
arrays are not renumbered. The dedicated permission command integrates the editor
operation without changing the legacy OLC serialization. Revocation takes effect
on the next locked service check. Staff status grants no bypass. Leaders can reset
an old delegation after rank changes; authorization never relies on the old
management threshold being a valid current rank.

The adapter calls the existing activation/cancellation transaction services. It
never changes Influence, timers or runtime bonuses itself. It uses the selected
trusted runtime policy, not a command-created live configuration. Normal commands
resolve the incarnation from current SQL membership and require an authenticated
account. Disabled defaults remain disabled.

Each mutation prints an exact retry command containing a generated operation UID
and explicit clan UID before calling the service:

```
clan buffs activate <family> <tier> <slot> <operation-uid> <clan-uid> [expected-active-uid]
clan buffs cancel <active-uid> <operation-uid> <clan-uid>
```

An occupied slot requires its expected active UID, not an inferred/automatic
replacement. These fully specified commands are also the retry format for an
ambiguous commit. Exact replay returns the original receipt without a second
charge, even after departure or feature disablement; altered operation reuse
conflicts. Explicit clan/operation input is not authorization: the locked services
validate ownership, membership, permissions, unlocks, resources, cooldowns,
replacement and replay proof. No player command enables configuration gates. Mutations also require checked lifecycle storage before any activation debit; a missing or altered lifecycle extension fails closed.

Receipts show original price and UTC expiration/next availability; cancellation
refunds nothing and preserves original cooldowns. Rejection flags are rendered as
individual explanations. Rehydration runs only after releasing the owned SQL
connection, never from invalidation callbacks while it is held.

Player help is supplied in `docs/help/clan_buffs.hlp` for the normal help import process; the live help database was not modified.

## Phase 25: shared combat-contribution query

`clan_combat_query` is a pure bounded query over trusted occurrence facts. Inputs
carry a stable encounter UID, enemy prototype/level/start maximum HP, legitimate
death and exclusion flags, occurrence room/times, actor/controlled-owner records,
authoritative player/account/incarnation/member state, and effective action facts.
There is no character-XP, legendary eligibility, killing-blow or payout input.

The query:

- Rejects non-world/player, invalid-death, owned/charmed/summoned/pet, test,
  staff-test and manipulated victims.
- Resolves controlled actors through bounded owner chains; cycles and corrupt
  indices fail closed. Pets contribute toward their player owner, never separately.
- Includes participating outsiders and controlled owners when computing H. A
  victim must be at least max(1,H−3), and a recipient within five levels of H.
- Requires living, actively playing, in-room accepted members with real account
  identity and a valid immutable clan incarnation.
- Counts actions in the encounter room in the inclusive preceding 120 seconds.
  It clamps amounts to actual available damage/missing-health/preventable damage,
  excludes repeated application packets, and uses encounter-start victim HP and
  non-clan ally maximum HP as the appropriate threshold bases.
- Accepts 5% damage, 10% effective healing to an engaged ally, 5% effective
  interception/mitigation for an engaged ally, two genuinely useful support/defense
  applications in distinct rounds, or two distinct damaging hostile attacks
  successfully received while engaged. Healing/mitigation totals are per ally:
  one actual ally must meet its threshold, rather than mixing HP denominators.
- Does not count mere grouping, arrival, overhealing, or a non-improving effect.
  The trusted collector must mark useful actions only after a real state change
  and supply stable application IDs rather than network-packet identities.
- Chooses the eligible lowest character ID as deterministic representative when
  multiple characters share an account; an ineligible alt does not suppress a
  qualifying one. No extra account reward can come from controlled creatures.

Thresholds come from the validated pilot configuration. Integer thresholds round
up and bounded totals saturate safely. Invalid facts leave all output entries
unchanged; normal ineligibility returns explicit reason flags per actor. Owner
resolution is precomputed, and arrays/action counts are strictly bounded. This
same result interface is intended for ordinary contributions, bounties and
expeditions; it has no level-30/boss restriction.

The live source adapters must freeze these facts from authoritative encounter and
participation data, not from command/browser amounts or modified XP returns.
**Phase 26 remains the entry point for connecting live kill/source collection and
receipt-backed ordinary contribution awards.** This phase does not start a second
legendary ledger, query SQL on every attack, or enable a combat reward hook.
Prototype/day farming counters, account limits and exactly-once reward receipts
belong to that subsequent award integration and the existing reward services.

## Validation

- Disposable-MariaDB tests compile the actual command adapter with ASan/UBSan and
  exercise listing, malformed/overflow input, account requirement, disabled gates,
  rank delegation/revocation, activation/cancellation, exact retry, conflicting
  operation reuse and departure/disabled replay, alongside previous lifecycle,
  rollback/crash/retry and runtime-effect regressions.
- `tests/run_clan_combat.py` exercises production eligibility with short and long
  fights, exact threshold/recency boundaries, healers/defenders/support, repeated
  packets, overkill/overheal, mixed levels/grouped outsiders, max-level owners and
  multiple controlled creatures, account deduplication, inactive/applicant and
  victim exclusions, owner cycles and integer bounds under ASan/UBSan.
- Native buff query/activation and prior read-only status regressions, Make and
  CMake builds.

Next: Phase 26 qualifying ordinary-kill contribution integration. Configuration
and live SQL authority must still be explicitly enabled through the trusted setup.
