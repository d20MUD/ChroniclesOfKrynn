-- Equipped ammunition listing; safe to reapply.
START TRANSACTION;
INSERT INTO help_entries(tag,category,entry,min_level,max_level,auto_generated)
VALUES ('AMMO','combat',REPLACE('AMMO - EQUIPPED AMMUNITION

Usage: ammo
       ammo <ammo keywords>
       ammo none

Type ammo with no arguments to show your equipped ammo pouch or quiver
and all visible ammunition inside it, including remaining quantities.
An empty pouch or a missing equipped pouch is reported explicitly.

With arguments, ammo works like ammopref: select preferred ammunition
using its keywords, or use none or auto to restore automatic selection.

See also: AMMOPREF, AMMUNITION, RANGED',CHAR(10),CONCAT(CHAR(13),CHAR(10))),0,1000,FALSE)
ON DUPLICATE KEY UPDATE category=VALUES(category),entry=VALUES(entry),min_level=VALUES(min_level),max_level=VALUES(max_level),auto_generated=FALSE;
INSERT IGNORE INTO help_keywords(help_tag,keyword) VALUES ('AMMO','ammo');
COMMIT;
