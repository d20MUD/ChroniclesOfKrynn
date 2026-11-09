Web loot notifications
======================

Krynn web clients can subscribe to LOOT_INFO for kill reward popups. Each JSON
response has a unique id, topic, terminal fallback body, grouped native fields,
and a truncated flag, matching the existing lore/score result schema.

The treasure.c kill reward path collects generated items, event rewards,
crafting materials, motes and bonus gold in one response. Existing treasure
chance, recipient selection and reward quantities are unchanged. Other reward
sources (admin creation, scrounging, etc.) do not open kill reward popups.

After an NPC corpse is built, visible non-money items produce a separate corpse
loot event. This covers ordinary and percentage-based zone reset G/E loads and
I-generated treasure. Its fields explicitly say the items remain on the corpse
and must be collected. It does not move or award items, include hidden objects,
or inspect player corpses. Empty and money-only corpses do not open a popup.

Windows > Loot drops enables/disables popups. Disabled results appear in the
main output, including native fields when an event has no terminal body. Other
MUD presets do not subscribe to this event by default. Closing the final popup
returns focus to the command bar.

Validation: tests/run_loot_notifications.py exercises actual native corpse
notifications and shared delivery. tests/run_crafting_treasure.py checks drop
chance/resource behavior; web output-window and MSDP tests cover opt-in,
unique-event routing, and readable fallback output.
