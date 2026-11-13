# Finished crafting deliveries

Clan deliveries retain material and ammunition contracts and now also support
finished recipes. The catalog adds twelve five-item requisitions: daggers,
quarterstaves, longswords, longbows, clothing, leather chest armor, small shields,
boots, gloves, cloaks, flutes and drums. Weekly rotation and existing clan-level
requirements still apply.

`CLAN_DELIVERY_CRAFTED` (kind 3) stores the stable crafting recipe index in the
existing definition's `resource` field. Definitions retain their existing JSON
shape, with grade and minimum enhancement zero and quantity bounded to 1–20.
The runtime validates the actual recipe registry. Weapon and armor subtypes,
instrument types and clothing wear slots determine matches; editable item names
do not. Items must carry the crafted flag and be ordinary, unenhanced, unbound,
directly carried goods. Contents, sheathed items, spell-bearing/special-ability,
legendary, estate-reserved and quartermaster goods are excluded. Materials and
ammunition retain their original matching rules.

Whole items use the existing saved `CDel` marker (amount 1) and inventory receipt.
Reservation leaves the original object intact. A committed SQL reward consumes
the reserved objects before the durable inventory acknowledgement; a rejected
reward clears reservations without reconstructing or duplicating items. Recovery
uses the frozen accepted definition, not the current catalog. Player commands
remain blocked while economic recovery is unresolved.

Text and web lists show recipe requirements. The existing web goods counter and
preview/confirm action support the new kind without a browser protocol change.
The reward remains 12.5 Favor and 6.25 Influence before caps; all delivery kinds
share the existing two-different-definitions-per-account weekly allowance.

No SQL schema migration is required. Deploy the compatible binary and expanded
catalog together, then reload the dev MUD with `copyover`. Do not replace the
compatible binary with one that cannot interpret accepted kind-3 contracts.

Validation uses disposable MariaDB and ASan/UBSan: SQL definition/receipt tests,
all 17 catalog definitions against the actual recipe registry, matching/exclusion
checks, all existing material/ammo recovery cases, and finished-item recovery at
five file boundaries, failed checkpoints, SQL commit/ack uncertainty, capped
refunds, reconnects and repeated retries. Web/help tests and both builds pass.
