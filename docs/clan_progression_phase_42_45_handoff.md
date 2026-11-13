# Clan progression: Phases 42–45 handoff

Implemented across the MUD repository and sibling `LuminariWebClient` repository.
Defaults remain disabled. No live migration, feature enablement, deployment,
publication, server restart or gateway restart occurred. Phase 46 contract,
expedition and quartermaster panels remain separate work.

## Phase 42: private server read model

`CLAN_PROGRESS` is a read-only, version-1 JSON MSDP variable. `clan_web_read`
uses a consistent SQL snapshot with indexed lookups for the authenticated
character's current membership, clan, Favor and account week, and at most five
buff-family rows. It does not read the whole clan store, accept another subject
from the client, award anything, mint operation identities, expire records or
change balances. The output is capped at 49,152 bytes; currencies, revisions and
the requesting character ID are decimal strings to preserve signed-64-bit values.

Reports include clan level/XP/Influence, personal Favor standing and next standing,
next buff/slot unlocks, account contribution usage/reset, current slots, saved
activation/expiry/cooldown identities, management permission, and per-tier/slot
availability with explanations. The existing pure activation validator supplies
permission, level, cost, cooldown, slot, combat-category and stale-target reasons.
Missing lifecycle storage disables all actions and explicitly marks buff data
unavailable. Basic clan/account fields remain independently readable when safe.

Only current accepted members receive private fields. Applicants, former members,
disbanded clans, disabled web/pilot gates and unavailable storage receive minimal
status reports. No treasury, leader/account identifiers, other members' Favor,
rosters or receipt histories are exported. Account usage is the requesting
account's legitimate aggregate across its own characters/clans.

Initial `REPORT` and `SEND` force a current report. Both initial character MSDP
refresh and periodic refresh call the adapter. Subscribed playing descriptors
refresh at most once per two seconds; shooting/kill, crafting/supply, completion,
reset, permission and membership updates therefore use current SQL state without
new per-source caches. Absolute saved timestamps handle expiry even before SQL
maintenance closes a row. Reads do not restart timers or apply effects.

Invalid identity/account/storage states replace the report rather than retaining
private fields. SEND/REPORT also clear it when no character is selected or the
player is switched. Periodic account-menu/switch clearing flushes only this
variable, avoiding unrelated stale reports. Pool connections are released before
protocol output. No SQL schema extension is added; authoritative core v5 and the
existing activation/lifecycle extensions supply the data.

## Phase 43: gateway and reconnect

The shared MSDP map includes `clanProgress`, with existing custom-name/trim and
case-insensitive resolution. Subscriptions and refresh SENDs require the Krynn
connection independently of modal preferences; unrelated and Star Wars presets
do not subscribe. The shared bounded parser handles JSON strings and objects,
checks versions/identities/types/array bounds and rejects malformed action data.
Missing/unsupported reports become null, clearing prior state.

Existing initial refresh, configuration refresh, login and copyover renegotiation
restore REPORT/SEND. The gateway clears cached clan data on renegotiation and
character changes. The browser records report arrival time and clears state on
disconnection. The checked-in wire fixture comes from the production C MSDPSend
function, then passes through fragmented telnet parsing, gateway state mapping
and browser normalization in tests.

## Phases 44–45: panel and buff controls

A configurable Krynn-only Clan sidebar tab displays exact currencies, level,
Favor standing, next unlocks and the requesting account's weekly usage. Existing
Player info stays the default tab. Responsive wrapping supports narrow columns
and screens; normal React text escaping and color/control stripping handle names
and labels without inserting server HTML. Active buffs and original cooldown
endpoints appear alongside tier, slot and fixed Influence costs.

Activate/replace/cancel require a review step. New actions are disabled for stale
reports (ten seconds), unavailable buff state, insufficient permission and every
server-provided lock reason. The review rechecks the newest report before send.
Replacement includes the exact expected activation ID; cancellation explains that
it gives no refund or timer reset. Buttons use the existing command method and
receipt-backed `clan buffs` command, never a new client-controlled economic API.

Before submission, a random operation ID and bounded intent are saved in browser
session storage, scoped to the requesting character/clan. Double clicks and the
retry button reuse that ID, including after remount/reconnect. No request is sent
if retry storage fails. Only fixed numeric/hexadecimal command fields are built;
no server label or arbitrary stored command becomes executable input. A recorded
activation can be recognized from its operation ID; cancellation outcome remains
in the normal command response. Clearing pending retry state is explicit.

Every command still rechecks SQL authority, current membership, management rights,
release gates, cost, cooldown, slot/category rules and expected activation under
the existing atomic commit. Identical receipt replay spends nothing twice, even
after departure; stale expectations fail without payment. Showing a button does
not confer authorization. Normal text commands continue to work independently.

## Verification

- `python3 tests/run_clan_web.py`: ASan/UBSan private MariaDB target reads, no-write
  proof, exact eighth points, disabled/applicant/former/delegated boundaries,
  contribution/reset refresh, active/cooldown endpoints, receipt replay and stale
  replacement/cancellation; native protocol adapter initial/periodic refresh,
  identity/account loss, absent-character/switched clearing and failed reads;
  production C MSDPSend frame export.
- Web `node --test tests/*.test.mjs`: 127 tests including actual fragmented wire
  parsing, Krynn subscription scope, copyover restore/identity clearing, malformed
  data, React rendering/escaping, disabled/stale controls, review double clicks,
  session retry persistence, layout normalization and existing client regressions.
- Web TypeScript/Vite build and ESLint pass (one pre-existing App.tsx hook
  dependency warning; no errors). Both Make and CMake server builds pass.

Browser verification uses actual React rendering and event handlers plus CSS
layout assertions; no interactive browser or live-server pilot was run. Database
tests use disposable socket-only MariaDB. Production rollout and broader help
publication remain later phases. Protocol details are also documented in
`LuminariWebClient/docs/CLAN_PROGRESS_MSDP.md`.
