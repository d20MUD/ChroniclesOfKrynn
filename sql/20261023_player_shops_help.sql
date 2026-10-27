-- Install/update the player-facing stall help entry during deployment.
INSERT INTO help_entries(tag,category,entry,min_level,max_level,auto_generated)
VALUES ('STALL','general',REPLACE('STALL STALLS PLAYERSHOP PLAYER-SHOPS

PLAYER SHOPS

Rent a market stall to sell your gear at prices you choose. Your stall stays
open while you are offline, as long as its rent is paid.

FINDING A MARKET

  stall directory                 Show market locations and stall numbers
  stall list                      Show stalls in your current room

Visit a market and use stall list to check availability, weekly rent and item
capacity. The usual rent is 1,000 gold per seven real days, with room for 20
items. Rent and capacity can vary between stalls.

RENTING YOUR FIRST STALL

  stall rent 3                    Rent stall #3 in your current room
  stall name The Rusty Dragon     Give your stall a name
  stall info                      Check your lease and unsold stock

You need enough carried gold to pay the rent. Each account may rent one stall
at a time. Only the character who rents it can manage its stock and collect
its earnings. Stall names may contain up to 80 characters.

SELLING YOUR GEAR

Carry the item you want to sell, then visit your open stall:

  stall sell sword 25000          Offer your sword for 25,000 gold
  stall info                      Find its listing number
  stall reprice 12 18000          Change listing #12 to 18,000 gold
  stall withdraw 12               Take listing #12 back into your inventory

Choose a price from 1 to 99,999,999 gold. There is no sales commission.
Stocking removes the item from your inventory and places it in your stall.
Its customizations and enchantments remain on it. Items stay listed until
sold, withdrawn, or your stall closes.

Items must be identified, unbound and transferable. You cannot stock quest
items, money, or items marked no-rent, no-sell or no-drop. Empty containers
and remove weapons from sheaths before selling them.

Stall numbers and listing numbers are different. A stall number identifies
the shop; a listing number identifies one item. Use the listing number shown
by stall info, stall browse or stall search when managing or buying an item.
The numbers in these examples are examples only; use the numbers you see.

COLLECTING YOUR GOLD

  stall earnings                  Show uncollected sales proceeds
  stall collect                   Collect your proceeds

Earnings and collection work from anywhere, including after your lease ends.
You do not need to be online when someone buys an item.

SHOPPING AT OTHER PLAYERS'' STALLS

  stall search sword              Search all open stalls for matching items
  stall browse 3                  List the items at local stall #3
  stall inspect 12                Examine the properties of listing #12
  stall buy 12                    Purchase listing #12

Search works from anywhere and shows the market location. Travel to that
market to browse, inspect or buy. Purchases require enough carried gold and
room in your inventory. You cannot buy your own listings.

RENEWING OR CLOSING YOUR STALL

  stall info                      Check when your rent expires
  stall renew                     Pay for another seven days
  stall close                     Give up your stall

Visit your stall to renew or close it. Renewing early adds seven days to your
existing expiry time. Rent uses real time and continues while you are offline.
The stall''s current weekly rate applies when you renew.

When rent expires, sales stop immediately. You have 48 hours to renew before
someone else can rent the stall. Renewing during this grace period starts a
new seven-day lease from the time you renew. You can also close during grace.
Closing does not refund unused rent.

Your unsold items and earnings remain yours after closing, expiration or
staff eviction. A new tenant cannot access them. From anywhere, use:

  stall info                      Find your old unsold listings
  stall withdraw 12               Reclaim one of those items
  stall collect                   Collect outstanding earnings

IF A TRANSFER IS PENDING

Interrupted transfers are recovered when you next log in. If a payment would
exceed your gold limit, make room in your gold balance and use stall collect
again. A stored payment message confirms gold paid or received. If the game
reports that your player data cannot be saved, contact staff before trying
more trades.

STAFF: REMOVING A MARKET ROOM

Senior staff can remove every stall in a room with:

  stall admin delete-room <room vnum>

Use the room number, not a stall number. The world room remains intact.
The market disappears from the directory and room display; sales stop and
all tenant reservations are released. Owners keep unsold items and earnings
and can use info, withdraw and collect from anywhere. Rent is not refunded.
Removed stalls cannot be re-enabled; admin add creates new stalls if needed.
Transaction history remains available through admin history <old stall ID>.

Type stall for a quick command reminder.

See also: AUCTIONHOUSE, BUY, SELL
',CHAR(10),CONCAT(CHAR(13),CHAR(10))),0,1000,FALSE)
ON DUPLICATE KEY UPDATE category=VALUES(category),entry=VALUES(entry),
min_level=VALUES(min_level),max_level=VALUES(max_level),auto_generated=FALSE;
INSERT IGNORE INTO help_keywords(help_tag,keyword) VALUES
('STALL','stall'),('STALL','stalls'),('STALL','playershop'),('STALL','player shops'),
('STALL','player shop'),('STALL','player-shops');
