# Quests and Travel

A request to help an elderly gardener can lead to smugglers, endangered families, and secrets buried beneath Palanthas. An assignment in Sanction can draw you from a training ground into haunted country and dangerous investigations. Beneath the streets, a healer needs help saving a child.

Chronicles of Krynn takes place just a few years after the War of the Lance. The great war has ended, but its consequences still shape the roads you travel and the people you meet. Our quests give you a place in that unsettled world, whether you seek service, discovery, wealth, or something worth fighting for.

## Taking your first quest

Look for people who ask for help, offer assignments, or draw your attention when you enter a room. Read their words: they often explain where to go, what to bring, and whom to find afterward.

At a quest giver, begin with:

```text
quest list
quest list 1
quest join 1
```

`quest list` displays that person's quests, including their level limits, completion status, and whether they can be repeated. `quest list 1` gives more information about the first entry; `quest join 1` accepts it if you qualify. Use the number beside the quest you actually want.

Some assignments require earlier quests, a particular item, or another condition. Seeing a quest in a list does not necessarily mean you can accept it yet.

### Keep track of your adventures

| Command | What it does |
|---|---|
| `quest progress` | Lists your active quests and their progress. |
| `quest progress <index>` | Displays details for one active quest. |
| `quest history` | Reviews completed quests. |
| `questline list` | Lists the story arcs recorded in the questline journal. |
| `questline show <id>` | Displays an arc using its ID from the list. |
| `questline show <id> all` | Displays the full recorded arc. |
| `quest leave <index>` | Abandons an active quest when permitted. |

**Quest menu numbers and active quest indexes are different.** Joining uses the number at the quest giver. Progress, leaving, and sharing use the index shown by `quest progress`, which starts at **0**. Check the current list before entering a number.

Some quests have time limits, and abandoning a quest can cost quest points. Certain story steps must be completed rather than abandoned. Read the assignment before accepting it and check your progress when returning from a long journey.

### What will you be asked to do?

Quests include defeating enemies, collecting supplies, delivering objects, finding people and places, protecting someone in danger, and resolving conversations. Some tasks involve crafting or other practical work rather than combat.

For a delivery, bring the requested object to the **specified person**. Two characters can share a keyword such as `healer`; inspect them carefully and use a numbered target when necessary, such as `look 2.healer` and `give mushroom to 2.healer`. The right item given to the wrong person does not fulfill the assignment.

Follow the quest's instructions after reaching an objective. A completed count may send you back to report, introduce a new contact, or open the next assignment. Rewards vary and can include experience, money, quest points, and equipment.

## Adventures already waiting for you

These introductions describe existing adventures without revealing their solutions. Follow the quest dialogue for the next contact and the requirements of each step.

### Palanthas: small favors, larger secrets

Begin in [Palanthas](/palanthas), where a mysterious old woman near the Central Plaza fountain needs help with the **Gardens of the Blue Phoenix**. **Welcome**, **Thieving Squirrels**, and **Filthy Wabbits** introduce the city's early quest path through seemingly ordinary errands.

The story soon reaches beyond the gardens. **The Cold War**, **Trouble at the Docks**, and **A Smuggler's Dream** draw you toward the waterfront and **Bethel Island**, where smuggling and draconian activity suggest that the war's aftermath is far from peaceful.

Other connected adventures include:

- **Reporting for Training!** and the training assignments: face increasingly dangerous opponents while learning your way around Palanthas's adventuring life.
- **A Family in Peril**, **The Rescue**, and **Home at Last**: follow a more personal thread through the city's troubles.
- **Guild Infiltration**, **Making Friends**, and **No Honour Among Thieves**: venture into less respectable company, where a friendly reception may have a price.
- **Components for Answers**: gather the materials needed for magical investigation as the larger mystery unfolds.
- **A Clue to the Past** and **A Crypt-ic Search**: turn toward old secrets and places where the dead do not necessarily rest quietly.
- **Whispers Beneath the Withering Boughs** and **Echos on the Wind**: carry the developing story beyond Palanthas toward Qualinesti and Que-Shu.

The Palanthas path moves between city streets, palace contacts, island journeys, and more distant destinations. Pay attention when someone names your next contact; the story does not stay with a single quest giver.

### Sanction: orders, investigations, and the Black Moon

[Sanction](/sanction) offers a very different beginning. **Sergeant Zock Deress** introduces a military path through **Welcome to Sanction** and **Roving Ranger**, followed by assignments such as **Beetle Juice**, **Rat Cull**, and **Skelly Bones**.

This is a harsh city shaped by Dragonarmy power. Some orders are brutal, and service here can raise uncomfortable questions about what your character is willing to do.

As the story expands, you can encounter:

- **Paths to Snow Wood Convent** and its connected assignments: leave the city for a troubled religious settlement and the dangers surrounding it.
- **To the Slimy Depths**: descend into another side of Sanction's world.
- **The Myth of the Ripper**: pursue a multipart investigation with an ominous reputation.
- **Going Rogue**, **Investigating Argile**, and **The Truth about Argile**: follow evidence, confront threats, and uncover the story behind a rogue encampment.
- **To Nuitari's Spike** and **Blood Under the Black Moon**: continue into a longer sequence of investigation and danger beneath the influence of the dark moon.

These adventures offer more than a succession of enemies. Reports, discoveries, and conversations carry the story forward, so read what your contacts tell you between assignments.

### The Shadowpeople: Mushy Feelings

Beneath Sanction, a healer of the Shadowpeople struggles to care for a sick infant. The multipart **Mushy Feelings** story asks you to find mushrooms and other needed ingredients to help her treatment.

It is a more intimate adventure amid the city's larger conflicts: a child is ill, supplies are missing, and someone needs your help. Keep track of the requested ingredients and return them to the healer caring for the infant.

### A Tale of Taverns

A frustrated half-elven bard has decided that you would make an excellent protagonist. **A Tale of Taverns** begins with a performer's wounded pride and an invitation to make trouble on his behalf.

Whether his account of events deserves your trust is another matter. This smaller adventure brings a different voice to the world, with boasts, grudges, and the promise of being immortalized in song.

### Battles in the Dreamplane

The **Dreamplane** holds a battle between Whitestone defenders and the Dragonarmies. Its assignments include **Thin the Dragonarmy Ranks**, **Break the Kapak Poison**, **Shatter the Dragonarmy Command**, and **Slay the Green Dragon**.

Here, the struggle takes the form of a dream-battle: hard-pressed knights, venomous kapaks, enemy officers, and a dragon looming over the field. Arrive prepared for combat and seek the commander's instructions.

### Setting out from Solace

[Solace](/solace) makes a welcoming base for exploring Abanasinia. Meet companions at the **Inn of the Last Home**, prepare in town, and follow the roads toward places such as **Darken Wood**, **Qualinost**, **Que-Shu**, and **Xak Tsaroth**.

You can also explore nearer destinations, including **Crystalmir Lake**, **Eld Manor**, the **Goblin Encampment**, and the **Infected Forest**. Exploring an area is an adventure in its own right; it need not begin with a formal entry in your quest journal.

## Share the journey with your party

You can offer an active quest to another player in your party. Both of you must be in the same room, and the recipient must already qualify for the quest.

```text
quest progress
quest share <index> <player>
```

The recipient chooses whether to take it:

```text
quest accept
quest decline
```

An offer expires after two minutes. Accepting adds a fresh copy of the quest to the recipient's list, with its own progress and any time limit. It does not copy the sharer's completed objectives or bypass earlier story requirements.

Some quests cannot be shared. Each player also needs space in their active quest list. Make sure everyone has accepted the assignment before setting out, and check individual progress as you go.

## Finding your way around Krynn

### Walk and explore

Use the exits shown in each room to travel on foot. The usual directions include `north`, `south`, `east`, `west`, `up`, and `down`, along with diagonal directions where available.

Read room descriptions as well as exit lists. Stairways, bridges, gates, and paths help you understand where you are, especially in places such as Solace, where part of the town is overhead.

### Landmarks and guided walking

For help reaching known destinations, use:

```text
landmarks
walkto <landmark name>
walkto cancel
```

`landmarks` shows available destinations. Choose a name from that list to begin walking toward it, or use `walkto cancel` to stop.

Active quests also have navigation shortcuts:

```text
walkto questtarget
walkto questmaster
```

If you have several quests, the command asks you to choose from its numbered list. Use that list's number when prompted; quest navigation numbers begin at **1**, unlike the active indexes used for sharing and leaving.

Follow any confirmation prompt before travel begins. Some objectives have no destination that guided walking can locate.

**Guided walking takes you through the world.** Enemies, terrain, and blocked routes still matter. Prepare for the journey rather than treating it as instant transport.

### Carriages: travel within a region

Find a carriage stand and type:

```text
carriage
carriage <destination>
```

The list shows the destinations available from that stand, their fares, and estimated journey times. Regular fares are **25 coins**.

| Region | Destinations on the carriage network |
|---|---|
| Solamnia | Palanthas Gate, Vingaard Keep, Thelgaard Keep, Caergoth Gate |
| Abanasinia | Solace, Que-Shu Village, Xak Tsaroth, Fireside Tavern, New Sea Docks, Qualinost, Pax Tharkas, Tarsis, Darken Wood |
| Taman Busuk | Sanction Gate, Neraka, City of Morning Dew, Plains of Dust |

Carriages connect stops within their region. To cross between regions, look for a sea connection and continue by carriage after landing.

### Ships: cross the waters

At a supported port, type:

```text
sail
sail <destination>
```

Use the destination name shown in the port's list. Sailing displays fares and estimated travel times; regular passage costs **50 coins**, while characters with the **Sailor background** receive free passage.

Ports include **Palanthas**, **Sanction**, **Bethel Island**, **Caergoth on the northern New Sea**, **Abanasinia on the southern New Sea**, **Eastern Abanasinia**, **Undomesticated Island**, and **Northern Ergoth**. The `sail` list at your current port shows which connections are available.

For travel between Solamnia and Abanasinia, the northern and southern **New Sea** ports provide a useful crossing. Combine a sea passage with carriage journeys to reach places farther inland.

### Magical travel

Magic provides other ways to cover distance. Recall spells include **Recall to Palanthas**, **Recall to Sanction**, and **Recall to Solace**. Other travel magic includes **Word of Recall**, **Teleport**, and **Portal**, subject to the spell's requirements and your character's abilities or available magical items.

Some places restrict magical movement. Before heading somewhere dangerous, consider how you will get back as well as how you will arrive. Use the in-game help for the particular spell or item you plan to rely on.

## Before you leave town

Read your current quest details, check any time limit, and confirm that everyone in your party has the assignments they need. Carry provisions, prepare your spells and equipment, and leave enough money for the return fare.

Then choose a road. A garden errand, a military report, or a healer's request may be the beginning of a much larger story.
