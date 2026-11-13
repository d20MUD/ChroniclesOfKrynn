-- Optional help publication only; does not enable purchases.
START TRANSACTION;
INSERT INTO help_entries(tag,category,entry,min_level,max_level,auto_generated)
VALUES ('CLANQUARTERMASTER CLANCOMMISSIONS','general',REPLACE('CLAN QUARTERMASTER

clan quartermaster list
clan quartermaster buy <catalog-id>
clan quartermaster commission <catalog-id>
clan quartermaster confirm <catalog-id> <purchase-id>

The quartermaster stocks a small selection of existing ordinary equipment.
List shows fixed gold prices, character and clan level requirements, Favor
requirements, proficiency coverage and item budgets. Later collections remain
unstocked until approved content is added. New purchases require current accepted
membership in the clan, enabled services and available authoritative storage.

Visit a quartermaster outside combat and timed crafting with inventory room.
Buy and commission show an exact price and a confirmation command, valid for
60 seconds. Neither preview spends anything. Copy the displayed confirmation
to purchase. A changed price or catalog requires another preview. Favor is an
eligibility requirement and is NEVER spent. Clan discounts do not change prices.

Keep the displayed purchase ID. Retrying its confirmation returns the recorded
purchase without another charge or item. A fresh preview creates a separate
purchase intent. Unconfirmed previews are lost on reconnect; preview again.
Accepted purchases recover automatically on reconnect, even if services are
subsequently disabled or you leave the clan. Storage failures may delay delivery;
commands wait for safe recovery instead of risking duplicate gold or items.

Purchased copies bind to the acquiring character through normal item binding.
They have no resale value. Binding does not grant proficiency or waive normal
class, alignment, character level, armor or weapon restrictions. Leaving a clan
does not remove the item or add a clan requirement to its use. Your retained
Favor cannot authorize new purchases while you are not an accepted member.
A committed purchase always delivers to its original recipient.

The initial catalog has a field map (Recognized, clan level 2), utility knife
(Trusted, clan level 7), and ordinary staff, sword, bow, hide armor and shield
commissions (Proven, clan level 8). The initial items have no added stats,
enchantments, activated spells or clan-dependent bonuses.
',CHAR(10),CONCAT(CHAR(13),CHAR(10))),0,34,0)
ON DUPLICATE KEY UPDATE entry=VALUES(entry),category=VALUES(category);
COMMIT;
