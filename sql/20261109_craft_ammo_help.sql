-- Craft ammunition help; safe to reapply.
START TRANSACTION;
INSERT INTO help_entries(tag,category,entry,min_level,max_level,auto_generated)
VALUES ('CRAFTAMMO','general',REPLACE('CRAFT AMMO

  craft ammo type <arrows|bolts|bullets>
  craft ammo material 1 <material name>
  craft ammo material 2 <material name>
  craft ammo material 3 <material name>
  craft ammo enhancement <0-8>
  craft ammo damage <none|fire|cold|acid|electricity|sonic> [die]
  craft ammo show
  craft ammo cost
  craft ammo begin
  craft ammo reset
  craft ammo kit

Configure your recipe, review it with show or cost, then begin crafting.
Selections are saved between logins. Selecting a type restores its default
materials but keeps your enhancement and damage selections. Reset restores
ordinary arrows and all default settings. Configuration and previews cost
nothing; only begin attempts to craft your saved recipe.

Every successful batch produces ONE ammunition item with 50 uses, whether
arrows, bolts, or bullets. Unlimited and returning ammo cannot be crafted.

THREE MATERIAL SLOTS
  Slot 1: arrow/bolt shafts (wood); bullet bodies (stone or metal).
  Slot 2: heads or cores (metal).
  Slot 3: binding (cloth).

Use crafting material names, including multiword names such as maple wood
and cold iron. Only materials with a crafting grade are accepted.
Default amounts, with your selected material substituted for each slot:
  Arrows:  2 ash wood, 1 iron, 1 hemp.
  Bolts:   1 ash wood, 2 iron, 1 hemp.
  Bullets: 3 stone, 1 iron, 1 hemp.
If two slots use the same material, their quantities are added together.
For example, iron bullet bodies and iron cores require four iron in total.

MATERIAL QUALITY AND DIFFICULTY
Each material contributes its grade, regardless of the quantity consumed.
The material enhancement bonus is the average of the three grades, rounded
down, minus one. The final enhancement is the greater of that bonus and
your requested enhancement; the two bonuses do not stack.

Each grade above 1 in each slot reduces the base crafting DC by 1. The base
DC starts at 11 and cannot drop below 5. Magical upgrades are added AFTER
this floor: enhancement above the material bonus adds its normal crafting
level adjustment (3.75 per point, rounded down); bonus damage adds its die
size. Better materials lower DC; magical upgrades make the batch harder.

Examples of grades:
  Wood: ash 1, maple 2, mahogany 3, valenwood 4, ironwood 5.
  Metal: tin/copper 1, bronze 2, iron 3, steel 4, mithril 5,
         adamantine 5, dragonmetal 6.
  Binding: hemp 1, linen/flax 2, wool 3, silk 4, satin 5.

ENHANCEMENT AND BONUS DAMAGE
Enhancement above material quality costs fire motes, discounted by the
material''s enhancement tier, with at least one mote for an upgrade.
Bonus damage adds +1d4, +1d6, +1d8, +1d10, or +1d12. Omitting the die uses
6. Choose none to remove bonus damage. Damage consumes matching motes:
fire, ice for cold, water for acid, lightning for electricity, air for sonic.
Fire damage and enhancement costs are added together.

The item level is the normal enhancement level adjustment plus the damage
die size, with a minimum of 1 and maximum of 30. Show displays the resulting
bonus, damage, DC, material quantities, and mote costs before you spend.

PORTABLE FLETCHING KIT
Use craft ammo kit to make a reusable portable fletching kit. This requires
2 ash wood, 2 bronze, and 2 hemp; DC 15 woodworking; an equipped woodworking
tool and a carpentry table. Materials are consumed even if the roll fails.

Carry the finished kit directly in your inventory (take it out of any bag).
It replaces both the tool and station requirements for crafting all three
ammo types, so you can craft while travelling. It does not replace the
workshop needed to make another kit, nor grant portable crafting for other
items. The kit is reusable; ammo materials and motes are still consumed on
every attempt. Without a kit, equip the correct tool and use a matching
station: woodworking for arrows/bolts, weaponsmithing for bullets.

EXAMPLE
  craft ammo type arrows
  craft ammo material 1 maple wood
  craft ammo material 2 steel
  craft ammo material 3 wool
  craft ammo enhancement 3
  craft ammo damage fire 6
  craft ammo show
  craft ammo begin

This recipe has material bonus +2 and crafts +3 arrows with +1d6 fire
damage, at DC 14 and item level 17. Show lists the exact material and mote
costs before you attempt it.

The earlier one-line syntax still works and crafts immediately:
  craft ammo arrows 2 fire 6
  craft ammo cost arrows 2 fire 6
  craft ammo arrows material ironwood
These one-line recipes use their own defaults; they do not alter your saved
settings. Use numbered material commands to customize all three slots.

RESTRICTIONS
Finish current crafting projects, supply orders, trades, and combat first.
You need enough skill to possibly succeed and space to carry the result.
Missing resources or equipment prevent an attempt without spending. Every
actual attempt consumes ingredients and motes, including failures, and
imposes a short delay. Success awards crafting experience.

AMMO POUCHES AND USE
Put finished ammunition into your equipped ammo pouch/quiver before firing.
Pouch capacity counts STACK OBJECTS, not weight or remaining shots. A full
50-use bundle occupies one slot; a partially used bundle still occupies one
slot. Removing a bundle frees its slot. Only ammunition can be put inside.
Actual carried weight still counts toward your character''s encumbrance.
Use ammopref to select preferred ammunition from your equipped pouch.

RANDOM AMMO DROPS
Random treasure also awards one finite 50-use stack. Its enhancement follows
the treasure source level, rather than your character''s level:
  Level  1-5:  +1 enhancement; elemental bonus, if rolled, +1d4.
  Level  6-10: +2 enhancement; elemental bonus, if rolled, +1d6.
  Level 11-15: +3 enhancement; elemental bonus, if rolled, +1d8.
  Level 16-20: +4 enhancement; elemental bonus, if rolled, +1d10.
  Level 21-25: +5 enhancement; elemental bonus, if rolled, +1d12.
  Level 26+:   +6 enhancement; elemental bonus, if rolled, +1d12.

Common drops have enhancement only. Rare, Legendary, and Mythical drops
have a 50%, 75%, and 100% chance of bonus damage, respectively. The damage
type is randomly fire, cold, acid, electricity, or sonic. Drop descriptions
show the enhancement and any elemental bonus. When no source level is
provided, the treasure grade determines its level tier instead.

These random-drop rules do not change crafting costs, DCs, or recipes.
Crafted ammo uses your chosen materials, enhancement, damage type and die.

See also: CRAFTING, RANGED, AMMUNITION, AMMOPREF',CHAR(10),CONCAT(CHAR(13),CHAR(10))),0,1000,FALSE)
ON DUPLICATE KEY UPDATE category=VALUES(category),entry=VALUES(entry),min_level=VALUES(min_level),max_level=VALUES(max_level),auto_generated=FALSE;
INSERT IGNORE INTO help_keywords(help_tag,keyword) VALUES
('CRAFTAMMO','craftammo'),
('CRAFTAMMO','craft ammo'),
('CRAFTAMMO','craft-ammo'),
('CRAFTAMMO','ammocrafting'),
('CRAFTAMMO','ammunitioncrafting'),
('CRAFTAMMO','ammunition crafting');
COMMIT;
