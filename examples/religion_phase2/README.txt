RELIGION PHASE 2: OPTIONAL KRYNN PILOT CHURCHES

This package authors zone 699 (vnums 69900-69999), the Courtyard of Mercy
(Mishakal, deity 2) and the Courtyard of the Open Book (Gilean, deity 8).
It is staged separately and is not installed into the live world indexes.
These deity IDs and the extra quest kill-list field require CAMPAIGN_DL.

Each church has a public priest and four pilgrimage objectives, all reachable
at rank 1. Its eastern sanctuary requires the same faith and at least rank 2.
A separate sentinel guards only the eastern direction; the western exit from
each sanctuary is always open. Inner rooms have NoTeleport, NoSummon and
NoRecall. No portals, scripts or alternate entry routes are authored here.

INSTALL

1. Verify zone/vnums 699/69900-69999 and temple IDs 69901/69902 are unused in
   the deployment. If they conflict, renumber all files, references and commands
   consistently before installing.
2. Install sql/20261123_religion_phase2.sql and the earlier religion/estate
   schemas if the game account cannot create missing tables itself.
3. Copy 699.wld, 699.mob, 699.qst and 699.zon into the corresponding
   lib/world/wld, mob, qst and zon directories. Add each filename to its index
   in numeric order before the terminal dollar sign. Save and restart the test
   server so native world loading, questmaster assignment and resets run.
   Do not put these files into a server using a different campaign's format.
4. Run the commands in setup.txt as a builder with zone editing permission.
   Configure all task slots before opening any service boards. Configuration,
   rank names and chests save immediately; temple records save to religion.json.
5. Add a public walking connection from your chosen world location to room
   69900. The two temple foyers already connect to each other. Save that zone
   edit. No outside-world room or vnum is assumed by this package.

PLAYTEST BEFORE OPENING TO PLAYERS

- Check both priests are awake, visible, affiliated correctly and present.
- Devote a normal eligible test player. Use religion task accept 1, read the
  preview, confirm, visit the indicated shrine and return for task turnin.
- Check 2 Standing is earned once, a repeated template is story-only, and
  three distinct completions use the weekly allowance across both churches.
- Leave an active assignment across Monday 00:00 UTC and verify the completion
  week's allowance is used. Story-only acceptance must never become rewarded.
- Verify each sanctuary denies unaffiliated, wrong-faith and rank-1 players;
  admits matching rank-2 and higher players; and always permits walking west
  out, including after leaving the faith.
- Check teleport, portal, dimension-door, summon and recall routes respect
  these authored room flags. Staff movement bypasses are intentional. Reaudit
  any additional scripted travel or connections you add; directional guards
  do not themselves enforce universal room admission.
- Test the chests with normal look in, examine, get and put, then reboot and
  confirm contents persist. Test hospitality yields both buffed provisions.

Offline checks:
  python3 tests/run_religion_pilot.py
  python3 tests/run_religion.py
  python3 tests/run_estate_integration.py
