# Study exit review

Quitting the main Study menu shows a review before the existing save/discard prompt. The review lists net ability changes (including boosts and Great Ability feats), trained skill ranks, feats and weapon/school/skill choices, spells and powers, discoveries, mercies, cruelties, languages, eidolon/aspect changes, racial and bloodline choices, dragon choices, Loremaster selections and epic feat suboptions. Remaining skill/training/boost and feat-point pools follow the selections. A session with no pending training says so explicitly.

The review is read-only: it does not finalize selections, spend points, or grant effects. Existing validation still runs before exit and before saving. Automatic consequences of saving, such as granted bloodline powers, retain their existing behavior.

Legacy Study options that already edit the live character are compared with a small entry snapshot: preferred classes, domains, school restrictions, familiar/companion, New Arcana slots and favored enemies. Changed settings are labelled as already applied and not undone by discarding pending training. This feature does not alter those legacy save/discard semantics. The entry snapshot is transient Study data and requires no player-file or SQL migration.

Validation: `python3 tests/run_study_summary.py` compiles the production summary functions with ASan/UBSan. It checks ordering before the prompt, no-change output, combined ability/boost/feat changes, skills, spells, languages, eidolon changes, epic suboptions, immediate preference changes, and that review does not mutate pending choices or character training.

The main Study menu shows only currently accessible options, retaining their original number/letter shortcuts as availability changes. Quit is always shown; Reset Character appears only at level one. Unavailable entries and the old asterisk legend are omitted. Existing selection guards still apply.

Eidolon access requires actual Summoner or Necromancer class ranks before checking unchosen base forms, evolutions or aspects. Both the main-menu entry and the F shortcut use this check. `python3 tests/run_eidolon_study_access.py` validates every base-class ID, multiclass access and missing-base-form behavior with ASan/UBSan.

The familiar option and shortcut 5 require Wizard, Sorcerer or Warlock ranks plus the Summon Familiar feature. A familiar feat from gear or stale data alone does not make unrelated classes eligible. `python3 tests/run_familiar_study_access.py` checks every class, missing features, multiclasses and NPCs using the production access macro with ASan/UBSan.
