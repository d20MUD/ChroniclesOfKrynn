# Clan text interface

The text commands use case-insensitive names, including full names containing
spaces and unique partial names. Exact names take precedence. Ambiguous names
request a more specific name instead of selecting an arbitrary action. Existing
internal keys remain accepted for compatibility and recovery; normal listings
and command hints use names.

Examples:

```
clan progress
clan progress Knights of Solamnia
clan favor 2
clan contracts gathering accept Fresh ash harvest
clan deliveries accept Ash shaft supplies
clan deliveries turnin
clan deliveries confirm
clan quartermaster buy Abanasinia field map
clan quartermaster confirm
clan expeditions create Stronghold Assault: Icewall Castle
clan expeditions join
clan expeditions claim Stronghold Assault
clan buffs activate Scholar 1 1
clan buffs replace Arcane Readiness 1 1
clan buffs cancel Arcane
```

Progress and Favor have colored headings, grouped balances and weekly limits.
Lists separate names, requirements, prices and status onto readable lines.
Dates use calendar dates in UTC rather than epoch seconds. Expedition rosters
show character names. Normal displays omit character, account, incarnation,
operation and catalog identifiers. Staff inspection retains explicit identifiers.

Confirmation without an identifier uses the current character's saved preview;
it does not create a new purchase or delivery identity. Expired or changed
previews are still rejected. Buff replacement resolves the slot's active identity
and submits that expected identity to the original checked service. An uncertain
buff commit still prints its exact recovery command, preserving replay safety.
Named history searches use only the character's retained Favor. Shared balances
remain restricted to current accepted members.

No economy thresholds, reward rules or web application code changed. The status
command now renders the installed runtime feature policy rather than OFF defaults.
The delivery command also retains ownership of its SQL connection through expiry
checking before releasing it.

Validation: name matching/date unit checks; status/privacy/history, objective
commands, delivery recovery, buff lifecycle/commands, expedition progress/claims,
expedition enrollment, expedition reconnect recovery, quartermaster purchase
recovery/commands and web/help publication suites. Both CMake and Make builds pass.

Dev deployment: binary and six help topics staged on port 4301, including help in
`krynn_dev`. Existing sessions were not interrupted. Load the staged binary with
an in-game `copyover`. Previous files and help tables are backed up under
`/home/krynn/dev/clan-ui-dev-backup-wgltxf21`. Production was not modified.
