-- Optional help publication only; no gameplay or player-data changes.
START TRANSACTION;
INSERT INTO help_entries(tag,category,entry,min_level,max_level,auto_generated)
VALUES ('CLANOVERVIEW','general',REPLACE('CLAN OVERVIEW

Clans are groups of adventurers who work together, build a shared identity,
and earn benefits for their members. You can contribute through adventuring,
clan jobs, and group outings.

GETTING STARTED

  clan list                See the available clans.
  clan info <clan>         Learn about a particular clan.
  clan apply <clan>        Ask to join; a clan officer must accept you.
  clan progress           View your clan''s progress and your standing.
  clan favor              View your standing with current and former clans.

Your clan grows through eligible activities. Influence helps pay for shared
clan buffs. Favor is your personal standing with a clan: it is earned, never
spent, and remains recorded if you leave. Some rewards and activities have
weekly limits; clan progress shows your current limits.

THINGS TO DO

  clan buffs                     See shared bonuses available to your clan.
  clan contracts bounty list     Browse hunting contracts.
  clan contracts gathering list  Browse gathering contracts.
  clan deliveries list           Browse supply and crafting deliveries.
  clan expeditions list          Browse group adventures.
  clan quartermaster list        See services and rewards available to you.

Visit a clan quartermaster for activities and services that require one.
The detailed help topics explain requirements, rewards, and limits:

  help clanprogression    Clan growth, Influence, Favor, and weekly limits.
  help clanbuffs          Shared bonuses and how to manage them.
  help clancontracts      Bounty and gathering contracts.
  help clandeliveries     Supplying materials and crafted goods.
  help clanexpeditions    Organizing outings and collecting rewards.
  help clanquartermaster  Purchases, services, and equipment improvements.',CHAR(10),CONCAT(CHAR(13),CHAR(10))),0,1000,FALSE)
ON DUPLICATE KEY UPDATE entry=VALUES(entry),category=VALUES(category),min_level=VALUES(min_level),max_level=VALUES(max_level),auto_generated=FALSE;
INSERT IGNORE INTO help_keywords(help_tag,keyword) VALUES
('CLANOVERVIEW','clanoverview'),
('CLANOVERVIEW','clan overview'),
('CLANOVERVIEW','clan system');
COMMIT;
