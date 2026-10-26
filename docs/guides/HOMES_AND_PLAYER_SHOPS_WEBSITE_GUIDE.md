# A Place to Call Yours: Homes & Player Shops

Adventure gives you stories. A home gives you somewhere to keep them. A shop gives your next great find a chance to fund the next expedition.

With neighborhood housing and player-run market stalls, you can put down roots, create a space with its own personality, and make your mark on the local marketplace. Build a welcoming retreat for friends, keep your treasured equipment close, or open a shop whose name other adventurers will remember.

## Turn a house into your home

Your home begins with a name and a room waiting for your imagination. From there, expand its layout, name its rooms, and write descriptions that bring each space to life.

Imagine a quiet study lined with accounts of your travels, a grand hall for welcoming companions, or a modest retreat with a view described exactly as you picture it. Add details for visitors to examine: a painting above the hearth, a trophy with a story, or an inscription that hints at your character's past. These are details you create through room text and extra descriptions, giving your home a character all its own.

You decide who receives guest access. Invite friends to explore and enjoy what you have created, while keeping changes to the house and access to its storage in your hands.

### Room to grow, treasures to keep

Add rooms as your ambitions and budget grow. A first purchase can become a larger home over time, with new spaces connected in the directions you choose.

Storage chests give your belongings a lasting place in that home. Their contents persist across restarts, and each chest offers unlimited item storage for eligible belongings. Customize a chest's name and appearance to suit its surroundings, then use familiar `put` and `get` commands to store and retrieve your gear.

**Keep valuables in chests.** Loose items left on the floor do not persist across restarts. Only the owner can use house storage; money and certain restricted items cannot be stored.

### Your first front door

Visit a neighborhood entrance and use `house list` to see its existing homes. To purchase your own, choose a name:

```text
house buy The Lantern's Rest
```

The purchase message supplies your house number. Use `house enter <house-id>` with that number to step inside. Once home, `house info` shows your details, and `house` gives you a reminder of the available commands. Use `house leave` when adventure calls again.

The default house price is **100,000 gold**, with additional rooms costing **50,000 gold** and storage chests **10,000 gold** each. Room and chest customization also carry fees. Prices can vary by neighborhood, so check local costs with staff before planning your purchase. Each character may own one house per neighborhood.

## Put your name above a market stall

That excellent sword you have outgrown could be another adventurer's next prized possession. A player shop lets you offer it at a price you choose, under a shop name of your own.

Rent a stall in a designated market, give it a memorable name, and stock it with gear you want to sell. Your items retain their customizations and enchantments, so buyers can find equipment with a history and personality as well as useful properties.

**Your stall keeps selling while you are offline**, as long as its rent is paid. There is no sales commission, and you can collect your proceeds from anywhere. Spend your time adventuring while your stock waits for the right buyer.

### Open for business

Use `stall directory` from anywhere to find market locations. At a market, `stall list` shows the local stalls, their availability, rent, and item capacity.

Rent an available stall using its displayed number, then make it yours:

```text
stall rent <stall-number>
stall name The Rusty Dragon
stall sell sword 25000
```

That example offers a carried sword for 25,000 gold. Use your item's keyword and your chosen price. Stock must be identified, unbound, and transferable; some items are restricted, and containers and sheaths must be empty.

Check `stall info` to review your lease and stock. You can adjust prices or withdraw unsold items, and `stall collect` brings in your earnings. Each account may rent one stall at a time, managed by the character who rented it.

### Discover your next great find

Shopping starts wherever you are. Try `stall search sword` to search all open stalls and see where matching items are offered. Travel to the market to browse the stall, inspect an item's properties, and make your purchase.

Stall numbers identify shops; listing numbers identify individual items. Use the numbers shown in the results when browsing or buying, and bring carried gold and enough inventory space for your purchase.

### Keep the doors open

The usual rent is **1,000 gold for seven real days**, with space for **20 items**, though individual stalls may differ. Visit your stall and use `stall renew` to extend your lease. Renewing before it expires adds another week to your existing time.

Rent continues while you are offline. Sales stop when the lease expires, but you have **48 hours to renew** before the stall becomes available to another tenant. If you close your shop or let the lease lapse, your unsold items and earnings remain yours: use `stall info`, `stall withdraw <listing-id>`, and `stall collect` from anywhere to reclaim them. Unused rent is not refunded.

## Make your next adventure personal

A home can become the setting for your character's stories. A shop can turn yesterday's discoveries into tomorrow's opportunities. Whether you want a quiet retreat, a gathering place for friends, or a familiar name in the market, start with a neighborhood visit or `stall directory`—and build something of your own.
