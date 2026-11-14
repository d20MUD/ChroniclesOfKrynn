# Dual wielding hand crossbows

Use `wield <crossbow>` for each hand crossbow in turn. The second fills the
offhand slot, subject to the normal hand, size, level and equipment restrictions.
Other ranged launchers cannot be paired, and ranged/melee weapon pairs remain
restricted.

A full ranged attack gains an offhand shot with the normal two-weapon attack
penalties. Improved, Greater and Perfect Two-Weapon Fighting add their offhand
attacks. Spending the move action or being staggered still permits only the
normal single attack. Each shot uses its own crossbow's statistics and effects,
and consumes ammunition from the equipped bolt pouch.

Each crossbow tracks its loaded state separately. With autoreload enabled the
server attempts to reload both, using the normal reload actions; Rapid Reload
makes hand-crossbow reloads free as before. Manual `reload` readies the primary
crossbow first, then the empty offhand crossbow when the primary is already
loaded. No new saved item format or SQL migration is needed.

Validation: `tests/run_dual_hand_crossbows.py` exercises native selection,
pair restrictions, independent loads, attack phases, feat attacks, ammo
exhaustion, target death, report modes and selection cleanup. Existing ranged
feat, throwing, hoopak, empty-launcher and single-attack checks also pass.
