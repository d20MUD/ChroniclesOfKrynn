# Clan progression pacing and implemented rules 2

Computed from the production character XP formula, ordinary auto-statted same-level mob XP, production per-award caps/newbie bonus, and the current game configuration (250% XP requirements). The approved ladders and smaller earning scale are implemented in rules 2/core schema 6. Running game databases and feature controls remain unchanged.

## Assumptions and units

- Ten 30-second fights take five minutes; rest/preparation takes another five. With an additional 50% AFK share, this is **30 fights per logged-in hour**. Rest and extra AFK are separate.
- Ordinary race, solo, no happy-hour/account/leadership/clan XP bonuses. Damage XP is excluded: it depends on damage events, mob HP and build. This intentionally makes the character baseline slower and the proposed reputation targets conservative. These are estimates, not actual-play observations.
- Cumulative character XP: `(L-1)^2 × (2000 + 800×(L-2)) × 250/100`. Ordinary auto-stat mob stored XP is `75×L²`; raw death credit is one third of that, followed by the actual newbie bonus and level-band caps.
- Implemented earning units: **0.25 Favor and 0.125 Influence per full qualifying kill**, both exactly representable in existing eighth-point storage. This is **7.5 Favor/hour and 3.75 Influence/hour** before caps. Other sources use the same quarter/eighth conversion; their bonuses change the complete activity schedule.
- Favor target: **2× character-leveling effort**. Clan target: **at least 10× for a reference clan of three equally active accounts**. More accounts can accelerate shared progress; time cannot be guaranteed for arbitrarily many contributors without an additional clan-wide budget.
- Keep six named Favor standings and twenty clan rank milestones. Spread those milestones across character levels 1–30. Thus ratios compare the corresponding **milestone span**, not six Favor promotions against thirty separate character promotions. The CSV also provides all thirty character-level equivalents. This mapping is implemented for the initial rules-2 release.
- Each actual rank increment is rounded **up** to a whole point. That keeps individual milestone spans at or above the requested time ratios.
- Influence rank measures **lifetime earned Influence**, not current spendable Influence. Spending on buffs must never lower rank. Rules 2 reuse the durable, non-spendable `xp_eighths` storage field as lifetime credited Influence; it is separate from the spendable `influence_eighths` balance. Legacy protocol field names remain for compatibility.

## Character leveling and rank-equivalent comparison

Times are cumulative logged-in hours, including the assumed rest and AFK. Favor/Influence columns here are thirty-level equivalents; the actual six/twenty-rank ladders below round each rank increment upward separately.

| Character level | Cumulative character XP | Character hours | Favor equivalent | Influence equivalent (3-account reference) |
| --- | ---: | ---: | ---: | ---: |
| 1 | 0 | 0.0 | 0 | 0 |
| 2 | 5,000 | 2.7 | 41 | 303 |
| 3 | 28,000 | 5.8 | 87 | 648 |
| 4 | 81,000 | 8.9 | 134 | 1,002 |
| 5 | 176,000 | 12.1 | 181 | 1,358 |
| 6 | 325,000 | 15.2 | 229 | 1,716 |
| 7 | 540,000 | 18.4 | 277 | 2,074 |
| 8 | 833,000 | 26.4 | 397 | 2,971 |
| 9 | 1,216,000 | 34.4 | 516 | 3,868 |
| 10 | 1,701,000 | 42.4 | 636 | 4,767 |
| 11 | 2,300,000 | 50.4 | 756 | 5,665 |
| 12 | 3,025,000 | 58.3 | 876 | 6,564 |
| 13 | 3,888,000 | 66.3 | 995 | 7,463 |
| 14 | 4,901,000 | 74.3 | 1,115 | 8,362 |
| 15 | 6,076,000 | 82.3 | 1,235 | 9,261 |
| 16 | 7,425,000 | 90.3 | 1,355 | 10,161 |
| 17 | 8,960,000 | 98.3 | 1,475 | 11,060 |
| 18 | 10,693,000 | 106.3 | 1,595 | 11,959 |
| 19 | 12,636,000 | 114.3 | 1,715 | 12,859 |
| 20 | 14,801,000 | 122.3 | 1,835 | 13,759 |
| 21 | 17,200,000 | 130.3 | 1,955 | 14,658 |
| 22 | 19,845,000 | 138.3 | 2,075 | 15,558 |
| 23 | 22,748,000 | 146.3 | 2,195 | 16,457 |
| 24 | 25,921,000 | 154.3 | 2,315 | 17,357 |
| 25 | 29,376,000 | 162.3 | 2,435 | 18,257 |
| 26 | 33,125,000 | 170.3 | 2,555 | 19,157 |
| 27 | 37,180,000 | 178.3 | 2,675 | 20,056 |
| 28 | 41,553,000 | 186.3 | 2,795 | 20,956 |
| 29 | 46,256,000 | 194.3 | 2,915 | 21,856 |
| 30 | 51,301,000 | 202.3 | 3,035 | 22,756 |

## Proposed six Favor standings

| Standing | Character milestone | Cumulative Favor | Added Favor | Logged hours |
| --- | ---: | ---: | ---: | ---: |
| Unproven | 1 | 0 | 0 | 0.0 |
| Recognized | 6 | 229 | 229 | 30.5 |
| Trusted | 12 | 876 | 647 | 116.8 |
| Proven | 18 | 1,596 | 720 | 212.8 |
| Distinguished | 24 | 2,316 | 720 | 308.8 |
| Exalted | 30 | 3,036 | 720 | 404.8 |

## Proposed twenty clan Influence ranks

| Clan rank | Character milestone | Cumulative Influence | Added Influence | Clan hours at 3 contributors |
| --- | ---: | ---: | ---: | ---: |
| 1 | 1 | 0 | 0 | 0.0 |
| 2 | 3 | 648 | 648 | 57.6 |
| 3 | 5 | 1,358 | 710 | 120.7 |
| 4 | 6 | 1,716 | 358 | 152.5 |
| 5 | 8 | 2,972 | 1,256 | 264.2 |
| 6 | 9 | 3,870 | 898 | 344.0 |
| 7 | 11 | 5,667 | 1,797 | 503.7 |
| 8 | 12 | 6,566 | 899 | 583.6 |
| 9 | 14 | 8,365 | 1,799 | 743.6 |
| 10 | 15 | 9,265 | 900 | 823.6 |
| 11 | 17 | 11,064 | 1,799 | 983.5 |
| 12 | 18 | 11,964 | 900 | 1063.5 |
| 13 | 20 | 13,764 | 1,800 | 1223.5 |
| 14 | 21 | 14,664 | 900 | 1303.5 |
| 15 | 23 | 16,464 | 1,800 | 1463.5 |
| 16 | 24 | 17,364 | 900 | 1543.5 |
| 17 | 26 | 19,164 | 1,800 | 1703.5 |
| 18 | 27 | 20,064 | 900 | 1783.5 |
| 19 | 29 | 21,864 | 1,800 | 1943.5 |
| 20 | 30 | 22,764 | 900 | 2023.5 |

## Calendar examples and cap/source caveats

At an illustrative two logged-in hours per day per account, before earning caps:

- Character level 30: approximately **101 days** (3.3 months).
- Exalted: approximately **202 days** (6.7 months).
- Top clan rank: approximately **3,035 days for one account**, **1,518 days for two**, or **1,012 days for three** (about 8.3, 4.2 or 2.8 years). These are combined-contribution estimates, not a requirement for everyone to play simultaneously.

**The initial analysis required scaling existing caps and rewards together.** The current ordinary-kill limit is 200 credited encounters/account/week; at the proposed kill rates that is at most 50 Favor and 25 Influence per account/week, before repeat-prototype reductions. With kills alone, Exalted would therefore need at least 61 weeks, and the top clan rank at least 304 weeks with three accounts. On the other hand, existing 50-Favor contracts and 100-Favor personal expedition rewards would become disproportionately large and could accelerate progression sharply. A 300-Influence shared expedition is twelve account-weeks of basic kill Influence at the proposed rate.

All source rewards, weekly/source caps, buff costs, stock gates and lifetime-rank accounting must be calibrated together. A consistent first conversion would scale all existing Favor rewards/caps by 1/4 and Influence rewards/caps/costs by 1/8, then recheck complete weekly activity schedules against the new ladders. This conversion is now implemented, with Influence prices rounded up; it is not claimed to preserve the kill-only pace for every source mix. Legacy stored Influence capped at 20000, while the new stored cap is 2500: rank therefore uses independent lifetime accounting.

To keep shortening easy later, the model exposes `--favor-time`, `--clan-time`, `--reference-accounts`, point rates and AFK/rest parameters independently. Changing a point unit should rescale thresholds/rewards together, rather than accidentally changing pacing. Actual runtime tuning must preserve historical receipts and follow the rules-version/configuration checks.

## Reproduce and verify

```sh
python3 scripts/model_clan_progression_pacing.py --output docs/clan_progression_pacing --game-config /home/krynn/live/mud/lib/etc/config
PYTHONDONTWRITEBYTECODE=1 python3 tests/run_clan_pacing_model.py
```

The native regression compares model values against extracted production C `level_exp` and `gain_exp_with_source` at both 100% and 250% requirements; it also checks the battle cycle and upward-rounded effort ratios.

The user has waived a staffed pilot. Mathematical modeling and automated/dev checks replace that requirement. The approved earning/rank tables are now applied and tested as rules 2/core v6. The earlier release candidate remains an unchanged historical snapshot and needs rebuilding before release review.


## Implemented tuning and cutover

`src/clan_pacing.h` contains two independent build-time percentages, both initially
100: `CLAN_FAVOR_PACE_PERCENT` and `CLAN_INFLUENCE_PACE_PERCENT`. Set either to
50 and rebuild/restart to halve its rank thresholds, rounding upward. Saved
balances and immutable receipts do not need conversion. Quartermaster standing
gates follow the Favor percentage. Prices and earning rates stay unchanged.
These percentages scale the approved 2x and 10x baselines; they are not runtime
commands and do not change existing buff durations or effects.

New qualifying sources freeze reward rules version 2. The frozen version-1
policy remains available for regression and historical receipt validation;
current-policy completion rejects pending version-1 awards. Completed historical
receipts still replay unchanged. Internal contribution weights retain their
legacy integer tuples; the authoritative award policy converts Influence by
1/8 and Favor by 1/4 before crediting. A full kill therefore grants exactly
0.125 Influence and 0.250 Favor. Sub-eighth reduced credit rounds down: a
half-credit kill grants zero Influence and 0.125 Favor. Source quotas and
account contribution bands remain conservative brakes on repeated farming.

Contract base credit is 6.250 Influence and 12.500 Favor; personal expedition
credit is 25 Favor. Shared bounty credit is 12.500 Influence and shared assault
or hunt credit is 37.500 Influence. Each credited Influence eighth also advances
rank by one eighth. Storage-full or weekly-capped Influence does not grant
phantom rank progress. Lifetime rank progress can continue above rank 20's
threshold, while rank remains 20. Spending never removes lifetime progress.

Account Favor is capped at 150/week; shared Influence at 625/week and 2500
stored. Buff tiers cost 13, 19 and 29 Influence (old prices divided by eight,
rounded up). All figures are before source/account/weekly reductions. The
30-fights/hour comparison is a kill-only baseline, not a guarantee for every
mix of contracts and expeditions. Their bonuses and the existing weekly kill
quota change actual pace; no staffed pilot observation is claimed.

Apply `sql/20261113_clan_progression_v006.sql` offline after core v5. The explicit
pre-release migration requires an unused economy and refuses nonzero balances,
contribution windows, pending awards or buff history. It never infers lifetime
Influence from old Clan XP or overwrites historical receipts. An already-used
v5 economy needs a separately reviewed reconciliation from its retained receipts
and backup. Core-v5 servers fail readiness with the new binary. No dev/live
migration, feature enablement or deployment is part of this implementation.
