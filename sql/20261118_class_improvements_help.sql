-- Reapply safely to update authored command help.
START TRANSACTION;
INSERT INTO help_entries(tag, category, entry, min_level, max_level, auto_generated)
VALUES ('CLASSFEATURES', 'general', REPLACE('CLASSFEATURES

Usage: classfeatures

Shows current Bard, Duelist and Stalwart Defender feature readiness,
gear requirements, guard eligibility, recovery and damage-buffer status.
Features use actual ranks in the named class, not total level or your
preferred perk classes. They are automatic; no perk purchase is required.

Bard: Battle Rhythm at 3/9/15, Rallying Verse at 7, and an improved
Song of Revelation at 11. See help perform and help rally.
Duelist: Measured Parry at 5, Find the Seam at 6, and Opening at 8.
Stalwart Defender: Guarded Companion at 2, Measured Advance at 4,
Intercepting Guard at 6, and Shielded Recovery at 8.

Measured Parry: once per combat interval, an otherwise successful,
non-critical melee weapon hit can be parried. d20 + Duelist ranks +
Dexterity modifier must exceed the incoming attack total; ties fail.
Requires a melee weapon, a free off hand, light/no armor, no casting,
and no total defense. A success grants one Riposte attack at -2 if you
have Riposte. It cannot chain reactive attacks or use a queued ability.

Find the Seam: against critical-immune targets, eligible dueling melee
weapon hits add half Duelist ranks (maximum 5) ordinary weapon damage
instead of Precise Strike. It does not restore critical or sneak attacks,
and is not multiplied by a critical hit.

Guarded Companion: while in stance, guarding a conscious groupmate
within melee reach grants them +1 AC against weapon attacks at Defender 2,
+2 at 6, and +3 at 10. Use the best defender''s bonus, never the sum.
Use guard <groupmate>, guard for status, and guard stop to end guarding.

Measured Advance: while in stance, advance closes up to 10 feet within
the room, costing a move action and one movement point per foot moved.
At Defender 4-8 it recovers every three combat intervals. At Defender 9
there is no interval, but the action and movement costs still apply.
Normal rooting and movement restrictions apply; exits retain the existing
Mobile Defense rules. Advance toward an NPC you are fighting.

Shielded Recovery: successful Renewed Defense at Defender 8 also grants
2 points per Defender rank (maximum 20) of weapon damage absorption for
three combat intervals. The buffer absorbs final damage after mitigation,
and does not absorb spells, damage over time, reflected packets or costs.
Renewed Defense keeps its existing recovery and use limits.

One combat interval is the native six-second combat period. Short effects
expire in real time, including outside combat. New personal cooldowns
survive logout. Stance/guard/target switching does not reset cooldowns.
', CHAR(10), CONCAT(CHAR(13), CHAR(10))), 0, 1000, FALSE)
ON DUPLICATE KEY UPDATE category=VALUES(category), entry=VALUES(entry), min_level=VALUES(min_level), max_level=VALUES(max_level), auto_generated=FALSE;
INSERT IGNORE INTO help_keywords(help_tag, keyword) VALUES ('CLASSFEATURES', 'CLASSFEATURES');
INSERT IGNORE INTO help_keywords(help_tag, keyword) VALUES ('CLASSFEATURES', 'CLASS-FEATURES');
INSERT IGNORE INTO help_keywords(help_tag, keyword) VALUES ('CLASSFEATURES', 'CLASSFEATURE');
INSERT INTO help_entries(tag, category, entry, min_level, max_level, auto_generated)
VALUES ('RALLY', 'general', REPLACE('RALLYING VERSE

Usage: rally [groupmate]
Requires: Bard 7 and an active, valid group performance.
Cost: swift action. Cooldown: 30 seconds, shared across all targets.

Give yourself (default) or a conscious same-room groupmate +2 morale to
all saving throws for two combat intervals (12 seconds). At Bard 15,
the bonus becomes +3. The recipient must be able to receive the song;
deaf recipients cannot benefit. This does not remove conditions or give
an extra saving throw. Stronger morale bonuses take precedence.

Multiple verses use the strongest bonus and refresh duration; they do
not add bonuses or durations together. See classfeatures for readiness.
', CHAR(10), CONCAT(CHAR(13), CHAR(10))), 0, 1000, FALSE)
ON DUPLICATE KEY UPDATE category=VALUES(category), entry=VALUES(entry), min_level=VALUES(min_level), max_level=VALUES(max_level), auto_generated=FALSE;
INSERT IGNORE INTO help_keywords(help_tag, keyword) VALUES ('RALLY', 'RALLY');
INSERT IGNORE INTO help_keywords(help_tag, keyword) VALUES ('RALLY', 'RALLYING-VERSE');
INSERT INTO help_entries(tag, category, entry, min_level, max_level, auto_generated)
VALUES ('OPENING', 'general', REPLACE('EXPLOIT THE OPENING

Usage: opening [current opponent]
Requires: Duelist 8; light/no armor, a melee weapon and free off hand.
Cost: swift action. Cooldown: 18 seconds.

Designate your current enemy within melee reach. Your next ordinary
primary-weapon attack against that target within two combat intervals
(12 seconds) gains +2 insight to hit and +1d6 ordinary weapon damage.
The attack roll consumes the benefit even on a miss. The damage is not
precision damage and is not multiplied on a critical hit.

Only one opening can be held. Leaving the room or losing the target
ends it. Reactive attacks do not consume or benefit from Opening.
Stronger insight attack bonuses take precedence. Use classfeatures
for gear requirements and readiness.
', CHAR(10), CONCAT(CHAR(13), CHAR(10))), 0, 1000, FALSE)
ON DUPLICATE KEY UPDATE category=VALUES(category), entry=VALUES(entry), min_level=VALUES(min_level), max_level=VALUES(max_level), auto_generated=FALSE;
INSERT IGNORE INTO help_keywords(help_tag, keyword) VALUES ('OPENING', 'OPENING');
INSERT IGNORE INTO help_keywords(help_tag, keyword) VALUES ('OPENING', 'EXPLOIT-THE-OPENING');
INSERT INTO help_entries(tag, category, entry, min_level, max_level, auto_generated)
VALUES ('INTERCEPT', 'general', REPLACE('INTERCEPTING GUARD

Usage: intercept
Requires: Stalwart Defender 6. Toggles interception on/off.

While in Defensive Stance and guarding a conscious same-room groupmate
within melee reach, redirect one successful single-target weapon attack
to yourself every three combat intervals (18 seconds).

The ally''s avoidance resolves first. Interception preserves the attack''s
critical result, subject to your own immunity, and uses your normal
mitigation without a second attack roll or avoidance roll. It can kill you.
It cannot redirect again or trigger a reactive attack chain.

Spells, area attacks, maneuvers, damage over time and reflected damage
cannot be intercepted. Existing guard/rescue checks still run normally;
an attack already redirected away from the ally is not intercepted again
for that ally. Multiple defenders never intercept the same attack twice.

Use guard <groupmate> to choose the ally, guard or classfeatures to see
status, guard stop to end guarding, and intercept to disable interception.
', CHAR(10), CONCAT(CHAR(13), CHAR(10))), 0, 1000, FALSE)
ON DUPLICATE KEY UPDATE category=VALUES(category), entry=VALUES(entry), min_level=VALUES(min_level), max_level=VALUES(max_level), auto_generated=FALSE;
INSERT IGNORE INTO help_keywords(help_tag, keyword) VALUES ('INTERCEPT', 'INTERCEPT');
INSERT IGNORE INTO help_keywords(help_tag, keyword) VALUES ('INTERCEPT', 'INTERCEPTING-GUARD');
INSERT INTO help_entries(tag, category, entry, min_level, max_level, auto_generated)
VALUES ('PERFORM', 'general', REPLACE('BARDIC PERFORMANCE

Usage: perform list
       perform status
       perform [1|2] <song>
       perform stop

Use perform list to see songs you know, and perform status to see active
slots and requirements. Starting or switching costs a standard action,
or a move action with Efficient Performance. Existing performance checks,
instrument benefits, stuttering and effect durations still apply.
Invalid names, unmet requirements and missing actions leave songs running.

Master of Motifs allows two songs: an unnumbered song fills an empty
second slot while a first song is running; otherwise it replaces slot 1.
Use perform 1 <song> or perform 2 <song> to replace a specific slot.
The same song cannot occupy both slots. Switching does not refresh
Crescendo''s used flag. Perform stop ends both songs. With no argument,
perform retains its original stop-if-active / list-if-inactive behavior.

Battle Rhythm: while actually performing a valid song, gain +1 competence
to weapon attack and damage rolls at Bard 3, +2 at 9, and +3 at 15.
Applies to melee, projectile and thrown weapons, not spell/song damage.
Multiple songs do not multiply the benefit. Stronger competence bonuses
replace it; lingering song effects alone do not qualify.

Revealing Refrain: Song of Revelation at Bard 11+ also helps your group
strike visible hostile creatures through concealment. On the first
concealed weapon attack against an engaged enemy each combat interval,
the strongest eligible same-room bard attempts to overcome spell
resistance and the target makes a Will save:
  DC = 10 + half Bard ranks (rounded down) + Charisma modifier.
A failed save suppresses concealment miss chance for that bard''s
same-room group for one combat interval. One save per target per interval
prevents repeated attempts by several bards. The song must remain active.
This is divination, not a mind-affecting compulsion. It does not bypass
blocked targeting, inability to see, cover or separate incorporeal defenses;
blinking targets retain their ethereal avoidance. Existing revelation
benefits remain. Deaf group members cannot benefit from this rider.

See help rally and help classfeatures for other automatic class features.
', CHAR(10), CONCAT(CHAR(13), CHAR(10))), 0, 1000, FALSE)
ON DUPLICATE KEY UPDATE category=VALUES(category), entry=VALUES(entry), min_level=VALUES(min_level), max_level=VALUES(max_level), auto_generated=FALSE;
INSERT IGNORE INTO help_keywords(help_tag, keyword) VALUES ('PERFORM', 'PERFORM');
INSERT IGNORE INTO help_keywords(help_tag, keyword) VALUES ('PERFORM', 'PERFORMANCES');
INSERT IGNORE INTO help_keywords(help_tag, keyword) VALUES ('PERFORM', 'BATTLE-RHYTHM');
INSERT IGNORE INTO help_keywords(help_tag, keyword) VALUES ('PERFORM', 'REVEALING-REFRAIN');
COMMIT;
