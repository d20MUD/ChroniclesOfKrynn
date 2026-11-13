-- Optional help publication; no progression enablement.
START TRANSACTION;
INSERT INTO help_entries(tag,category,entry,min_level,max_level,auto_generated)
VALUES ('CLANDELIVERIES CLAN_DELIVERIES','general',REPLACE('CLAN DELIVERIES

clan deliveries list
clan deliveries accept <id>
clan deliveries active
clan deliveries abandon
clan deliveries turnin
clan deliveries confirm <quote-id>
clan deliveries retry

Deliveries exchange specified goods for clan progression credit. You must be
an accepted member of an eligible clan. Accepting and turning in requires a
quartermaster, outside combat and timed crafting. Readouts and abandonment
are available elsewhere. Delivery contracts must be enabled by the game.

There is one active delivery slot. Contracts expire at the displayed weekly
UTC boundary. Abandoning gives no credit and consumes nothing. You may finish
two DIFFERENT delivery definitions per account per UTC week, including all
alts and clan changes. Reaccepting does not reset a completed definition.

Material contracts require exactly 50 units of the named crafting material
and grade from your stored materials. Ammunition contracts require 100 actual
remaining finite shots of the named type and minimum enhancement. The game
uses your equipped ammo pouch and directly carried bundles, splitting stacks
as needed. Infinite and Returning ammunition does not count. Other containers
and ammunition without a saved finite bundle size do not count. Purchased
and transferred goods qualify; these are delivery, not gathering contracts.

Base credit is 6.250 lifetime Influence, 6.250 spendable Influence and 12.500 Favor. Account, source, clan
and Favor caps may reduce credit. Turnin previews the EXACT credit before
charging anything. Confirm its quote ID within 60 seconds. Changed goods,
membership, expired quotes or changed credit require a new preview. An all-zero
credit cannot consume goods. Character XP bonuses never multiply this credit.

Successful delivery consumes goods and awards credit once. Recovery retries
the saved transaction after a disconnect or storage failure. A rejected paid
transaction refunds its goods before cancellation. While recovery is pending,
commands pause so reserved goods cannot be moved or spent. Retry after storage
recovers, or contact staff if the message persists. Never delete recovery
markers or reuse a transaction ID to clear a lock.

Choices rotate each Monday at 00:00 UTC, with at least two alternatives in
supported unlock/character bands. Use list for this week''s server-owned IDs
and next reset time. Rotation never changes accepted terms, replenishes an
account limit or removes a committed reward. Unfinished slots expire on your
next contract command; saved occurrences are recovered before that reset.
',CHAR(10),CONCAT(CHAR(13),CHAR(10))),0,34,0)
ON DUPLICATE KEY UPDATE entry=VALUES(entry),category=VALUES(category);
COMMIT;
