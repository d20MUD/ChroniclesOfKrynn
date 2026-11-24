/* Church membership, temple services and builder-authored faith requirements.
 * GET_DEITY remains authoritative; NPC affiliation never uses player_specials. */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "handler.h"
#include "spells.h"
#include "act.h"
#include "deities.h"
#include "religion.h"
#include "clan_access.h"
#include "housing.h"
#include "constants.h"
#include "estate.h"
#include "durable_file.h"
#include <json-c/json.h>
#include <errno.h>
#include <limits.h>

#define RELIGION_MAX_TEMPLES 128
#define RELIGION_NAME_LENGTH 100
#define RELIGION_CONFIRM_SECONDS 120
#define RELIGION_JOIN 1
#define RELIGION_LEAVE 2

struct religion_temple
{
  int id, deity, entrance, priest;
  bool enabled, hospitality;
  char name[RELIGION_NAME_LENGTH];
};

static struct religion_temple temples[RELIGION_MAX_TEMPLES];
static int temple_count;
static unsigned long temple_revision;
static bool temples_ready;
static bool service_position(struct char_data *ch);
static char temple_path[512] = LIB_ETC "religion.json";

bool religion_deity_valid(int deity)
{
  return deity > DEITY_NONE && deity < NUM_DEITIES && deity < RELIGION_DEITY_CAPACITY &&
         deity_list[deity].pantheon != DEITY_PANTHEON_NONE;
}

const char *religion_rank_name(int rank)
{
  static const char *const names[] = {"None", "Devotee", "Faithful", "Acolyte",
                                     "Votary", "Steward", "Preceptor", "Elder", "Luminary"};
  return rank >= 0 && rank <= 8 ? names[rank] : "Invalid";
}

int religion_rank(struct char_data *ch)
{
  static const int thresholds[] = {0, 12, 36, 72, 120, 180, 252, 336};
  int rank, standing;
  if (IS_NPC(ch))
    return religion_deity_valid(ch->mob_deity) && ch->mob_temple_rank >= 0 &&
           ch->mob_temple_rank <= 8 ? ch->mob_temple_rank : 0;
  if (!religion_deity_valid(GET_DEITY(ch)))
    return 0;
  standing = ch->player_specials->saved.religion_standing[GET_DEITY(ch)];
  if (standing < 0 || standing > 336) return 0;
  for (rank = 7; rank > 0 && standing < thresholds[rank]; --rank)
    ;
  return rank + 1;
}

bool religion_mob_valid(const struct char_data *mob)
{
  if (mob->mob_temple_rank < 0 || mob->mob_temple_rank > 8 ||
      (mob->mob_deity != DEITY_NONE && !religion_deity_valid(mob->mob_deity)))
    return false;
  if ((MOB_FLAGGED(mob, MOB_BLOCK_RELIGION) || MOB_FLAGGED(mob, MOB_BLOCK_TEMPLE_RANK)) &&
      !religion_deity_valid(mob->mob_deity))
    return false;
  return !MOB_FLAGGED(mob, MOB_BLOCK_TEMPLE_RANK) || mob->mob_temple_rank >= 1;
}

bool religion_guard_allows(struct char_data *ch, struct char_data *guard)
{
  if (!MOB_FLAGGED(guard, MOB_BLOCK_RELIGION) && !MOB_FLAGGED(guard, MOB_BLOCK_TEMPLE_RANK))
    return true;
  /* Preserve the movement system's existing NPC and staff bypass policy. */
  if (IS_NPC(ch) || PRF_FLAGGED(ch, PRF_NOHASSLE))
    return true;
  if (!religion_mob_valid(guard) || GET_DEITY(ch) != guard->mob_deity)
    return false;
  return !MOB_FLAGGED(guard, MOB_BLOCK_TEMPLE_RANK) || religion_rank(ch) >= guard->mob_temple_rank;
}

/* Item requirements do not depend on directional guard flags, and do not
 * grant staff/NPC exemptions. A zero deity or zero rank leaves ordinary stock
 * unrestricted; malformed nonzero metadata fails closed. */
static bool religion_item_allows(struct char_data *buyer, int deity, int rank, bool explain,
                                 const char *action)
{
  int buyer_deity;
  if (deity < 0 || deity >= RELIGION_DEITY_CAPACITY || rank < 0 || rank > 8)
    goto invalid;
  if (!deity || !rank)
    return true;
  if (!religion_deity_valid(deity))
    goto invalid;
  buyer_deity = IS_NPC(buyer) ? buyer->mob_deity : GET_DEITY(buyer);
  if (buyer_deity == deity && religion_rank(buyer) >= rank)
    return true;
  if (explain)
    send_to_char(buyer, "%s requires temple rank %s or higher in the faith of %s.\r\n",
                 action, religion_rank_title(deity, rank), deity_list[deity].name);
  return false;
invalid:
  if (explain)
    send_to_char(buyer, "%s has unavailable religion requirements. Please notify staff.\r\n", action);
  return false;
}

bool religion_purchase_allows(struct char_data *buyer, int deity, int rank, bool explain)
{
  return religion_item_allows(buyer, deity, rank, explain, "This purchase");
}

bool religion_object_wear_allows(struct char_data *ch, struct obj_data *obj, bool explain)
{
  return obj && religion_item_allows(ch, obj->religion_requirement.deity,
                                    obj->religion_requirement.rank, explain, "Wearing this item");
}

bool religion_pet_purchase_allows(struct char_data *buyer, struct char_data *pet, bool explain)
{
  mob_rnum rnum;
  if (!pet || !IS_NPC(pet))
    return false;
  if (!religion_purchase_allows(buyer, pet->mob_deity, pet->mob_temple_rank, explain))
    return false;
  rnum = GET_MOB_RNUM(pet);
  if (rnum == NOBODY || top_of_mobt == NOBODY || rnum > top_of_mobt)
  {
    if (explain) send_to_char(buyer, "That companion is not available for purchase.\r\n");
    return false;
  }
  /* The delivered pet is created from the prototype. An old display mob must
   * not bypass requirements edited on that prototype since the last reset. */
  return religion_purchase_allows(buyer, mob_proto[rnum].mob_deity,
                                  mob_proto[rnum].mob_temple_rank, explain);
}

static bool object_purchase_tree_allows(struct char_data *buyer, struct obj_data *obj, bool explain,
                                        int depth, int *visited)
{
  struct obj_data *child;
  mob_rnum rnum;
  if (!obj || depth > 64 || ++*visited > 10000)
  {
    if (explain) send_to_char(buyer, "That purchase contains invalid item data. Please notify staff.\r\n");
    return false;
  }
  if (!clan_access_check(buyer, &obj->clan_requirement, explain)) return false;
  if (!religion_purchase_allows(buyer, obj->religion_requirement.deity,
                                obj->religion_requirement.rank, explain))
    return false;
  if (GET_OBJ_TYPE(obj) == ITEM_PET)
  {
    /* Pet tokens have their own requirements AND those of the resulting mob. */
    rnum = real_mobile(GET_OBJ_VNUM(obj));
    if (rnum == NOBODY)
    {
      if (explain) send_to_char(buyer, "That companion is not available for purchase.\r\n");
      return false;
    }
    if (!religion_purchase_allows(buyer, mob_proto[rnum].mob_deity,
                                  mob_proto[rnum].mob_temple_rank, explain))
      return false;
  }
  /* A restricted item cannot be sold around its gate by packaging it in a bag
   * or sheath. Do not follow this root's next_content: that is other stock. */
  for (child = obj->contains; child; child = child->next_content)
    if (!object_purchase_tree_allows(buyer, child, explain, depth + 1, visited))
      return false;
  if (obj->sheath_primary &&
      !object_purchase_tree_allows(buyer, obj->sheath_primary, explain, depth + 1, visited))
    return false;
  if (obj->sheath_secondary &&
      !object_purchase_tree_allows(buyer, obj->sheath_secondary, explain, depth + 1, visited))
    return false;
  return true;
}

bool religion_object_purchase_allows(struct char_data *buyer, struct obj_data *obj, bool explain)
{
  int visited = 0;
  return object_purchase_tree_allows(buyer, obj, explain, 0, &visited);
}

/* Shared by world files and every object-record restore path. Preserve an
 * invalid sentinel on malformed data instead of accidentally clearing a gate. */
bool religion_parse_requirement(const char *text, struct religion_requirement *out)
{
  long values[2];
  int i;
  char *end;
  out->deity = out->rank = -1;
  for (i = 0; i < 2; ++i)
  {
    while (isspace((unsigned char)*text)) ++text;
    errno = 0;
    values[i] = strtol(text, &end, 10);
    if (errno || end == text || values[i] < 0 || (*end && !isspace((unsigned char)*end)))
      return false;
    text = end;
  }
  while (isspace((unsigned char)*text)) ++text;
  if (*text || values[0] >= RELIGION_DEITY_CAPACITY || values[1] > 8)
    return false;
  out->deity = (int)values[0];
  out->rank = (int)values[1];
  return true;
}

void religion_clear_confirmation(struct char_data *ch)
{
  if (!ch || IS_NPC(ch) || !ch->player_specials)
    return;
  if (ch->player_specials->restring_quote)
    json_object_put(ch->player_specials->restring_quote);
  ch->player_specials->restring_quote = NULL;
  ch->player_specials->church_preview_until = 0;
  ch->player_specials->religion_task_preview_until = 0;
  ch->player_specials->religion_pending_action = 0;
  ch->player_specials->religion_pending_deity = 0;
  ch->player_specials->religion_pending_source = 0;
  ch->player_specials->religion_pending_temple = 0;
  ch->player_specials->religion_pending_revision = 0;
  ch->player_specials->religion_pending_until = 0;
}

bool religion_load_player_cooldowns(struct char_data *ch, const char *line)
{
  long long values[3];
  int i;
  char *end;
  for (i = 0; i < 3; ++i)
  {
    while (isspace((unsigned char)*line)) ++line;
    errno = 0;
    values[i] = strtoll(line, &end, 10);
    if (errno || end == line || values[i] < 0 || (*end && !isspace((unsigned char)*end)))
      return false;
    line = end;
  }
  while (isspace((unsigned char)*line)) ++line;
  if (*line || values[2] > 1 || (long long)(time_t)values[0] != values[0] ||
      (long long)(time_t)values[1] != values[1])
    return false;
  ch->player_specials->saved.religion_devote_after = (time_t)values[0];
  ch->player_specials->saved.religion_hospitality_after = (time_t)values[1];
  ch->player_specials->saved.religion_channel_off = values[2];
  return true;
}

static bool number(const char *s, int minimum, int maximum, int *out)
{
  char *end;
  long value;
  if (!s || !*s)
    return false;
  errno = 0;
  value = strtol(s, &end, 10);
  if (errno || *end || value < minimum || value > maximum)
    return false;
  *out = (int)value;
  return true;
}

bool religion_load_player_standing(struct char_data *ch, const char *line)
{
  char id_text[32], standing_text[32], extra;
  int id, standing;
  if (sscanf(line, "%31s %31s %c", id_text, standing_text, &extra) != 2 ||
      !number(id_text, 1, RELIGION_DEITY_CAPACITY - 1, &id) ||
      !number(standing_text, 0, 336, &standing))
    return false;
  ch->player_specials->saved.religion_standing[id] = standing;
  return true;
}

static bool plain_name(const char *s)
{
  const unsigned char *p = (const unsigned char *)s;
  if (!s || !*s || strlen(s) >= RELIGION_NAME_LENGTH)
    return false;
  for (; *p; ++p)
    if (*p < 32 || *p > 126)
      return false;
  return true;
}

static bool json_int(struct json_object *obj, const char *key, int low, int high, int *out)
{
  struct json_object *value;
  int64_t n;
  if (!json_object_is_type(obj, json_type_object) ||
      !json_object_object_get_ex(obj, key, &value) ||
      !json_object_is_type(value, json_type_int))
    return false;
  n = json_object_get_int64(value);
  if (n < low || n > high)
    return false;
  *out = (int)n;
  return true;
}

static bool json_boolean_field(struct json_object *obj, const char *key, bool *out)
{
  struct json_object *value;
  if (!json_object_is_type(obj, json_type_object) ||
      !json_object_object_get_ex(obj, key, &value) ||
      !json_object_is_type(value, json_type_boolean))
    return false;
  *out = json_object_get_boolean(value);
  return true;
}

/* Drafts may refer to rooms/mobs not yet built. Enabled records are checked
 * against the live world every time a service is requested. */
static bool temple_live(const struct religion_temple *t)
{
  mob_rnum rnum;
  if (!t->enabled || t->priest <= 0 || !religion_deity_valid(t->deity) || real_room(t->entrance) == NOWHERE ||
      (rnum = real_mobile(t->priest)) == NOBODY)
    return false;
  return religion_mob_valid(&mob_proto[rnum]) && mob_proto[rnum].mob_deity == t->deity &&
         mob_proto[rnum].mob_temple_rank >= 1;
}

bool religion_load_temples(const char *path)
{
  struct json_object *root, *array, *entry, *name;
  struct religion_temple loaded[RELIGION_MAX_TEMPLES];
  FILE *probe;
  int count, i, j, version;
  if (!path || strlen(path) >= sizeof(temple_path))
    return false;
  /* Revisions invalidate outstanding previews even after a failed reload. */
  ++temple_revision;
  temples_ready = false;
  probe = fopen(path, "r");
  if (!probe)
  {
    if (errno != ENOENT)
      return false;
    temple_count = 0;
    strlcpy(temple_path, path, sizeof(temple_path));
    temples_ready = true;
    return true;
  }
  fclose(probe);
  root = json_object_from_file(path);
  if (!root)
    return false;
  if (!json_int(root, "version", 1, 1, &version) ||
      !json_object_object_get_ex(root, "temples", &array) ||
      !json_object_is_type(array, json_type_array) ||
      json_object_array_length(array) > RELIGION_MAX_TEMPLES)
    goto invalid;
  memset(loaded, 0, sizeof(loaded));
  count = (int)json_object_array_length(array);
  for (i = 0; i < count; ++i)
  {
    entry = json_object_array_get_idx(array, i);
    if (!json_int(entry, "id", 1, INT_MAX, &loaded[i].id) ||
        !json_int(entry, "deity", 1, RELIGION_DEITY_CAPACITY - 1, &loaded[i].deity) ||
        !json_int(entry, "entrance", 0, INT_MAX, &loaded[i].entrance) ||
        !json_int(entry, "priest", 0, INT_MAX, &loaded[i].priest) ||
        !json_boolean_field(entry, "enabled", &loaded[i].enabled) ||
        !json_boolean_field(entry, "hospitality", &loaded[i].hospitality) ||
        !json_object_object_get_ex(entry, "name", &name) ||
        !json_object_is_type(name, json_type_string) ||
        !plain_name(json_object_get_string(name)) ||
        strlen(json_object_get_string(name)) != (size_t)json_object_get_string_len(name))
      goto invalid;
    strlcpy(loaded[i].name, json_object_get_string(name), sizeof(loaded[i].name));
    for (j = 0; j < i; ++j)
      if (loaded[j].id == loaded[i].id ||
          (loaded[i].priest && loaded[j].priest == loaded[i].priest))
        goto invalid;
  }
  memcpy(temples, loaded, sizeof(temples));
  temple_count = count;
  strlcpy(temple_path, path, sizeof(temple_path));
  temples_ready = true;
  json_object_put(root);
  return true;
invalid:
  json_object_put(root);
  return false;
}

void religion_boot(void)
{
  int i;
  mob_rnum rnum;
  if (!religion_load_temples(temple_path))
    log("SYSERR: Religion temple registry unavailable; services disabled. Check %s.", temple_path);
  if (top_of_mobt != NOBODY)
    for (rnum = 0; rnum <= top_of_mobt; ++rnum)
      if (!religion_mob_valid(&mob_proto[rnum]))
        log("SYSERR: Mob #%d has invalid religion settings; its faith gates fail closed.",
            mob_index[rnum].vnum);
  for (i = 0; i < temple_count; ++i)
    if (temples[i].enabled && !temple_live(&temples[i]))
      log("SYSERR: Temple %d (%s) has unavailable deity/room/priest; services disabled.",
          temples[i].id, temples[i].name);
}

static bool save_temples(void)
{
  struct json_object *root = json_object_new_object(), *array = json_object_new_array(), *obj;
  const char *text;
  bool saved;
  int i;
  json_object_object_add(root, "version", json_object_new_int(1));
  json_object_object_add(root, "temples", array);
  for (i = 0; i < temple_count; ++i)
  {
    obj = json_object_new_object();
    json_object_object_add(obj, "id", json_object_new_int(temples[i].id));
    json_object_object_add(obj, "deity", json_object_new_int(temples[i].deity));
    json_object_object_add(obj, "name", json_object_new_string(temples[i].name));
    json_object_object_add(obj, "entrance", json_object_new_int(temples[i].entrance));
    json_object_object_add(obj, "priest", json_object_new_int(temples[i].priest));
    json_object_object_add(obj, "enabled", json_object_new_boolean(temples[i].enabled));
    json_object_object_add(obj, "hospitality", json_object_new_boolean(temples[i].hospitality));
    json_object_array_add(array, obj);
  }
  text = json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY);
  saved = durable_file_replace(temple_path, text, strlen(text));
  json_object_put(root);
  return saved;
}

static int temple_index(int id)
{
  int i;
  for (i = 0; i < temple_count; ++i)
    if (temples[i].id == id)
      return i;
  return -1;
}

static struct religion_temple *present_priest(struct char_data *ch, int deity, bool hospitality)
{
  struct char_data *mob;
  int i;
  if (!temples_ready || IN_ROOM(ch) == NOWHERE)
    return NULL;
  for (i = 0; i < temple_count; ++i)
  {
    if (temples[i].deity != deity || !temple_live(&temples[i]) ||
        (hospitality && !temples[i].hospitality) ||
        real_room(temples[i].entrance) != IN_ROOM(ch))
      continue;
    for (mob = world[IN_ROOM(ch)].people; mob; mob = mob->next_in_room)
      if (IS_NPC(mob) && GET_MOB_VNUM(mob) == (mob_vnum)temples[i].priest &&
          mob->mob_deity == deity && mob->mob_temple_rank >= 1 &&
          AWAKE(mob) && !FIGHTING(mob) && CAN_SEE(ch, mob))
        return &temples[i];
  }
  return NULL;
}

bool religion_service_context(struct char_data *ch, int requested, int *temple, int *deity, int *priest)
{
  struct religion_temple *t = NULL;
  struct char_data *mob;
  int index;
  if (!service_position(ch)) return false;
  if (!requested) t = present_priest(ch, GET_DEITY(ch), false);
  else if (temples_ready && IN_ROOM(ch) != NOWHERE && (index = temple_index(requested)) >= 0) {
    struct religion_temple *candidate = &temples[index];
    if (candidate->deity == GET_DEITY(ch) && temple_live(candidate) &&
        real_room(candidate->entrance) == IN_ROOM(ch))
      for (mob = world[IN_ROOM(ch)].people; mob; mob = mob->next_in_room)
        if (IS_NPC(mob) && GET_MOB_VNUM(mob) == (mob_vnum)candidate->priest &&
            mob->mob_deity == candidate->deity && mob->mob_temple_rank >= 1 &&
            AWAKE(mob) && !FIGHTING(mob) && CAN_SEE(ch, mob)) { t = candidate; break; }
  }
  if (!t) {
    send_to_char(ch, "Visit the assigned priest at an available temple of your faith.\r\n");
    return false;
  }
  *temple = t->id; *deity = t->deity; *priest = t->priest;
  return true;
}

static bool has_live_temple(int deity)
{
  int i;
  if (!temples_ready)
    return false;
  for (i = 0; i < temple_count; ++i)
    if (temples[i].deity == deity && temple_live(&temples[i]))
      return true;
  return false;
}

static int find_deity(const char *name)
{
  int i, result = 0, id;
  if (number(name, 1, NUM_DEITIES - 1, &id))
    return religion_deity_valid(id) ? id : 0;
  if (!*name)
    return 0;
  for (i = 1; i < NUM_DEITIES; ++i)
    if (religion_deity_valid(i) && !strcasecmp(name, deity_list[i].name))
      return i;
  for (i = 1; i < NUM_DEITIES; ++i)
    if (religion_deity_valid(i) && is_abbrev(name, deity_list[i].name))
    {
      if (result)
        return 0;
      result = i;
    }
  return result;
}

static bool patron_locked(struct char_data *ch)
{
  return CLASS_LEVEL(ch, CLASS_CLERIC) > 0 || CLASS_LEVEL(ch, CLASS_PALADIN) > 0 ||
         CLASS_LEVEL(ch, CLASS_BLACKGUARD) > 0 || CLASS_LEVEL(ch, CLASS_INQUISITOR) > 0 ||
         CLASS_LEVEL(ch, CLASS_SACRED_FIST) > 0 || GET_1ST_DOMAIN(ch) != DOMAIN_UNDEFINED ||
         GET_2ND_DOMAIN(ch) != DOMAIN_UNDEFINED;
}

static bool domains_match(struct char_data *ch, int deity)
{
  int domains[2], d, i;
  domains[0] = GET_1ST_DOMAIN(ch);
  domains[1] = GET_2ND_DOMAIN(ch);
  for (d = 0; d < 2; ++d)
  {
    if (domains[d] == DOMAIN_UNDEFINED)
      continue;
    for (i = 0; i < 6 && deity_list[deity].domains[i] != domains[d]; ++i)
      ;
    if (i == 6)
      return false;
  }
  return true;
}

static bool service_position(struct char_data *ch)
{
  if (GET_POS(ch) < POS_RESTING || FIGHTING(ch))
  {
    send_to_char(ch, "You must be conscious and outside combat.\r\n");
    return false;
  }
  return true;
}

static void wait_message(struct char_data *ch, time_t deadline, const char *what)
{
  long long remaining = (long long)deadline - (long long)time(NULL);
  remaining = remaining > 0 ? (remaining + 59) / 60 : 0;
  send_to_char(ch, "%s available in %lld hour(s), %lld minute(s).\r\n", what,
               remaining / 60, remaining % 60);
}

static bool join_allowed(struct char_data *ch, int deity)
{
  if (!temples_ready)
  {
    send_to_char(ch, "Temple records are temporarily unavailable. Try devotion later.\r\n");
    return false;
  }
  if (GET_DEITY(ch) != DEITY_NONE)
  {
    send_to_char(ch, "Leave your current faith before devoting yourself to another.\r\n");
    return false;
  }
  if (ch->player_specials->saved.religion_devote_after > time(NULL))
  {
    wait_message(ch, ch->player_specials->saved.religion_devote_after, "Devotion");
    return false;
  }
  if (!domains_match(ch, deity))
  {
    send_to_char(ch, "That patron is incompatible with your domains. Ask staff about a supported change.\r\n");
    return false;
  }
  if (has_live_temple(deity) && !present_priest(ch, deity, false))
  {
    send_to_char(ch, "Visit a registered priest of that faith at its public temple entrance.\r\n");
    return false;
  }
  return true;
}

static void describe_faith(struct char_data *ch, int deity)
{
  int i;
  send_to_char(ch, "%s\r\nPortfolio: %s\r\nSymbol: %s\r\n%s\r\n",
               deity_list[deity].name, deity_list[deity].portfolio,
               deity_list[deity].symbol, deity_list[deity].description);
  send_to_char(ch, "Ranks (Standing): Devotee (0), Faithful (12), Acolyte (36), Votary (72),\r\n"
                   "Steward (120), Preceptor (180), Elder (252), Luminary (336).\r\n"
                   "Membership provides faith identity, the religion channel and configured hospitality.\r\n"
                   "Use religion task for the service board. Blessings and rank appearances are planned.\r\n"
                   "Leaving ends membership services; your Standing is retained. Rejoining or choosing\r\n"
                   "another faith requires seven real days after departure. Divine patrons/domains\r\n"
                   "may require a supported staff change. No additional alignment restrictions apply.\r\n");
  for (i = 1; i <= 8; ++i)
    send_to_char(ch, "  Rank %d: %s\r\n", i, religion_rank_title(deity, i));
  if (!has_live_temple(deity))
    send_to_char(ch, "Temple services not yet available. You can still join for identity and the channel.\r\n");
  else
    for (i = 0; i < temple_count; ++i)
      if (temples[i].deity == deity && temple_live(&temples[i]))
        send_to_char(ch, "Temple: %s (%s); hospitality %s.\r\n", temples[i].name,
                     world[real_room(temples[i].entrance)].name, ONOFF(temples[i].hospitality));
}

static void preview_change(struct char_data *ch, int action, int deity)
{
  struct religion_temple *temple;
  religion_clear_confirmation(ch);
  if (!service_position(ch))
    return;
  if (action == RELIGION_LEAVE)
  {
    if (GET_DEITY(ch) == DEITY_NONE)
    {
      send_to_char(ch, "You have no faith to leave.\r\n");
      return;
    }
    if (patron_locked(ch))
    {
      send_to_char(ch, "Your divine class or domains require a supported patron change. Ask staff.\r\n");
      return;
    }
    send_to_char(ch, "Leaving removes faith access, services and channel membership. Standing and\r\n"
                     "item appearances remain. You must wait seven real days before any new devotion.\r\n");
  }
  else
  {
    if (!religion_deity_valid(deity) || !join_allowed(ch, deity))
      return;
    describe_faith(ch, deity);
    send_to_char(ch, "You will join %s with %d saved Standing.\r\n", deity_list[deity].name,
                 ch->player_specials->saved.religion_standing[deity]);
  }
  temple = action == RELIGION_JOIN ? present_priest(ch, deity, false) : NULL;
  ch->player_specials->religion_pending_action = action;
  ch->player_specials->religion_pending_source = GET_DEITY(ch);
  ch->player_specials->religion_pending_deity = deity;
  ch->player_specials->religion_pending_temple = temple ? temple->id : 0;
  ch->player_specials->religion_pending_revision = temple_revision;
  ch->player_specials->religion_pending_until = time(NULL) + RELIGION_CONFIRM_SECONDS;
  send_to_char(ch, "Use 'religion confirm' within two minutes, or 'religion cancel'.\r\n");
}

static void confirm_change(struct char_data *ch)
{
  struct player_special_data *ps = ch->player_specials;
  struct religion_temple *temple;
  int old_deity = GET_DEITY(ch), action = ps->religion_pending_action, deity = ps->religion_pending_deity;
  time_t old_deadline = ps->saved.religion_devote_after;
  if (!action || ps->religion_pending_until < time(NULL) ||
      ps->religion_pending_source != old_deity || ps->religion_pending_revision != temple_revision)
  {
    religion_clear_confirmation(ch);
    send_to_char(ch, "No current confirmation. Preview the change again.\r\n");
    return;
  }
  if (!service_position(ch))
    return;
  if (action == RELIGION_JOIN)
  {
    if (!religion_deity_valid(deity) || !join_allowed(ch, deity))
      return;
    temple = present_priest(ch, deity, false);
    if ((temple ? temple->id : 0) != ps->religion_pending_temple)
    {
      religion_clear_confirmation(ch);
      send_to_char(ch, "The priest or location has changed; preview devotion again.\r\n");
      return;
    }
    if (old_deity != deity && !religion_blessing_remove(ch))
    { send_to_char(ch, "Unable to persist blessing removal; membership unchanged.\r\n"); return; }
    GET_DEITY(ch) = deity;
  }
  else
  {
    if (patron_locked(ch))
    {
      religion_clear_confirmation(ch);
      send_to_char(ch, "Your divine class or domains now require a supported staff change.\r\n");
      return;
    }
    if (!religion_blessing_remove(ch))
    { send_to_char(ch, "Unable to persist blessing removal; membership unchanged.\r\n"); return; }
    GET_DEITY(ch) = DEITY_NONE;
    ps->saved.religion_devote_after = time(NULL) + RELIGION_CHANGE_SECONDS;
  }
  if (!save_char_checked(ch, 0))
  {
    GET_DEITY(ch) = old_deity;
    ps->saved.religion_devote_after = old_deadline;
    /* Reconcile a possible post-rename durability failure with the old state. */
    if (!save_char_checked(ch, 0))
      log("SYSERR: Religion membership rollback could not persist for player %ld.", GET_IDNUM(ch));
    religion_clear_confirmation(ch);
    send_to_char(ch, "The change could not be saved. Your previous membership is retained.\r\n");
    return;
  }
  religion_clear_confirmation(ch);
  if (action == RELIGION_JOIN)
    send_to_char(ch, "You are now a %s of %s.\r\n", religion_rank_name(religion_rank(ch)), deity_list[deity].name);
  else
    send_to_char(ch, "You have left your faith. Standing is preserved; devotion is available in seven days.\r\n");
}

/* Use the existing forage effect/flavour pairs and ordinary eat/drink bonuses. */
static struct obj_data *hospitality_item(bool food)
{
  static const int effects[] = {APPLY_AC_NEW, APPLY_STR, APPLY_DEX, APPLY_CON,
    APPLY_INT, APPLY_WIS, APPLY_CHA, APPLY_DAMROLL, APPLY_HITROLL,
    APPLY_ENCUMBRANCE, APPLY_HIT, APPLY_MOVE, APPLY_HP_REGEN, APPLY_MV_REGEN,
    APPLY_PSP, APPLY_PSP_REGEN, APPLY_SAVING_FORT, APPLY_SAVING_REFL, APPLY_SAVING_WILL};
  int choices[sizeof(effects) / sizeof(*effects)], count = 0, i, effect, modifier = 2;
  struct obj_data *obj;
  char text[256];
  for (i = 0; i < (int)(sizeof(effects) / sizeof(*effects)); ++i)
    if (!!apply_type_food_or_drink[effects[i]] == food && *apply_type_food_names[effects[i]])
      choices[count++] = effects[i];
  if (!count) return NULL;
  effect = choices[rand_number(0, count - 1)];
  if (effect == APPLY_DAMROLL || effect == APPLY_HITROLL) modifier = 1;
  else if (effect == APPLY_HIT) modifier *= 10;
  else if (effect == APPLY_MOVE) modifier *= 100;
  else if (effect == APPLY_PSP) modifier *= 5;
  obj = create_obj();
  if (!obj) return NULL;
  obj->item_number = NOTHING;
  GET_OBJ_TYPE(obj) = food ? ITEM_FOOD : ITEM_DRINK;
  GET_OBJ_BOUND_ID(obj) = NOBODY;
  GET_OBJ_WEIGHT(obj) = 1;
  GET_OBJ_VAL(obj, 0) = 100; /* Existing consumable buff duration, in game ticks. */
  SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);
  SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_IDENTIFIED);
  obj->affected[0].location = effect;
  obj->affected[0].modifier = modifier;
  obj->affected[0].bonus_type = food ? BONUS_TYPE_FOOD : BONUS_TYPE_DRINK;
  snprintf(text, sizeof(text), "temple %s %s", food ? "food" : "drink", apply_type_food_names[effect]);
  obj->name = strdup(text);
  snprintf(text, sizeof(text), "some fresh %s", apply_type_food_names[effect]);
  obj->short_description = strdup(text);
  snprintf(text, sizeof(text), "Some fresh %s from a temple have been left here.", apply_type_food_names[effect]);
  obj->description = strdup(text);
  return obj;
}

static void hospitality(struct char_data *ch)
{
  struct player_special_data_saved *saved = &ch->player_specials->saved;
  struct obj_data *food, *drink;
  if (!service_position(ch)) return;
  if (!religion_deity_valid(GET_DEITY(ch)) || !present_priest(ch, GET_DEITY(ch), true)) {
    send_to_char(ch, "Visit an affiliated priest with hospitality enabled at its public temple entrance.\r\n");
    return;
  }
  if (saved->religion_hospitality_after > time(NULL)) {
    wait_message(ch, saved->religion_hospitality_after, "Hospitality"); return;
  }
  if (IS_CARRYING_N(ch) > CAN_CARRY_N(ch) - 2 || IS_CARRYING_W(ch) > CAN_CARRY_W(ch) - 2) {
    send_to_char(ch, "Make room to carry a food item and a drink before requesting hospitality.\r\n"); return;
  }
  food = hospitality_item(true);
  drink = hospitality_item(false);
  if (!food || !drink) {
    if (food) extract_obj(food);
    if (drink) extract_obj(drink);
    send_to_char(ch, "The temple's provisions are unavailable. No hospitality use was spent.\r\n");
    return;
  }
  /* The durable journal owns serialized copies; recovery delivers them once. */
  if (estate_hospitality_claim(ch, food, drink, time(NULL), RELIGION_HOSPITALITY_SECONDS)) {
    send_to_char(ch, "The temple offers you fresh food and a drink, each with a consumable blessing.\r\n");
    estate_recover(ch);
  }
  extract_obj(food);
  extract_obj(drink);
}

static bool channel_member(struct char_data *ch, int deity)
{
  return ch && !IS_NPC(ch) && GET_DEITY(ch) == deity &&
         !ch->player_specials->saved.religion_channel_off;
}

static void faith_channel(struct char_data *ch, const char *action, const char *message)
{
  struct descriptor_data *d;
  struct char_data *recipient;
  int deity = GET_DEITY(ch);
  if (!religion_deity_valid(deity) || !channel_member(ch, deity))
  {
    send_to_char(ch, "Join a faith and turn its religion channel on first.\r\n");
    return;
  }
  if (!strcmp(action, "who"))
  {
    send_to_char(ch, "Visible online worshippers of %s:\r\n", deity_list[deity].name);
    for (d = descriptor_list; d; d = d->next)
      if (STATE(d) == CON_PLAYING && !d->original && channel_member(d->character, deity) &&
          CAN_SEE(ch, d->character))
        send_to_char(ch, "  %s\r\n", show_pers(d->character, ch));
    return;
  }
  if (!*message)
  {
    send_to_char(ch, "Say what to your faith?\r\n");
    return;
  }
  if (PLR_FLAGGED(ch, PLR_NOSHOUT) || AFF_FLAGGED(ch, AFF_SILENCED) ||
      (IS_ANIMAL(ch) && !IS_WILDSHAPED(ch)) ||
      (GET_LEVEL(ch) < LVL_STAFF && IN_ROOM(ch) != NOWHERE && ROOM_FLAGGED(IN_ROOM(ch), ROOM_SOUNDPROOF)))
  {
    send_to_char(ch, "You cannot speak on the religion channel right now.\r\n");
    return;
  }
  if (GET_LEVEL(ch) < CONFIG_LEVEL_CAN_SHOUT)
  {
    send_to_char(ch, "You must reach level %d before using this channel.\r\n", CONFIG_LEVEL_CAN_SHOUT);
    return;
  }
  /* Use the normal communication output route, with no public history or bridge.
   * Native channels currently have no player ignore-list facility. */
  ++communication_output_depth;
  for (d = descriptor_list; d; d = d->next)
  {
    recipient = d->character;
    if (STATE(d) != CON_PLAYING || d->original || !recipient || IS_NPC(recipient))
      continue;
    /* Staff monitoring follows the existing opt-out used by clan channels. */
    if (recipient->player_specials->saved.religion_channel_off)
      continue;
    if (!channel_member(recipient, deity) &&
        !(GET_LEVEL(recipient) >= LVL_STAFF && !PRF_FLAGGED(recipient, PRF_NOCLANTALK)))
      continue;
    if (recipient != ch && GET_LEVEL(ch) < LVL_STAFF &&
        (AFF_FLAGGED(recipient, AFF_DEAF) || (IN_ROOM(recipient) != NOWHERE &&
         ROOM_FLAGGED(IN_ROOM(recipient), ROOM_SOUNDPROOF))))
      continue;
    if (recipient == ch && PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(ch, "%s", CONFIG_OK);
    else
      send_to_char(recipient, "[Religion: %s] %s: %s\r\n", deity_list[deity].name,
                   recipient == ch ? "You" : show_pers(ch, recipient), message);
  }
  --communication_output_depth;
}

ACMD(do_religion)
{
  char action[MAX_INPUT_LENGTH];
  const char *rest;
  int deity, i;
  bool old;
  if (IS_NPC(ch) || !ch->player_specials || (ch->desc && ch->desc->original))
    return;
  rest = one_argument(argument, action, sizeof(action));
  if (religion_blessing_command(ch, action, rest)) return;
  if (!strcmp(action, "task")) { religion_tasks_command(ch, rest); return; }
  if (!*action || !strcmp(action, "status"))
  {
    religion_tasks_restore(ch);
    deity = GET_DEITY(ch);
    if (religion_deity_valid(deity))
    {
      send_to_char(ch, "Deity: %s\r\nChurch Standing: %d\r\nChurch rank: %s\r\nReligion channel: %s\r\n",
                   deity_list[deity].name, ch->player_specials->saved.religion_standing[deity],
                   religion_rank_title(deity, religion_rank(ch)), ONOFF(!ch->player_specials->saved.religion_channel_off));
      send_to_char(ch, "%s\r\n", has_live_temple(deity) ? "Temple services are configured; see religion info." :
                   "Temple services not yet available.");
    }
    else
      send_to_char(ch, "You are unaffiliated with an available faith.\r\n");
    religion_tasks_status(ch);
    if (ch->player_specials->saved.religion_devote_after > time(NULL))
      wait_message(ch, ch->player_specials->saved.religion_devote_after, "Devotion");
    if (ch->player_specials->saved.religion_hospitality_after > time(NULL))
      wait_message(ch, ch->player_specials->saved.religion_hospitality_after, "Hospitality");
    return;
  }
  if (!strcmp(action, "list"))
  {
    for (i = 1; i < NUM_DEITIES; ++i)
      if (religion_deity_valid(i))
        send_to_char(ch, "%-24s %s%s\r\n", deity_list[i].name, deity_list[i].portfolio,
                     has_live_temple(i) ? "" : " (temple services not yet available)");
    return;
  }
  if (!strcmp(action, "info") || !strcmp(action, "devote"))
  {
    if (!strcmp(action, "devote")) religion_clear_confirmation(ch);
    deity = find_deity(rest);
    if (!deity)
    {
      send_to_char(ch, "Choose an unambiguous deity name from 'religion list'.\r\n");
      return;
    }
    if (!strcmp(action, "info")) describe_faith(ch, deity);
    else preview_change(ch, RELIGION_JOIN, deity);
    return;
  }
  if (!strcmp(action, "leave")) { preview_change(ch, RELIGION_LEAVE, 0); return; }
  if (!strcmp(action, "confirm")) { confirm_change(ch); return; }
  if (!strcmp(action, "cancel"))
  {
    religion_clear_confirmation(ch);
    send_to_char(ch, "Religion confirmation cancelled.\r\n");
    return;
  }
  if (!strcmp(action, "hospitality")) { hospitality(ch); return; }
  if (!strcmp(action, "say") || !strcmp(action, "who")) { faith_channel(ch, action, rest); return; }
  if (!strcmp(action, "channel"))
  {
    if (strcmp(rest, "on") && strcmp(rest, "off"))
    {
      send_to_char(ch, "Use religion channel on|off.\r\n");
      return;
    }
    old = ch->player_specials->saved.religion_channel_off;
    ch->player_specials->saved.religion_channel_off = !strcmp(rest, "off");
    if (!save_char_checked(ch, 0))
    {
      ch->player_specials->saved.religion_channel_off = old;
      if (!save_char_checked(ch, 0))
        log("SYSERR: Religion channel preference rollback failed for player %ld.", GET_IDNUM(ch));
      send_to_char(ch, "Your preference could not be saved.\r\n");
    }
    else
      send_to_char(ch, "Religion channel %s.\r\n", ONOFF(!ch->player_specials->saved.religion_channel_off));
    return;
  }
  send_to_char(ch, "Religion: status, list, info <deity>, devote <deity>, leave, confirm, cancel,\r\n"
                   "hospitality, task, bless, blessing, say <message>, channel on|off, who.\r\n");
}

static void temple_display(struct char_data *ch, const struct religion_temple *t)
{
  send_to_char(ch, "Temple %d: %s\r\n  Deity: %s (%d); entrance: %d; priest: %d\r\n"
                   "  Enabled: %s; hospitality: %s; live services: %s\r\n", t->id, t->name,
               religion_deity_valid(t->deity) ? deity_list[t->deity].name : "Unavailable", t->deity,
               t->entrance, t->priest, ONOFF(t->enabled), ONOFF(t->hospitality), YESNO(temple_live(t)));
}

ACMD(do_religionadmin)
{
  char action[MAX_INPUT_LENGTH], id_text[MAX_INPUT_LENGTH], field[MAX_INPUT_LENGTH];
  const char *rest;
  struct religion_temple backup[RELIGION_MAX_TEMPLES];
  int old_count, id, index, value, i;
  if (IS_NPC(ch) || GET_LEVEL(ch) < LVL_BUILDER)
    return;
  rest = one_argument(argument, action, sizeof(action));
  if (!strcmp(action, "rankname")) { religion_rank_admin(ch, rest); return; }
  if (!strcmp(action, "chest")) { temple_chest_admin(ch, rest); return; }
  rest = one_argument(rest, id_text, sizeof(id_text));
  if (!strcmp(action, "reload"))
  {
    send_to_char(ch, "%s\r\n", religion_load_temples(temple_path) ? "Temple registry reloaded." :
                 "Invalid registry; services disabled. Correct the file and reload.");
    return;
  }
  if (!temples_ready)
  {
    send_to_char(ch, "The temple registry is unavailable. Correct etc/religion.json and reload.\r\n");
    return;
  }
  if (!*action || !strcmp(action, "list"))
  {
    for (i = 0; i < temple_count; ++i) temple_display(ch, &temples[i]);
    send_to_char(ch, "religionadmin create <id> <deity-id> <name> | show <id> | set <id> <field> <value>\r\n"
                     "Fields: name, deity, entrance, priest, enabled (0|1), hospitality (0|1).\r\n"
                     "religionadmin delete <id> confirm | reload\r\n"
                     "religionadmin task <temple-id> <slot 1..4> <quest-vnum; 0 clears>\r\n"
                     "religionadmin rankname <deity-id> <rank 1..8> <title>\r\n"
                     "religionadmin chest <deity-id> <minimum-rank 1..8> <item-limit 1..10000> (current room)\r\n");
    return;
  }
  if (!number(id_text, 1, INT_MAX, &id))
  {
    send_to_char(ch, "Supply a positive numeric temple ID.\r\n");
    return;
  }
  index = temple_index(id);
  if (!strcmp(action, "task") && index >= 0) {
    religion_tasks_admin(ch, id, temples[index].deity, temples[index].priest, rest);
    return;
  }
  if (!strcmp(action, "show"))
  {
    if (index < 0) send_to_char(ch, "No such temple.\r\n");
    else temple_display(ch, &temples[index]);
    return;
  }
  memcpy(backup, temples, sizeof(backup));
  old_count = temple_count;
  if (!strcmp(action, "create"))
  {
    rest = one_argument(rest, field, sizeof(field));
    if (index >= 0 || temple_count == RELIGION_MAX_TEMPLES ||
        !number(field, 1, NUM_DEITIES - 1, &value) || !religion_deity_valid(value) || !plain_name(rest))
      goto invalid;
    index = temple_count++;
    memset(&temples[index], 0, sizeof(temples[index]));
    temples[index].id = id;
    temples[index].deity = value;
    strlcpy(temples[index].name, rest, sizeof(temples[index].name));
  }
  else if (!strcmp(action, "delete") && index >= 0 && !strcmp(rest, "confirm"))
  {
    memmove(&temples[index], &temples[index + 1], (temple_count - index - 1) * sizeof(temples[0]));
    --temple_count;
    index = -1;
  }
  else if (!strcmp(action, "set") && index >= 0)
  {
    rest = one_argument(rest, field, sizeof(field));
    if (!strcmp(field, "name"))
    {
      if (!plain_name(rest)) goto invalid;
      strlcpy(temples[index].name, rest, sizeof(temples[index].name));
    }
    else
    {
      if (!number(rest, 0, INT_MAX, &value)) goto invalid;
      if (!strcmp(field, "deity") && religion_deity_valid(value)) temples[index].deity = value;
      else if (!strcmp(field, "entrance") && real_room(value) != NOWHERE) temples[index].entrance = value;
      else if (!strcmp(field, "priest") && real_mobile(value) != NOBODY) temples[index].priest = value;
      else if (!strcmp(field, "enabled") && value <= 1) temples[index].enabled = value;
      else if (!strcmp(field, "hospitality") && value <= 1) temples[index].hospitality = value;
      else goto invalid;
    }
    if (temples[index].enabled && !temple_live(&temples[index])) goto invalid;
    for (i = 0; i < temple_count; ++i)
      if (i != index && temples[index].priest && temples[i].priest == temples[index].priest)
        goto invalid;
  }
  else goto invalid;
  if (!save_temples())
  {
    /* A failed directory fsync can follow a successful rename. Re-read the
     * authoritative file instead of keeping an in-memory guess. */
    religion_load_temples(temple_path);
    send_to_char(ch, "Temple save could not be verified. Registry re-read; inspect before retrying.\r\n");
    return;
  }
  ++temple_revision;
  if (index >= 0) temple_display(ch, &temples[index]);
  send_to_char(ch, "Temple registry saved.\r\n");
  mudlog(NRM, LVL_BUILDER, TRUE, "RELIGION: %s %s temple %d", GET_NAME(ch), action, id);
  return;
invalid:
  memcpy(temples, backup, sizeof(temples));
  temple_count = old_count;
  send_to_char(ch, "Invalid temple edit. Use religionadmin list for syntax. Enabled temples require\r\n"
                   "a valid deity, public entrance and unique priest prototype with matching faith\r\n"
                   "and temple rank 1..8. Disable a temple before rebuilding its configuration.\r\n");
}
