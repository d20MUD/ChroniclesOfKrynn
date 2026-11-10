-- Hoopak player combat modes; safe to reapply.
START TRANSACTION;
INSERT INTO help_entries(tag,category,entry,min_level,max_level,auto_generated)
VALUES ('HOOPAK','general',REPLACE('HOOPAK COMBAT MODES

Usage: hoopak
       hoopak ranged   (or hoopak a)
       hoopak hybrid   (or hoopak b)
       hoopak melee    (or hoopak c)

A hoopak is both a melee weapon and a ranged weapon. Its sling end uses
sling bullets from an equipped ammo pouch or quiver. The hoopak setting is
saved between logins. Use hoopak without arguments to see your setting.

A) RANGED
Keep using ranged attacks, even when your opponent is within melee reach.
Use fire <target> to open combat.

B) HYBRID (DEFAULT)
Open combat at range. When your opponent closes to within 5 feet, switch
automatically to normal hoopak melee attacks. If the opponent moves out of
melee reach again and ammunition is available, resume ranged attacks.
Use fire <target> to open combat.

C) MELEE
Always use the hoopak as a normal melee weapon, even with ammunition in
your pouch. Use hit <target> to open combat.

NO AMMUNITION
In every mode, no equipped ammo pouch, an empty pouch, or no usable sling
bullets means normal hoopak melee. You can use hit <target> to start a fight
in this state. Running out of ammunition does not end your existing fight.

Hoopak melee is a normal use of the weapon: it does not receive the -8 attack
and -4 damage penalties applied to improvised melee attacks with empty
bows, crossbows, or slings.

Changing modes does not close distance. Use advance if your opponent is
outside melee reach. Advancing uses your move action and limits you to one
attack that round. Formation restrictions still apply.

See also: RANGED, AMMUNITION, FIRE, ADVANCE, FORMATION',CHAR(10),CONCAT(CHAR(13),CHAR(10))),0,1000,FALSE)
ON DUPLICATE KEY UPDATE category=VALUES(category),entry=VALUES(entry),min_level=VALUES(min_level),max_level=VALUES(max_level),auto_generated=FALSE;
INSERT IGNORE INTO help_keywords(help_tag,keyword) VALUES
('HOOPAK','hoopak'),('HOOPAK','hoopak-mode'),('HOOPAK','hoopak-modes');
COMMIT;
