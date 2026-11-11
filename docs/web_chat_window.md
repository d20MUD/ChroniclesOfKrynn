# Web communication pane

Krynn now uses the client's existing chat pane with a separate MSDP transport.
Its default dock is left of the main output. Layout → Chat pane lets players
choose the right side, font family, font size and width (pixels or percentage).
Preferences persist with existing client settings. The pane can collapse and
retains the existing bounded chat history and ANSI rendering. Small viewports use
the existing stacked layout. Minimalist mode continues to hide auxiliary panes.

Star Wars keeps its own destination-tag routing and text-format fallback. Krynn
does not use those regexes to recognize speech or channel lines.

## Transport

Krynn sends transient MSDP `CHAT_MESSAGE` pairs with a JSON object:

    {"text":"\u001b[33mYou chat, 'Hello'\u001b[0m\r\n"}

The text is recipient-specific output after ordinary color processing. This is
an event stream, not a cached reported variable: identical successive messages
are sent individually. JSON escapes quotes/control characters inside the MSDP
value. The proxy forwards valid Krynn events as existing `chat` WebSocket messages;
malformed events are discarded. No channel classification uses terminal prose.

A nestable communication scope covers say/rsay/OOC, group speech, tell/reply,
whisper/ask, global channels (chat/gossip, shout, holler, auction, grats, gemote),
quest communication, clan chat, staff chat, emotes/targeted emotes, communication
rolls and channel history. Discord inbound channel sends use the same scope.
Existing channel, room, visibility, hearing, ignore and recipient restrictions
run before output is emitted. Channel history insertion alone never emits a
web event, because some history entries are retained for recipients who cannot
currently hear that channel. Scope wrappers restore routing even on early returns
and nest safely when one communication handler calls another.

For negotiated MSDP connections identifying as LuminariWebClient, communication
output goes to the event stream rather than being duplicated in the main pane.
Ordinary output and prompts stay in the terminal. Output capture for existing
modals keeps its existing priority. Clients without negotiated MSDP retain normal
terminal output, including colors. No Telnet/ANSI output is stripped.

The `chat`/`gossip` OOC channel shows a visible player's character name unless
they are anonymous and the reader lacks their introduction. That case retains the
normal short-description identity. Invisible speakers and NPCs keep ordinary
recipient-aware naming. Other roleplay channels retain their original names.
The same chosen recipient-specific identity is used for the channel's history.

## Validation and rollout

`tests/run_chat_window.py` exercises production MSDP emission and identity helpers
under ASan/UBSan, including repeated events, ANSI text/escaping, Telnet fallback,
anonymous/unintroduced readers, visibility, NPCs, nested scopes and early returns.
Web session tests exercise Krynn MSDP events separately from Star Wars split-tag
routing and ensure ordinary Krynn prose remains terminal output. All 117 web
client tests and the web TypeScript/Vite build pass, as do Make/CMake game builds.

The web proxy and game process must run the updated builds, and existing browser
sessions should reload to pick up the new pane. No game/proxy process was restarted
as part of these source changes.


## Reconnect history

The Krynn proxy subscribes to `CHARACTER_NAME` independently of display settings.
The first nonempty in-play identity on a new TCP session clears the browser chat
pane and sends `history all` once. This covers saved and manual logins, including
reconnecting to an existing character. Repeated state reports do not replay the
history. Selecting another character restores that character's own history.
History output follows the existing MSDP `CHAT_MESSAGE` route and ANSI rendering;
normal terminal prose is unaffected. Star Wars retains its separate destination
protocol and receives no Krynn-specific history command. History availability and
retention remain those of the game's existing combined history command.
