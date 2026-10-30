# Sorcerer Perk Trees

Sorcerers have an independent copy of the wizard perk trees. This is the initial
baseline for later sorcerer-specific customization; see [Wizard Perk Trees](WIZARD_PERKS.md)
for the copied perk designs.

## Trees and IDs

- General: 800–808.
- Evoker: 809–826.
- Controller: 827–844.
- Versatile Caster: 845–859.

Each sorcerer perk uses the corresponding wizard ID plus 700. The definitions are
separate and keep the same names, categories, costs, maximum ranks, effects, and
tier level requirements. Prerequisites refer exclusively to sorcerer perk IDs.
Use `perk list sorcerer` to view the tree and `perk buy <id>` to purchase a perk.

Arcane Augmentation (#803) reduces sorcerer spell-slot recovery time by 5% per rank,
up to 25%, subject to the existing minimum recovery time. Its wizard counterpart
continues to reduce wizard spell preparation time.

## Points and existing characters

Sorcerer progression awards sorcerer points, independently of wizard points.
Loremaster and Dragon Disciple progression retain their existing choice of base
class, now using the separate sorcerer pool when applicable.

On the first login after this change, characters with sorcerer levels and no wizard
levels move their old shared wizard point pool and wizard perk purchases to the
corresponding sorcerer pool and IDs. Ranks are preserved, and a saved migration
marker prevents repeated transfers.

Wizard/sorcerer multiclasses retain their existing wizard purchases and unspent
wizard points because the old shared pool does not record which class earned them.
Their future sorcerer awards go to the new pool. Copied special-effect handlers
recognize either tree; matching ranked special effects use the higher rank rather
than adding the same bonus twice. Purchases and prerequisites remain independent.
