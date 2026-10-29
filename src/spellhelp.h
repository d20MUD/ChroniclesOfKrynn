#ifndef SPELLHELP_H
#define SPELLHELP_H

/* Caller owns the returned text. Includes documented legacy IDs; NULL means unknown/hidden ID or OOM. */
char *spellhelp_render(int spellnum);
struct char_data;
/* Show exact or unique-prefix matches; return 0 without output otherwise. */
int spellhelp_try_show(struct char_data *ch, const char *argument);
int spellhelp_public(int spellnum);

#endif
