# Web quest history

Krynn's Quests tab ends with a Quest history text link. It requests a paginated
list of the character's saved completed quests. Quest names and pagination are
text links, not buttons. A quest link loads description, quest information and
completion text into the modal; Back restores the previous list page. Existing
color parsing, paragraph layout, native dialog closing and command focus apply.
The Quest details Windows preference also controls history: when off, the footer
runs the existing terminal `quest history` command instead of opening a modal.

The server exposes the opt-in read-only MSDP variable `QUEST_HISTORY`:

- `quest webhistory [page]`: zero-based pages, 50 saved completions per page.
  JSON includes `id`, `kind: "list"`, `entries` (`questId`, `name`, `available`),
  `total`, `page`, and `pages`.
- `quest webhistorydetail <quest-vnum>`: JSON includes `id`, `kind: "detail"`,
  `questId`, `name`, `description`, `info`, and `completion`.
- Failed requests include `error`; the browser retains a route back to the list.

Requests must originate from a player reporting QUEST_HISTORY. Detail requests
check that the vnum is in that player's completed list before looking up prose.
Uncompleted quests cannot be inspected. Deleted prototypes remain visible as
unavailable history entries, without a detail link. These are current quest
prototype descriptions, as with terminal history; historic prose snapshots and
repeatable completions not recorded by the existing quest system are not added.
Requests do not change progress, award rewards or modify completion history.

Unique response IDs distinguish repeated requests. Numeric arguments, completion
counts, page indices and payload sizes are bounded. The gateway subscribes only
for supported web clients and forwards data through `MudState.questHistory`.
No terminal-output parsing is used.

Validation: native `tests/run_quest_history.py` runs under address/undefined
sanitizers and covers opt-in, empty/full history, ownership, malformed requests,
page boundaries, deleted quests and payload limits. Web quest tests cover
parsing, colored/escaped content, text links, modal request/navigation state,
Windows fallback and gateway mapping. Deploy both the game and web gateway/client
for this feature; the legacy terminal history command remains available.
