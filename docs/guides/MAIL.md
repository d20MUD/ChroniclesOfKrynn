# In-game mail

Use `mail` to see the command list, or `mail list inbox` to find a message's ID.

## Reply to a message

```text
mail reply <mail-id>
```

For example, `mail reply 42` opens the mail editor addressed to the sender of
message 42. The subject is copied with `Re:` added if needed. Write your reply,
then enter `/s` to send it or `/a` to cancel. The original message is unchanged.

You can reply to received mail and announcements sent to All. Replies to an
announcement go privately to its original sender. Messages belonging to other
players, messages you deleted, and messages from unavailable senders cannot be
used to start a reply.

Other commands: `mail list sent`, `mail read <id>`, `mail delete <id>`,
`mail days <days>`, and `mail send <name> <subject>`.
