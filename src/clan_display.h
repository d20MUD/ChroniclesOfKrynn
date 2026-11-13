#ifndef CLAN_DISPLAY_H
#define CLAN_DISPLAY_H
/* Text-command presentation only. Internal durable identities stay unchanged. */
#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
static inline void clan_display_key(const char *s, char *out, size_t size)
{
  size_t n = 0;
  for (; s && *s; s++) {
    if (*s == '\t' && s[1]) { s++; continue; }
    if (isalnum((unsigned char)*s) && n + 1 < size)
      out[n++] = (char)tolower((unsigned char)*s);
  }
  if (size) out[n] = 0;
}
/* Exact names/legacy keys take precedence over unique partial names. */
static inline int clan_display_match(const char *query, const char *name, const char *id)
{
  char q[256], n[256], k[256];
  clan_display_key(query, q, sizeof(q));
  clan_display_key(name, n, sizeof(n));
  clan_display_key(id, k, sizeof(k));
  if (!*q) return 0;
  if (!strcmp(q, n) || (*k && !strcmp(q, k))) return 2;
  return strstr(n, q) ? 1 : 0;
}
/* Catalogs are bounded arrays of structs containing fixed-size name/id fields. */
static inline int clan_display_find(const char *query, const void *entries, size_t count,
                                    size_t stride, size_t name_at, size_t id_at)
{
  size_t i; int best = 0, found = -1;
  for (i = 0; i < count; i++) {
    const char *e = (const char *)entries + i * stride;
    int score = clan_display_match(query, e + name_at, e + id_at);
    if (score > best) { best = score; found = (int)i; }
    else if (score && score == best) found = -2;
  }
  return found;
}
#define CLAN_DISPLAY_FIND(query, catalog, type) \
  clan_display_find(query, (catalog).entries, (catalog).count, sizeof(type), \
                    offsetof(type, name), offsetof(type, id))
static inline const char *clan_display_time(int64_t value, char out[64])
{
  time_t t = (time_t)value; struct tm date;
  if ((int64_t)t == value && gmtime_r(&t, &date) &&
      strftime(out, 64, "%a %d %b, %H:%M UTC", &date)) return out;
  return "time unavailable";
}
#endif
