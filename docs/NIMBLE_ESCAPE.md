# Goblin Nimble Escape

Nimble Escape is passive and uses `flee` or `flee <direction>`. It permits a chosen exit and bypasses the encounter enemy-speed comparison. It retains the normal move-action and movement-point costs, usable-exit requirements, and movement-prevention checks.

Directed fleeing previously called `do_simple_move()` directly and omitted `perform_flee()` combat cleanup. The shared `perform_flee_direction()` routine now handles random and directed retreats, including standing/incapacitation checks, action costs and cleanup of the fleeing character and all combatants targeting it after successful movement. Failed movement leaves combat intact. Spring Attack directed retreats use the same fix. The ordinary `perform_flee()` entry point retains random retreat behavior.

`python3 tests/run_nimble_escape.py` exercises the production command, common flee routine and speed check with ASan/UBSan. It covers an outpaced Goblin, both retreat forms, multiple attackers, failed movement, unavailable actions, closed exits, stun and grounding. No live gameplay commands or server restart are performed by the test.
