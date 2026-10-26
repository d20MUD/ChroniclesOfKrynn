# Phase 10: live techniques

Implemented 2026-10-24. All T01–T18 now use the phase 9 activation engine through
`infuse use <ID|quoted name> [target]`. This phase does not enable signatures.
Existing equipment ownership, attunement, three-power allocation, ordinary caps,
source leases, cooldowns and rolling budgets continue to apply.

## Catalog adapters

The callbacks use `legendary_resolve_power` directly. Inspect/choices generate
numeric rank/modifier values from that same catalog; no second balance table
was introduced. Base values apply at II–IV, with the final replacement at V.

| ID | Runtime integration |
| --- | --- |
| T01 Measured strike | The next actual ordinary weapon attempt gains +2 attack (+3 at V), subject to ordinary attack caps, then consumes even on a miss. A positive eligible hit adds a noncritical 10 damage (15 at V) after ordinary defenses. It obeys effective resistance/immunity and the 60/60s offensive allowance; overkill is not charged. |
| T02 Steady hands | +3 (+4 at V) on both sides of actual opposed trip, disarm and shove checks for 18s (24s at V). Applies to the generic CMB/CMD roll and legacy knockdown/counter-trip rolls. |
| T03 Opening | A weapon hit within 12s creates one target mark for 12s. Another same-room group member's next ordinary weapon attempt gains +2 attack (+3 at V) and consumes the mark even on a miss. The owner's attacks do not consume that mark. One mark exists per target. |
| T04 Assess | Reports the target's real fire/cold/acid/electricity/sonic resistance, including vulnerability, and registered hard-control protection. At V also reports slash/pierce/bludgeon reduction. The control label identifies the registered boss property; other immunity depends on the specific condition. |
| T05 Brace | One physical packet within 12s loses 20%, capped at 30 prevented damage (40 at V), after ordinary defenses. |
| T06 Hold ground | +4 (+6 at V) on actual checks defending against trip/forced movement for 18s (24s at V), without automatic immunity. |
| T07 Interpose | One ally's next physical packet within 12s redirects 20%, capped at 30 (40 at V), to the source. Source HP cannot fall below one; unshifted excess remains on the ally. No extra defenses or recursive triggers apply to the transfer. |
| T08 Recover footing | Stands a recipient with a genuine recoverable knockdown marker. It cannot cure voluntary sitting, gravity-well immobilization, pinning, grapple/entangle, stun/paralysis, unconsciousness or existing recovery. Ordinary standing opportunity attacks still apply. At V also provides +2 against trip checks for 12s. |
| T09 Field dressing | Restores up to 30 HP (40 at V), limited by missing HP, ordinary healing penalties and the recipient's shared healing allowance. |
| T10 Resolve | Removes the oldest supported timed Fear/Dragonfear if at most 12s remain, otherwise shortens that record by exactly 12s. V uses 18s. Other fear records, permanent and scripted/explicitly locked conditions remain. |
| T11 Clean wound | Removes exactly one oldest supported nonmagical bleed and heals up to 10 HP (20 at V), with normal penalties/recipient limits. Does not remove poison, disease, curse, profane or psionic bleeding. |
| T12 Trail stance | Final ordinary walking cost is reduced by 20%, minimum one, for 30s (45s at V). Chooses the strongest legendary reduction against F15 rather than multiplying them. No mounted or free-movement refund. |
| T13 Pursuit mark | Tracks one legal enemy for 30s (45s at V). `infuse track` reports only the same room or an immediately adjacent visible quarry through a structurally traversable open exit. Moving the source ends it; the quarry may move into an adjacent room. No travel or visibility bypass occurs. |
| T14 Coordinate | One ally's next ordinary weapon attempt OR supported real skill roll within 12s gains +2 (+3 at V), then consumes. Stat/display queries, crafting, counters, saves and spell DCs do not consume it. |
| T15 Failed weave | An eligible paid spell/power failing Concentration before delivery restores 20% of actual points paid, capped at 10 (15 at V), within an 18s preparation. Shares the 20/180s resource allowance and actual pool maximum. |
| T16 Focused reserve | The next eligible paid spell/power costing at least 20 points within 18s costs five fewer (eight at V), minimum final cost one. Shares the real resource-savings allowance with refunds. |
| T17 Breath brace | One actual dragon's registered native breath packet within 18s loses 20%, capped at 40 (50 at V). Ordinary fire damage, non-dragon racial breath and later components do not falsely consume it. |
| T18 Veiled retreat | Static +3 Stealth (+4 at V) for 18s (24s at V), applied through the real ability computation used by hiding/perception. Grants no invisibility, free flee, concealment or disengagement. |

A blocked/missed or zero-damage packet does not create an Opening mark. Measured
strike's explicit bonus is a separate nonrecursive component following the
ordinary packet's defenses/legendary ward resolution, consistent with the shared
bonus-origin contract. It cannot be multiplied by criticals, vulnerability or
backstab and does not grant extra lifesteal or participation actions. Its source
UUID/channel/version and active attunement are checked again at damage delivery.
Ordinary attacks retain all their existing bonuses when no technique applies.

## Modifiers, actions and lifetimes

Every catalog-permitted Ready, Enduring and Distant Aid modifier is active.
Ready uses the pure resolver's rounded individual cooldown; family/global timing
is unchanged. Enduring extends the preparation window before a later effect's
separate lifetime: Opening's 12s mark does not become an 18s mark. Distant Aid
requires another visible, willing, same-room group ally; it does not allow self
as the substituted target or bypass Recover footing's restrictions.

Activation checks the native standard-action event as well as command wait,
casting, position and incapacity. It starts a six-second standard-action recovery
after durable cooldown commit. A longer wait cannot be shortened. The sole
sitting exception is Recover footing with a genuine marker, not the spell that
uses `SKILL_TRIP` as a placeholder for immobilization.

Prepared/timed benefits use the existing UUID/channel source leases and second
clock. Unequip, invalid grip/capability, unattunement, death, disconnect and
extraction cancel them. Allied bonuses/wards end on room/group separation.
Prepared effects are not recreated after reconnect, and cooldowns/allowances
remain durable. `infuse track` is a read-only report, with no second activation.
It neither runs movement triggers nor rolls Athletics/spends movement to find a
route. Hidden exits remain concealed; darkness, fog, closed doors, zone/housing
restrictions, terrain access and tunnel capacity are checked. Structural route
eligibility does not guarantee passing a later movement trigger or skill roll.

## Conditions and saved characters

Native knockdown paths mark a genuine recoverable fall; normal standing and death
clear that runtime marker. T08 leaves all unrelated affects and recovery intact.

Fear durations are native combat rounds, with duration zero lasting until the
next affect update. The adapter tracks that update's deadline to compute actual
remaining seconds. Shortened fear receives an absolute expiry override, serviced
by the one-second heartbeat and honored instead of native round decrements.
Only the selected record is affected. The save reader/writer retain the absolute
expiry and locked provenance; an already expired record is not restored on load.

The supported bleed records are `SKILL_BLEEDING_ATTACK`,
`ABILITY_BLEEDING_CRITICAL`, `SKILL_TIGER_CLAW` and `EVOLUTION_BLEED_EFFECT` carrying
`AFF_BLEED`. Script casts and direct DG affects carry `LEG_CONDITION_LOCKED`.
Native mechanics may explicitly set that flag for a boss-locked condition;
a caster's own control immunity does not automatically make its attacks locked.
Tail-most matching insertion defines oldest. Saving/restoring affect rows now
preserves that application order rather than reversing it on each save.

Two optional fields are appended to saved affect rows: locked provenance and
absolute expiry. New code still reads the existing 5/7/8/9/10/14-field formats.
This is a character-file format extension; an older binary is not a compatible
reader for newly written metadata rows. No live player files were rewritten by
this implementation or its disposable tests.

The native player writer temporarily removes and re-equips gear/affects. A save
scope now defers source reconciliation until restoration, keeping valid leases
and current pools intact through this transient operation. Foundations still
strip normally for the saved base data. After restoration, real invalidations
(including gear rejected by a wear trigger) reconcile normally. Save failures
use the same restoration path; a nested writer cannot enter the transient scope.
An actual unequip continues to cancel preparations immediately.

## Paid spell/power accounting

Normal manifest input computes ordinary class discounts and normalizes unusable
augmentation before queuing a unique in-memory payment receipt. A read-only
remaining-budget quote allows an otherwise affordable discounted power without
spending allowance during validation. Actual payment occurs after native cast
prerequisites/target checks and immediately before Concentration; the locked
payment revalidates affordability and saves only the amount actually deducted.

Only a real paid cost of at least 20 qualifies for Failed weave. Focused reserve
checks the eligible cost before its own final flat discount. Repeated casting
ticks do not pay twice. The Concentration failure branch runs the refund once,
after the native reroll opportunity; deafness fumbling, cancellations, resisted
spells and casts already delivered do not qualify. Delivery is marked at the
normal immediate/finished-cast entry points, so a weapon/item proc cannot falsely
claim delivery of an outstanding cast. Reset, interruption and character cleanup
release the receipt. A second cast cannot replace the receipt of an ongoing cast.

Vancian/prepared slots with no PSP payment, free casts, item casts and unrelated
class abilities do not invent point refunds, discounts or spell slots. Existing
ordinary casting costs/augmentation rules are retained; unsupported casting
entry points that do not use the normal point-payment path gain no resource
benefit. No combat effects or paid-cast receipts are replayed after restart.

## Validation

Run `python3 tests/run_legendary_techniques.py` for the live catalog fixture,
`python3 tests/run_legendary_effects.py` for the separate phase 9 synthetic engine
fixture, and `python3 tests/run_legendary_rules.py` for pure rules.

The live fixture compiles the production catalog, activation/effect engine,
technique adapters, equipment/source cache, identity, SQL and object codec against
disposable MariaDB. All eighteen powers exercise base/V values, every permitted
modifier, invalid/valid targets, individual cooldown values, timing and source
loss. Specific cases cover attack miss/hit consumption, mark ownership, source
version changes, safe redirection, actual healing, condition selection/expiry,
standard-action locks, traversal/visibility, walking stacking and paid/refunded
points. The actual player affect writer block and loader are extracted verbatim
for metadata/order/legacy/expired-row round trips. Production-hook placement is
checked alongside full server builds. Ordinary combat dice, world/policy
functions and stat recalculation use the existing fixture adapter; these tests
are not a running-server combat simulation. Leak checking is disabled for the
fixture's deliberately retained mock world, while ASan/UBSan remain enabled.

Validation passed: the live technique suite under ASan/UBSan, the separate
phase 9 engine regression suite (including earlier phases), pure rules tests,
Make and CMake server builds, and `git diff --check`. Final builds produced no
compiler warnings. Expected SQL fault-injection and quarantine messages in the
fixtures are not test failures.

Schema remains v5; no SQL migration is needed. No live world/quest data changed,
no live database was migrated, and the MUD was not restarted. Signatures and the
later full staging/balance/release phases remain outstanding.
