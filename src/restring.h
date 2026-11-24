#ifndef KRYNN_RESTRING_H
#define KRYNN_RESTRING_H
struct char_data;
void do_restringapply(struct char_data *, const char *, int, int);
void do_restringedit(struct char_data *, const char *, int, int);
bool restring_editor_input(struct char_data *, const char *);
void restring_cleanup(struct char_data *);
#endif
