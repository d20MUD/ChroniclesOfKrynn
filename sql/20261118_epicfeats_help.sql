-- Player epic feat planner help; safe to reapply. Does not run automatically.
START TRANSACTION;
INSERT INTO help_entries(tag, category, entry, min_level, max_level, auto_generated)
VALUES ('EPICFEATS', 'general', REPLACE('EPICFEATS shows learnable epic feats alphabetically in compact columns.
You can plan at any character level, even before reaching level 21.

  epicfeats                       Compact paged list.
  epicfeats all                   Same compact list.
  epicfeats class psionicist       Class epic feat pool eligibility.
  epicfeats class artificer        Artificer class pool choices.
  epicfeats class summoner         Summoner class pool choices.
  epicfeats search ward            Case-insensitive name search.
  epicfeats known                  Your learned epic feats.
  epicfeats available              Feats you currently qualify to select.
  epicfeats info epic dodge        Full prerequisites and mechanics.
  epicfeats info <ID>              Look up a listed feat ID.
  epicfeats <name or ID>           Shortcut for info.
  epicfeats help                   Command syntax.

Each entry has an ID, four independent status markers and a feat name:
  C (cyan)     Class epic feat for at least one of your current classes.
  K (yellow)   You have learned at least one rank.
  M (magenta)  You have learned the maximum number of ranks.
  + (green)    You currently qualify to select the feat or its next rank.
A dash means the corresponding status does not apply. A known feat can also
be eligible for another rank. Waiting for a later level is not maximum rank.
Repeatable choices without a single global cap are not marked maximum.

Long names end in ~ when shortened to fit your screen. Use the displayed
ID with epicfeats info <ID> to see the full name, prerequisites and effects.
All epic feats require character level 21+. Full details include later-rank
requirements, suboptions, cooldowns and limits.
Class filters refer to eligible class epic feat point pools; general epic
points can purchase any feat whose requirements you meet. Availability does
not imply that you have an unspent point. Select and buy feats through STUDY.

See also: EPIC-EXPANSION, FEATS, STUDY', CHAR(10), CONCAT(CHAR(13), CHAR(10))), 0, 1000, FALSE)
ON DUPLICATE KEY UPDATE category=VALUES(category), entry=VALUES(entry),
  min_level=VALUES(min_level), max_level=VALUES(max_level), auto_generated=FALSE;
INSERT IGNORE INTO help_keywords(help_tag, keyword) VALUES
('EPICFEATS', 'epicfeats'),
('EPICFEATS', 'epic-feat-planner');
COMMIT;
