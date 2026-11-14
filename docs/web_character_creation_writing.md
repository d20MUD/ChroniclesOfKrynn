# Web character creation: writing and age

`CHARACTER_CREATION` reports editorText, editorMaxBytes, editorBusy and editorError
for the existing description, background, goals, personality, ideals, bonds and
flaws editors. The web form loads the current value, preserves paragraphs, counts
UTF-8 bytes including CRLF conversion, and offers Save and Cancel.

The gateway accepts a creation-editor save/cancel message only for an active
Krynn creation editor. It checks the server-provided byte limit and transfers
ASCII JSON-quoted chunks on a paced connection-specific timer. Quoting preserves
Unicode, dollar signs, newlines and literal editor commands through the MUD's
terminal input filter. Disconnect cancels pending sends. Normal input is held
while a transfer is being sent.

The MUD accepts these frames only during the seven supported creation editor
states. Its existing backup string and size limit remain authoritative. The
transfer replaces the edit buffer; final Save and Cancel call the native string
editor cleanup with /s or /a. Malformed/oversized transfers cannot save, and Cancel
restores the original value. Ordinary terminal editors keep their existing flow.

Web choices for Goals, Personality, Ideals, Bonds and Flaws open the corresponding
editor directly. The terminal's background-based inspiration menus remain
available in terminal creation. An already-open inspiration screen in the web
client offers Open editor. Bonds now opens its own editor rather than Ideals.
Character-details and background-archetype choices are explicitly reported.

Age is reported as step age with exact server choice values and an adjustments
array per option, drawn from character_age_attributes. The browser uses separate
cards with ability rows and a button containing only the age choice. Terminal
age prose is hidden in this view.

Validation: native web creation/editor transfer tests, native editor save-command
checks, creation help checks, 142 browser/gateway tests, CMake server build,
TypeScript/Vite build, and lint (zero errors, one existing hook warning).

Deployment requires the matching MUD binary, gateway and browser. The initial
clan authority migration requirements are unchanged; this update adds no SQL.
