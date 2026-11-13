#include "clan_policy.h"
#include <json-c/json.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

static bool reject(char *reason, size_t size, const char *message)
{
  if (reason && size) snprintf(reason, size, "%s", message);
  return false;
}
static struct json_object *field(struct json_object *o, const char *key,
    enum json_type type)
{
  struct json_object *value = NULL;
  return json_object_object_get_ex(o, key, &value) &&
      json_object_is_type(value, type) ? value : NULL;
}
bool clan_policy_parse(const char *text, size_t length, bool sql_ready,
    struct clan_progression_config *out, char *reason, size_t size)
{
  struct json_tokener *parser;
  struct json_object *root = NULL, *mode, *version, *pilots;
  struct clan_progression_config c = clan_progression_defaults;
  const char *validation = NULL;
  size_t end, i;
  bool ok = false;
  if (reason && size) *reason = 0;
  if (!out || !text || !length || length > CLAN_POLICY_MAX_BYTES)
    return reject(reason, size, "Missing or oversized clan startup configuration.");
  parser = json_tokener_new();
  if (!parser) return reject(reason, size, "Cannot allocate clan configuration parser.");
  json_tokener_set_flags(parser, JSON_TOKENER_STRICT | JSON_TOKENER_VALIDATE_UTF8);
  root = json_tokener_parse_ex(parser, text, (int)length);
  end = json_tokener_get_parse_end(parser);
  while (end < length && isspace((unsigned char)text[end])) end++;
  if (json_tokener_get_error(parser) != json_tokener_success || end != length ||
      !root || !json_object_is_type(root, json_type_object) ||
      json_object_object_length(root) != 9) {
    reject(reason, size, "Clan configuration must be one JSON object with exactly nine settings.");
    goto done;
  }
  version = field(root, "version", json_type_int);
  mode = field(root, "mode", json_type_string);
  pilots = field(root, "pilot_clans", json_type_array);
  if (!version || json_object_get_int64(version) != 1 || !mode || !pilots ||
      json_object_get_string_len(mode) != (int)strlen(json_object_get_string(mode))) {
    reject(reason, size, "Invalid clan configuration version, mode or pilot_clans list.");
    goto done;
  }
  c.controls.store_mode = sql_ready ? CLAN_STORE_MYSQL : CLAN_STORE_LEGACY;
  if (!strcmp(json_object_get_string(mode), "off")) c.controls.mode = CLAN_PROGRESSION_OFF;
  else if (!strcmp(json_object_get_string(mode), "shadow")) c.controls.mode = CLAN_PROGRESSION_SHADOW;
  else if (!strcmp(json_object_get_string(mode), "live")) c.controls.mode = CLAN_PROGRESSION_LIVE;
  else { reject(reason, size, "Clan mode must be off, shadow or live."); goto done; }
#define FLAG(key) do { \
  struct json_object *value = field(root, #key, json_type_boolean); \
  if (!value) { reject(reason, size, "Clan setting " #key " must be boolean."); goto done; } \
  c.controls.key = json_object_get_boolean(value); \
} while (0)
  FLAG(buffs); FLAG(contracts); FLAG(expeditions); FLAG(quartermaster); FLAG(web); FLAG(public_release);
#undef FLAG
  c.controls.pilot_count = json_object_array_length(pilots);
  if (c.controls.pilot_count > CLAN_PROGRESSION_MAX_PILOT_CLANS) {
    reject(reason, size, "Too many pilot clan identities."); goto done;
  }
  for (i = 0; i < c.controls.pilot_count; i++) {
    struct json_object *uid = json_object_array_get_idx(pilots, i);
    if (!uid || !json_object_is_type(uid, json_type_string) ||
        json_object_get_string_len(uid) != CLAN_PROGRESSION_UID_SIZE - 1 ||
        !clan_progression_uid_valid(json_object_get_string(uid))) {
      reject(reason, size, "Invalid pilot clan incarnation identity."); goto done;
    }
    memcpy(c.controls.pilot_clans[i], json_object_get_string(uid), CLAN_PROGRESSION_UID_SIZE);
  }
  if (clan_progression_config_validate(&c, &validation) != CLAN_CONFIG_OK) {
    reject(reason, size, validation ? validation : "Invalid clan policy."); goto done;
  }
  if (c.controls.mode != CLAN_PROGRESSION_OFF && !clan_progression_campaign_supported()) {
    reject(reason, size, "Clan progression is unavailable in this campaign."); goto done;
  }
  *out = c;
  ok = true;
done:
  if (root) json_object_put(root);
  json_tokener_free(parser);
  return ok;
}
bool clan_policy_load(const char *path, bool sql_ready, struct clan_progression_config *out,
    bool *present, char *reason, size_t size)
{
  char bytes[CLAN_POLICY_MAX_BYTES + 1];
  size_t count;
  FILE *file;
  bool read_ok;
  if (!path || !out || !present) return reject(reason, size, "Missing clan configuration destination.");
  file = fopen(path, "rb");
  if (!file) {
    if (errno == ENOENT) {
      struct clan_progression_config c = clan_progression_defaults;
      c.controls.store_mode = sql_ready ? CLAN_STORE_MYSQL : CLAN_STORE_LEGACY;
      *out = c; *present = false;
      if (reason && size) *reason = 0;
      return true;
    }
    return reject(reason, size, "Cannot read clan startup configuration.");
  }
  count = fread(bytes, 1, sizeof(bytes), file);
  read_ok = !ferror(file);
  if (fclose(file)) read_ok = false;
  if (!read_ok) return reject(reason, size, "Failed reading clan startup configuration.");
  if (!clan_policy_parse(bytes, count, sql_ready, out, reason, size)) return false;
  *present = true;
  return true;
}
