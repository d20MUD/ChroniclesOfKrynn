-- Optional in-game help entries. Apply after the normal help schema exists.
INSERT IGNORE INTO help_entries(tag,entry,min_level,max_level,auto_generated) VALUES
('NEIGHBORHOODHOUSING',
'At a neighborhood entrance: house list; house buy <name>; house enter <house-id>.\r\nInside your house: house info; house guest <player>; house build <direction>;\r\nhouse title <text>; house description <text>; house extra <keyword> <description>;\r\nhouse chest <command>; house leave. Room construction creates reciprocal exits.\r\nGuests can enter; only the owner can modify the house or use chest storage.\r\nLoose floor objects do not persist across a restart. Store valuables in chests.\r\n',0,1000,FALSE),
('HOUSECHESTS',
'house chest buy; house chest list; house chest contents <id>; house chest move <id>.\r\nCustomize: house chest keywords|short|room <id> <text>.\r\nUse normal put <item> <chest> and get <item> <chest>; look in <chest>.\r\nChest IDs are keywords. Numeric selectors and all/all.<keyword> are supported.\r\nOnly the house owner can deposit or withdraw, from the room containing the chest.\r\nContents are unlimited and persist in MySQL, including nested and sheathed objects.\r\nNonpersistent items, cursed/nodrop items and money cannot be stored.\r\n',0,1000,FALSE),
('AUCTIONHOUSE',
'Visit an auction house. Use auctionhouse <command>:\r\nlist [all|mine|item type|wear slot] [keywords]; view <id>; sell <item> <price>;\r\nbuy <id>; cancel <id>; recover <expired-id>; reprice <id> <price>; collect.\r\nListings last 14 days. Prices are 1 to 99,999,999 gold. Items must be identified,\r\nunbound and transferable. Only the seller can cancel, recover or reprice.\r\nSeller proceeds are collected at login or with auctionhouse collect.\r\nThis fixed-price market is separate from the auction chat channel.\r\n',0,1000,FALSE);
INSERT IGNORE INTO help_keywords(help_tag,keyword) VALUES
('NEIGHBORHOODHOUSING','neighborhood'),('NEIGHBORHOODHOUSING','housing'),
('NEIGHBORHOODHOUSING','house'),('HOUSECHESTS','housechests'),
('AUCTIONHOUSE','auctionhouse');
