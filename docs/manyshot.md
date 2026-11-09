# Manyshot command and feat

The feat remains a passive two-arrow first shot on full bow attacks. The
`manyshot <target>` command for feat-only characters now executes a full
attack rather than returning an explanatory message. It requires two usable
arrows and standard/move actions; staggered characters cannot use it.
Normal automatic first attacks still set the same transient pending flag.
Actual paired shots announce Manyshot, use a shared attack roll and spend
two arrows. Precision/critical damage occurs once, and each arrow resolves
its own reduction and elemental damage. Crossbows with mixed pouches cannot
qualify by merely carrying spare arrows.

With the ranger perk, the command retains its separate 3-shot active volley,
5 with Improved Manyshot, standard-action cost and 120/60-second cooldown.
Perk descriptions now match this implementation. Volleys stop if the target
is extracted, killed or moves rooms. Owning both preserves the active perk
command and the feat on ordinary full attacks.

Tests: `tests/run_manyshot_command.py` executes native command handlers with
controlled combat dependencies; `tests/run_ranged_feats.py` runs production
eligibility, ammo commitment and the second arrow damage path. Adjacent ranged
ammo, preference and pouch-capacity harnesses cover their shared inventory path.

`fire` now dispatches an eligible Manyshot opening automatically using the
already-selected target pointer (preserving autohit and duplicate-mob targeting).
An available perk uses its volley; feat-only characters use a full attack if
standard/move actions and two arrows are available. A cooldown, insufficient
arrows/actions, or staggered condition falls back to the original single shot.
`tests/run_fire_autohit.py` verifies the fire command dispatches to this path
without an extra ordinary shot or a second name lookup, while the command
harness exercises native opening eligibility and both implementations.
