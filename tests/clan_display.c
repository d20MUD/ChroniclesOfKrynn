#define _POSIX_C_SOURCE 200809L
#include "clan_display.h"
#include <assert.h>
struct choice { char id[40], name[80]; };
int main(void)
{
  struct { size_t count; struct choice entries[3]; } catalog = {3, {
    {"ash-v1", "Fresh ash harvest"}, {"maple-v1", "Fresh maple harvest"},
    {"map-v1", "Abanasinia field map"}}};
  char date[64];
  assert(CLAN_DISPLAY_FIND("Fresh ash harvest", catalog, struct choice)==0);
  assert(CLAN_DISPLAY_FIND("ASH", catalog, struct choice)==0);
  assert(CLAN_DISPLAY_FIND("\"Fresh ash harvest\"", catalog, struct choice)==0);
  assert(CLAN_DISPLAY_FIND("fresh", catalog, struct choice)==-2);
  assert(CLAN_DISPLAY_FIND("map", catalog, struct choice)==-2);
  assert(CLAN_DISPLAY_FIND("map-v1", catalog, struct choice)==2);
  assert(CLAN_DISPLAY_FIND("", catalog, struct choice)==-1);
  assert(CLAN_DISPLAY_FIND("missing", catalog, struct choice)==-1);
  assert(CLAN_DISPLAY_FIND("  Abanasinia FIELD map  ", catalog, struct choice)==2);
  assert(!strcmp(clan_display_time(0,date), "Thu 01 Jan, 00:00 UTC"));
  puts("Clan names: exact, partial, quoted, case-insensitive, legacy keys, ambiguity and readable UTC dates passed");
  return 0;
}
