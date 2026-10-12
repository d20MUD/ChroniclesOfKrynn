# Account reward bonuses

Use `accexp bonus` to see permanent account bonuses. Buy one percentage point with:

- `accexp bonus experience` (also `xp` or `exp`)
- `accexp bonus treasure`
- `accexp bonus gold`
- `accexp bonus questpoints` (also `qp`)

Each purchase costs **2,000 account XP**. Each category caps independently at **10%**.
Maxing one category costs 20,000 account XP; maxing all four costs 80,000.
The bonuses apply to every character on the account and appear in `account`.

Experience, gold, and quest point bonuses multiply eligible positive rewards after
happyhour bonuses, rounding down to whole points or coins. Normal reward caps remain.
Negative rewards, currency exchanges, and transfers do not receive these bonuses.
Treasure upgrades add percentage points to the existing treasure drop chance,
capped at 100%: a base 5% drop chance becomes 15% with all ten upgrades.

Rewards include combat experience and corpse gold, random treasure, quest and
mission rewards, hunt rewards, and supply order rewards. The experience bonus
applies to character experience, rather than artisan or crafting skill experience.
Corpse gold uses the killer's account bonus (or the pet owner's) before looting;
shared corpse gold follows the existing group loot rules.

Purchases update the account XP balance and bonus in one guarded SQL statement.
Other logged-in characters on the account update immediately. The four account
columns are created automatically during normal database startup, with zero bonuses
for existing accounts. No purchases are made during migration.
