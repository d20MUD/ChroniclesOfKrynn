# Builder clan gear and quartermaster services

In `oedit` enter `clan`, `rank` or `favor` from the main menu. These set
clan vnum (0..65534), minimum officer rank (0..15), and minimum Favor rank
(0..5). Zero disables that requirement. Officer rank 1 is highest: setting
rank 3 admits accepted ranks 1, 2 and 3. Favor counts upward. A rank or Favor
requirement without a clan number accepts members of any active clan.
Applicants do not qualify. All zero retains normal item behavior.

NPC shops, player stalls and the auction house check these requirements
before accepting a purchase. Normal wear and low-level equipment restoration
check them before equipping. Item requirements survive saved inventory and
estate transfers. World object records use optional `U` followed by three
integers; saved objects use `CReq`. Prototype edits update existing objects'
requirements, preserving instance enchantments. Saving a character's base
stats temporarily removes/restores gear without treating that restoration as
a fresh equip request.

In `medit` use the same three menu words for guard requirements. Enable the
`Block-Clan` NPC flag and one or more directional blocking flags. A zero clan
number disables that clan guard. Rank and Favor zero skip their individual
checks. Qualified members pass; other players are stopped. Clan checks are
independent of race/class/Ghost exemptions. Existing immortal no-hassle
movement remains available. Mobile espec names are `Clan`, `ClanRank` and
`ClanFavorRank`.

The quartermaster storefront now offers services rather than template gear.
See `help clanquartermaster` or `clan quartermaster info`. Allowances are
per character, shared across that character's clans, resetting Monday 00:00
UTC. `lib/clan/quartermaster_services.json` controls all prices and six rank
allowances and is read when commands execute. Favor is never spent.

Import `sql/20261113_clan_quartermaster_services_v001.sql` into the selected
game database before starting the matching binary. This additive migration
retains acquisition facts, weekly usage and per-weapon reroll receipts. It
does not change core schema version 6 or authority. Republish the services
help migration as well. Existing legacy gear receipts still recover through
the old coordinator; the old storefront cannot accept new gear purchases.
Browser quartermaster content points to the new text services.

Receipts freeze the selected effect and command word when confirmation is
accepted. Lower-rank eligible effects have weight `6 - minimum Favor rank`.
Incompatible types are filtered (for example, Adaptive requires a ranged
weapon, Returning requires a throwable and Vorpal requires slashing). Initial
rolls and rerolls use only currently unlocked effects. No roll is revealed
by previews. Initial enchantments do not consume the single weekly reroll.
The weapon's durable identity and SQL unique key enforce the limit across
transfers. `CEnc` persists identity, receipt, effect, state, command word,
Bane family and reroll week. Existing spell/ability lists are preserved.

Currency, motes and quest points share a player-file checkpoint. Weapon
changes and an inventory receipt share a separate inventory checkpoint.
The existing quartermaster input recovery gate prevents commands between
acceptance and acknowledgement. Recovery applies accepted facts without
rechecking membership or newly changed prices. Never delete receipts or
restore SQL and player/inventory files from different checkpoints.

Validation: `tests/run_clan_qm_services.py` uses a private MariaDB and native
production commands with independently restartable stores. It covers rank
requirements, unlocked-only rolls, per-character quotas, weekly rerolls,
activation, departure and crash/uncertain-save recovery. The legacy
quartermaster suite retains old delivery recovery coverage, and estate
codec tests verify the new item fields through file and SQL serialization.
