Dialogue quest check details
============================

Quest acceptance and completion events can include a dialogueCheck record:

    {"skill":"diplomacy","roll":6,"modifier":-1,"total":5,"dc":15,"success":false}

Values come directly from the dialogue skill check. The web client shows the
skill, d20 roll plus signed modifier and total, DC, and Success/Failed above
the quest text. The normal completion rewards and fallback quest instructions
remain in their existing modals. A failed attempt with no fallback quest also
opens a result popup and retains the active quest for retry.

Check details are transient and keyed to the character's quest slot and quest
vnum. They survive the delayed completion event and are consumed once by the
matching acceptance or completion payload. Assigning/clearing a slot resets
old check data. The normal Windows acceptance/completion controls apply;
terminal clients continue to receive the existing skill-check output.

Tests: run_dialogue_quest_modal.py exercises all three dialogue skills and both
failure branches; run_quest_events.py verifies native payload values, equality
at DC, and one-time consumption. Web quest-details tests cover success/failure
rendering and malformed data.
