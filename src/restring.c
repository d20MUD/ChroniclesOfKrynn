/* Shared appearance catalogue. Drafts never replace published text until a
 * validated publish. Applications use estate escrow + debit/delivery receipts
 * so old equipment and a charged appearance cannot survive as separate goods. */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "constants.h"
#include "religion.h"
#include "restring.h"
#include "clan.h"
#include "clan_access.h"
#include "crafting_new.h"
#include "assign_wpn_armor.h"
#include "estate.h"
#include "genolc.h"
#include "deities.h"
#include <json-c/json.h>
#include <limits.h>
#include <sys/stat.h>
#include <errno.h>
#ifdef RELIGION_TASK_TEST
int restring_test_stop_after_commit;
#endif
#define PS(ch) ((ch)->player_specials)
static struct json_object *field(struct json_object *o, const char *key)
{
  struct json_object *v = NULL;
  if (o)
    json_object_object_get_ex(o, key, &v);
  return v;
}
static int number(struct json_object *o, const char *key)
{
  return json_object_get_int(field(o, key));
}
static const char *text(struct json_object *o, const char *key)
{
  const char *s = json_object_get_string(field(o, key));
  return s ? s : "";
}
static void setn(struct json_object *o, const char *key, int64_t n)
{
  json_object_object_add(o, key, json_object_new_int64(n));
}
static void sets(struct json_object *o, const char *key, const char *s)
{
  json_object_object_add(o, key, json_object_new_string(s));
}
static bool ready(void)
{
  return estate_init();
}
static struct json_object *load(int id, bool draft)
{
  MYSQL_RES *r;
  MYSQL_ROW row;
  struct json_object *o = NULL;
  r = estate_select("SELECT revision,%s FROM restring_catalogue WHERE id=%d AND deleted=0",
                    draft ? "COALESCE(draft,published)" : "published", id);
  if (r)
  {
    if ((row = mysql_fetch_row(r)) && row[1])
    {
      o = json_tokener_parse(row[1]);
      if (o)
      {
        setn(o, "id", id);
        setn(o, "revision", atoi(row[0]));
      }
    }
    mysql_free_result(r);
  }
  return o;
}
static const char *orgname(int category, int org)
{
  int r;
  if (category == 2)
    return religion_deity_valid(org) ? deity_list[org].name : "invalid deity";
  r = real_clan(org);
  return r >= 0 && r < (int)num_of_clans ? CLAN_NAME(r) : "invalid clan";
}
static bool orgvalid(int category, int org)
{
  int r;
  if (category == 2)
    return religion_deity_valid(org);
  r = real_clan(org);
  return org > 0 && org <= 65534 && r >= 0 && r < (int)num_of_clans;
}
static const char *positions[] = {
    "light",      "finger-right", "finger-left",  "neck",         "neck-second", "body",
    "head",       "legs",         "feet",         "hands",        "arms",        "shield",
    "about",      "waist",        "wrist-right",  "wrist-left",   "mainhand",    "hold",
    "offhand",    "hold-second",  "twohand",      "hold-twohand", "face",        "ammo-pouch",
    "ear-right",  "ear-left",     "eyes",         "badge",        "shoulders",   "ankle-right",
    "ankle-left", "sheath",       "instrument",   "sickle",       "axe-tool",    "knife-tool",
    "pickaxe",    "alchemy",      "armor-hammer", "jewel-pliers", "needle",      "weapon-hammer",
    "back"};
static const int position_bits[NUM_WEARS] = {ITEM_WEAR_TAKE,
                                             ITEM_WEAR_FINGER,
                                             ITEM_WEAR_FINGER,
                                             ITEM_WEAR_NECK,
                                             ITEM_WEAR_NECK,
                                             ITEM_WEAR_BODY,
                                             ITEM_WEAR_HEAD,
                                             ITEM_WEAR_LEGS,
                                             ITEM_WEAR_FEET,
                                             ITEM_WEAR_HANDS,
                                             ITEM_WEAR_ARMS,
                                             ITEM_WEAR_SHIELD,
                                             ITEM_WEAR_ABOUT,
                                             ITEM_WEAR_WAIST,
                                             ITEM_WEAR_WRIST,
                                             ITEM_WEAR_WRIST,
                                             ITEM_WEAR_WIELD,
                                             ITEM_WEAR_TAKE,
                                             ITEM_WEAR_WIELD,
                                             ITEM_WEAR_TAKE,
                                             ITEM_WEAR_WIELD,
                                             ITEM_WEAR_TAKE,
                                             ITEM_WEAR_FACE,
                                             ITEM_WEAR_AMMO_POUCH,
                                             ITEM_WEAR_EAR,
                                             ITEM_WEAR_EAR,
                                             ITEM_WEAR_EYES,
                                             ITEM_WEAR_BADGE,
                                             ITEM_WEAR_SHOULDERS,
                                             ITEM_WEAR_ANKLE,
                                             ITEM_WEAR_ANKLE,
                                             ITEM_WEAR_SHEATH,
                                             ITEM_WEAR_INSTRUMENT,
                                             ITEM_WEAR_CRAFT_SICKLE,
                                             ITEM_WEAR_CRAFT_AXE,
                                             ITEM_WEAR_CRAFT_KNIFE,
                                             ITEM_WEAR_CRAFT_PICKAXE,
                                             ITEM_WEAR_CRAFT_ALCHEMY,
                                             ITEM_WEAR_CRAFT_ARMOR_HAMMER,
                                             ITEM_WEAR_CRAFT_JEWEL_PLIERS,
                                             ITEM_WEAR_CRAFT_NEEDLE,
                                             ITEM_WEAR_CRAFT_WEAPON_HAMMER,
                                             ITEM_WEAR_ON_BACK};
static int norm(int pos)
{
  return pos == WEAR_NECK_2 ? WEAR_NECK_1 : pos;
}
static bool slotvalid(int pos)
{
  return pos >= 0 && pos < NUM_WEARS && pos < (int)(sizeof(positions) / sizeof(*positions));
}
static bool armor_slot(int pos)
{
  return pos == WEAR_BODY || pos == WEAR_HEAD || pos == WEAR_ARMS || pos == WEAR_LEGS ||
         pos == WEAR_HANDS || pos == WEAR_FEET || pos == WEAR_NECK_1;
}
static bool weapon_slot(int pos)
{
  return pos == WEAR_WIELD_1 || pos == WEAR_WIELD_OFFHAND || pos == WEAR_WIELD_2H;
}
static bool valid(struct json_object *o)
{
  int cat = number(o, "category"), rank = number(o, "rank"), pos = number(o, "position"),
      value = number(o, "value");
  const char *kind = text(o, "kind");
  if (*text(o, "deity_name") && strcmp(text(o, "deity_name"), orgname(cat, number(o, "org"))))
    return false;
  if ((cat != 1 && cat != 2) || !orgvalid(cat, number(o, "org")) || rank < 1 ||
      rank > (cat == 1 ? 5 : 8) || !slotvalid(pos))
    return false;
  if (!strcmp(kind, "armor"))
  {
    if (!armor_slot(norm(pos)) || value < 0 || value > 3)
      return false;
  }
  else if (!strcmp(kind, "symbol"))
  {
    if (norm(pos) != WEAR_NECK_1 || value < 0 || value > 3)
      return false;
  }
  else if (!strcmp(kind, "shield"))
  {
    if (pos != WEAR_SHIELD || value < SPEC_ARMOR_TYPE_BUCKLER ||
        value > SPEC_ARMOR_TYPE_TOWER_SHIELD)
      return false;
  }
  else if (!strcmp(kind, "weapon"))
  {
    if (!weapon_slot(pos) || value < 0 || value >= NUM_WEAPON_FAMILIES)
      return false;
  }
  else if (!strcmp(kind, "other"))
  {
    if (weapon_slot(pos) || pos == WEAR_SHIELD || value <= 0 || value >= NUM_ITEM_TYPES ||
        value == ITEM_ARMOR || value == ITEM_WEAPON || value == ITEM_FIREWEAPON)
      return false;
  }
  else
    return false;
  if (strpbrk(text(o, "keywords"), "\r\n\033") || strpbrk(text(o, "short"), "\r\n\033") ||
      strpbrk(text(o, "room"), "\r\n\033") || strchr(text(o, "inspection"), '\033'))
    return false;
  if (strspn(text(o, "keywords"), " \t") == strlen(text(o, "keywords")) ||
      strspn(text(o, "short"), " \t") == strlen(text(o, "short")) ||
      strspn(text(o, "room"), " \t") == strlen(text(o, "room")))
    return false;
  if (strchr(text(o, "keywords"), '~') || strchr(text(o, "short"), '~') ||
      strchr(text(o, "room"), '~') || strchr(text(o, "inspection"), '~'))
    return false;
  return *text(o, "keywords") && strlen(text(o, "keywords")) <= 100 && *text(o, "short") &&
         strlen(text(o, "short")) <= 150 && *text(o, "room") && strlen(text(o, "room")) <= 512 &&
         strlen(text(o, "inspection")) < 2048;
}
static bool matches(struct obj_data *obj, struct json_object *entry)
{
  int pos = number(entry, "position"), v = number(entry, "value"), type = GET_OBJ_TYPE(obj), a;
  const char *kind = text(entry, "kind");
  if (type == ITEM_SPELLBOOK ||
      ((type == ITEM_CONTAINER || type == ITEM_AMMO_POUCH) && obj->contains))
    return false;
  if (!valid(entry) || !CAN_WEAR(obj, position_bits[pos]) ||
      (obj->worn_by && norm(obj->worn_on) != norm(pos)))
    return false;
  if (!strcmp(kind, "symbol"))
    return norm(pos) == WEAR_NECK_1 && type != ITEM_ARMOR && type != ITEM_WEAPON &&
           type != ITEM_FIREWEAPON;
  if (!strcmp(kind, "weapon"))
  {
    a = GET_OBJ_VAL(obj, 0);
    return (type == ITEM_WEAPON || type == ITEM_FIREWEAPON) && a > WEAPON_TYPE_UNDEFINED &&
           a < NUM_WEAPON_TYPES && weapon_list[a].weaponFamily == v;
  }
  if (!strcmp(kind, "shield"))
  {
    a = GET_ARMOR_TYPE(obj);
    return type == ITEM_ARMOR && a == v;
  }
  if (!strcmp(kind, "armor"))
  {
    if (type != ITEM_ARMOR)
      return v == 0 && type == ITEM_WORN;
    a = GET_ARMOR_TYPE(obj);
    return a > SPEC_ARMOR_TYPE_UNDEFINED && a < NUM_SPEC_ARMOR_TYPES &&
           armor_list[a].armorType == v;
  }
  return type == v;
}
static bool eligibility(struct char_data *ch, int cat, int org, int rank)
{
  struct clan_requirement r = {0};
  if (cat == 2)
    return GET_DEITY(ch) == org && religion_rank(ch) >= rank;
  r.clan = org;
  r.favor = rank;
  return clan_access_check(ch, &r, false);
}
static bool service(struct char_data *ch, int cat)
{
  int t, d, p;
  struct char_data *mob;
  if (IS_NPC(ch) || !ch->desc || ch->desc->original || GET_POS(ch) < POS_RESTING || FIGHTING(ch) ||
      GET_CRAFT(ch).craft_duration > 0)
    return false;
  if (cat == 2)
    return religion_service_context(ch, 0, &t, &d, &p);
  if (IN_ROOM(ch) == NOWHERE || !has_quartermaster_in_room(ch))
    return false;
  for (mob = world[IN_ROOM(ch)].people; mob; mob = mob->next_in_room)
    if (IS_NPC(mob) && MOB_FLAGGED(mob, MOB_QUARTERMASTER) && AWAKE(mob) && !FIGHTING(mob) &&
        CAN_SEE(ch, mob))
      return true;
  return false;
}
static int rate(void)
{
  struct json_object *o = json_object_from_file("etc/restring.json"), *v;
  struct stat present;
  int n = 100;
  if (!o && (stat("etc/restring.json", &present) == 0 || errno != ENOENT))
    return 0;
  if (o)
  {
    v = field(o, "gold_per_level");
    if (!v || !json_object_is_type(v, json_type_int) || json_object_get_int64(v) < 1 ||
        json_object_get_int64(v) > INT_MAX / 1000)
      n = 0;
    else
      n = json_object_get_int(v);
    json_object_put(o);
  }
  return n;
}
static int price(struct obj_data *obj)
{
  int level = GET_OBJ_LEVEL(obj), r = rate();
  return level >= 0 && level <= 1000 && r > 0 && r <= INT_MAX / MAX(1, level) ? r * MAX(1, level)
                                                                              : -1;
}
static void preview(struct char_data *ch, struct json_object *o)
{
  int pos = number(o, "position");
  send_to_char(ch,
               "\r\n\tC-- Restring Entry #%d --\tn\r\n"
               "\tcOrganization: \tn%s: %s\r\n"
               "\tcRank:         \tn%d\r\n"
               "\tcPosition:     \tn%s\r\n"
               "\tcMatcher:      \tn%s %d\r\n\r\n"
               "\tcKeywords:   \tn%s\r\n"
               "\tcShort:      \tn%s\r\n"
               "\tcRoom:       \tn%s\r\n"
               "\tcInspection: \tn%s\tn\r\n",
               number(o, "id"), number(o, "category") == 1 ? "Clan" : "Religion",
               orgname(number(o, "category"), number(o, "org")), number(o, "rank"),
               slotvalid(pos) ? positions[pos] : "unset", text(o, "kind"), number(o, "value"),
               text(o, "keywords"), text(o, "short"), text(o, "room"), text(o, "inspection"));
}
void restring_cleanup(struct char_data *ch)
{
  if (!ch || !PS(ch))
    return;
  if (PS(ch)->restring_editor)
    json_object_put(PS(ch)->restring_editor);
  if (PS(ch)->restring_quote)
    json_object_put(PS(ch)->restring_quote);
  PS(ch)->restring_editor = PS(ch)->restring_quote = NULL;
}
static void editor_menu(struct char_data *ch)
{
  struct json_object *s = PS(ch)->restring_editor, *o = field(s, "entry");
  int stage = number(s, "stage"), cat = number(s, "category"), rank = number(s, "rank"),
      page = number(s, "page"), filter = number(s, "filter");
  MYSQL_RES *r;
  MYSQL_ROW row;
  if (stage == 0)
  {
    send_to_char(ch, "\r\n\tC-- Restring Catalogue --\tn\r\n\r\n"
                     "\tG1\tn) Clans\r\n\tG2\tn) Religion\r\n\r\n\tYQ\tn) Quit\r\n");
    return;
  }
  if (stage == 1)
  {
    send_to_char(ch, "\r\n\tC-- %s Restring Ranks --\tn\r\n\r\n", cat == 1 ? "Clan Favor" : "Religion");
    for (rank = 1; rank <= (cat == 1 ? 5 : 8); rank++)
      send_to_char(ch, "\tG%d\tn) %s\r\n", rank, cat == 1 ? "Clan Favor" : religion_rank_name(rank));
    send_to_char(ch, "\r\n\tYB\tn) Back\r\n\tYQ\tn) Quit\r\n");
    return;
  }
  if (stage == 3)
  {
    preview(ch, o);
    send_to_char(
        ch,
        "\r\n\tC-- Edit Fields --\tn\r\n"
        "\tG1\tn) Organization\r\n"
        "\tG2\tn) Rank\r\n"
        "\tG3\tn) Position\r\n"
        "\tG4\tn) Matcher\r\n"
        "\tG5\tn) Keywords\r\n"
        "\tG6\tn) Short description\r\n"
        "\tG7\tn) Room description\r\n"
        "\tG8\tn) Inspection description\r\n"
        "\tG9\tn) Preview\r\n"
        "\r\n\tGS\tn) Save draft\r\n"
        "\tGP\tn) Publish\r\n"
        "\tYB\tn) Back (confirm discarding unsaved changes)\r\n"
        "\tYQ\tn) Quit (confirm discarding unsaved changes)\r\n");
    return;
  }
  if (stage == 4)
  {
    preview(ch, o);
    send_to_char(ch,
                 "\r\n\tRDelete this entry?\tn Existing items retain their appearance.\r\n"
                 "\tRyes\tn) Delete entry\r\n\tYno\tn) Cancel\r\n");
    return;
  }
  if (stage == 5)
  {
    send_to_char(ch, "\r\n\tYDiscard unsaved editor changes?\tn\r\n"
                     "\tRyes\tn) Discard changes\r\n\tGno\tn) Continue editing\r\n");
    return;
  }
  send_to_char(ch, "\r\n\tC-- %s Restring Catalogue --\tn\r\n"
                   "\tcRank: \tn%d   \tcPage: \tn%d   \tcOrganization filter: \tn%d\r\n\r\n",
               cat == 1 ? "Clan" : "Religion", rank, page + 1, filter);
  r = estate_select(
      "SELECT id,revision,published,draft FROM restring_catalogue WHERE deleted=0 ORDER BY id");
  if (r)
  {
    int count = 0;
    while ((row = mysql_fetch_row(r)))
    {
      int j;
      for (j = 0; j < 2; j++)
      {
        struct json_object *e = row[j ? 3 : 2] ? json_tokener_parse(row[j ? 3 : 2]) : NULL;
        if (e && number(e, "category") == cat && number(e, "rank") == rank &&
            (!filter || number(e, "org") == filter))
        {
          if (count >= page * 15 && count < (page + 1) * 15)
            send_to_char(ch, "\tc#%s\tn %s %s / %s: %s\tn\r\n", row[0],
                         j ? "\tYDRAFT\tn" : "\tGPUBLISHED\tn",
                         orgname(cat, number(e, "org")),
                         slotvalid(number(e, "position")) ? positions[number(e, "position")]
                                                          : "unset",
                         text(e, "short"));
          count++;
        }
        if (e)
          json_object_put(e);
      }
    }
    mysql_free_result(r);
  }
  send_to_char(ch, "\r\n\tC-- Catalogue Commands --\tn\r\n"
                   "\tGA\tn) Add entry\r\n"
                   "\tGE <id>\tn) Edit entry\r\n"
                   "\tRD <id>\tn) Delete entry\r\n"
                   "\tGF <org-id>\tn) Filter organization (0 clears)\r\n"
                   "\tGN\tn) Next page\r\n"
                   "\tGV\tn) Previous page\r\n"
                   "\tGC <deity>\tn) Wardrobe coverage\r\n"
                   "\tYB\tn) Back\r\n"
                   "\tYQ\tn) Quit\r\n");
}
static bool store(struct char_data *ch, struct json_object *o, bool publish, bool deleted)
{
  int id = number(o, "id"), rev = number(o, "revision");
  char *q = estate_quote(json_object_to_json_string(o));
  bool ok = false;
  if (!q || !estate_query("START TRANSACTION"))
    goto done;
  if (!id)
  {
    if (!estate_query("INSERT INTO restring_catalogue() VALUES()"))
      goto rollback;
    id = (int)mysql_insert_id(conn);
    setn(o, "id", id);
  }
  {
    MYSQL_RES *locked = estate_select(
        "SELECT revision FROM restring_catalogue WHERE id=%d AND deleted=0 FOR UPDATE", id);
    MYSQL_ROW row;
    bool current = locked && (row = mysql_fetch_row(locked)) && atoi(row[0]) == rev;
    if (locked)
      mysql_free_result(locked);
    if (!current)
      goto rollback;
  }
  if (!estate_query("UPDATE restring_catalogue SET draft=%s%s%s, "
                    "published=%s%s%s,revision=revision+%d,deleted=%d WHERE id=%d AND revision=%d "
                    "AND deleted=0",
                    publish || deleted ? "NULL" : "'", publish || deleted ? "" : q,
                    publish || deleted ? "" : "'",
                    deleted   ? "NULL"
                    : publish ? "'"
                              : "published",
                    publish && !deleted ? q : "", publish && !deleted ? "'" : "",
                    publish || deleted ? 1 : 0, deleted ? 1 : 0, id, rev))
    goto rollback;
  if (!estate_query("INSERT INTO restring_audit(entry_id,revision,editor,action,record) "
                    "VALUES(%d,%d,%ld,'%s','%s')",
                    id, rev + (publish || deleted ? 1 : 0), GET_IDNUM(ch),
                    deleted   ? "delete"
                    : publish ? "publish"
                              : "draft",
                    q) ||
      !estate_query("COMMIT"))
    goto rollback;
  setn(o, "revision", rev + (publish || deleted ? 1 : 0));
  ok = true;
  goto done;
rollback:
  estate_query("ROLLBACK");
done:
  free(q);
  return ok;
}
static void coverage(struct char_data *ch, int org)
{
  int ranks[9][4][7] = {{{0}}},
      slots[] = {WEAR_NECK_1, WEAR_BODY, WEAR_ARMS, WEAR_LEGS, WEAR_HEAD, WEAR_HANDS, WEAR_FEET}, i,
      j, k;
  MYSQL_RES *r;
  MYSQL_ROW row;
  r = estate_select("SELECT id,published FROM restring_catalogue WHERE deleted=0 AND published IS "
                    "NOT NULL ORDER BY id");
  if (r)
  {
    while ((row = mysql_fetch_row(r)))
    {
      struct json_object *o = json_tokener_parse(row[1]);
      if (o && valid(o) && number(o, "category") == 2 && number(o, "org") == org)
      {
        int rank = number(o, "rank"), v = number(o, "value");
        if (v >= 0 && v < 4 &&
            (!strcmp(text(o, "kind"), "armor") || !strcmp(text(o, "kind"), "symbol")))
          for (k = 0; k < 7; k++)
            if (norm(number(o, "position")) == slots[k])
              ranks[rank][v][k]++;
      }
      if (o)
        json_object_put(o);
    }
    mysql_free_result(r);
  }
  send_to_char(ch, "%s published wardrobe coverage (neck body arms legs head hands feet):\r\n",
               orgname(2, org));
  for (i = 1; i <= 8; i++)
    for (j = 0; j < 4; j++)
    {
      send_to_char(ch, "Rank %d style %d:", i, j);
      for (k = 0; k < 7; k++)
        send_to_char(ch, " %s", ranks[i][j][k] ? "ready" : "MISSING");
      send_to_char(ch, "\r\n");
    }
}
ACMD(do_restringedit)
{
  if (IS_NPC(ch) || GET_LEVEL(ch) < LVL_BUILDER || !PS(ch) || !ch->desc || ch->desc->original)
    return;
  if (!ready())
  {
    send_to_char(ch, "Catalogue storage unavailable.\r\n");
    return;
  }
  if (!PS(ch)->restring_editor)
    PS(ch)->restring_editor = json_object_new_object();
  editor_menu(ch);
}
bool restring_editor_input(struct char_data *ch, const char *input)
{
  struct json_object *s, *o;
  int stage, prompt, n, i;
  char word[MAX_INPUT_LENGTH];
  const char *rest;
  if (!ch || IS_NPC(ch) || !PS(ch) || !(s = PS(ch)->restring_editor))
    return false;
  if (GET_LEVEL(ch) < LVL_BUILDER || !ch->desc || ch->desc->original)
  {
    restring_cleanup(ch);
    return true;
  }
  stage = number(s, "stage");
  prompt = number(s, "prompt");
  o = field(s, "entry");
  rest = one_argument(input, word, sizeof(word));
  n = atoi(rest);
  if (!prompt && !strcasecmp(word, "q"))
  {
    if (stage == 3 && number(s, "dirty"))
    {
      setn(s, "quit", 1);
      setn(s, "stage", 5);
      editor_menu(ch);
      return true;
    }
    restring_cleanup(ch);
    send_to_char(ch, "Restring editor closed.\r\n");
    return true;
  }
  if (prompt)
  {
    if (prompt == 8 && strcmp(input, "@") && strcmp(input, "-"))
    {
      const char *previous = text(s, "inspection_buffer");
      char buffer[2048];
      if (strchr(input, '~') || strchr(input, '\033') ||
          strlen(previous) + strlen(input) + 2 >= sizeof(buffer))
      {
        send_to_char(ch, "Inspection text exceeds 2047 characters or contains unsupported escapes. "
                         "End with @.\r\n");
        return true;
      }
      snprintf(buffer, sizeof(buffer), "%s%s\r\n", previous, input);
      sets(s, "inspection_buffer", buffer);
      return true;
    }
    if (prompt == 8)
    {
      sets(o, "inspection", !strcmp(input, "-") ? "" : text(s, "inspection_buffer"));
      setn(s, "prompt", 0);
      setn(s, "dirty", 1);
      editor_menu(ch);
      return true;
    }
    if (prompt == 1 || prompt == 2)
    {
      setn(o, prompt == 1 ? "org" : "rank", atoi(input));
      if (prompt == 1 && number(o, "category") == 2)
        sets(o, "deity_name", orgname(2, atoi(input)));
    }
    else if (prompt == 3)
    {
      int pos = -1;
      for (i = 0; i < (int)(sizeof(positions) / sizeof(*positions)); i++)
        if (!strcasecmp(input, positions[i]))
          pos = i;
      if (pos < 0 && *input && strspn(input, "0123456789") == strlen(input))
        pos = atoi(input);
      setn(o, "position", pos);
      sets(o, "kind", "");
      setn(o, "value", -1);
    }
    else if (prompt == 4)
    {
      char *end;
      long value;
      skip_spaces_c(&rest);
      errno = 0;
      value = strtol(rest, &end, 10);
      while (*end && isspace((unsigned char)*end))
        end++;
      if (!*rest || !isdigit((unsigned char)*rest) || errno || *end ||
          value < 0 || value > INT_MAX)
      {
        send_to_char(ch, "Enter a matcher and number, for example: symbol 0 or armor 1.\r\n");
        return true;
      }
      sets(o, "kind", word);
      setn(o, "value", value);
    }
    else
    {
      const char *keys[] = {"keywords", "short", "room", "inspection"};
      size_t limits[] = {100, 150, 512, 2047};
      i = prompt - 5;
      if (i < 0 || i > 3 || strlen(input) > limits[i] || strchr(input, '~') ||
          strchr(input, '\033'))
      {
        send_to_char(ch, "Text rejected: length, tilde or terminal escape.\r\n");
        return true;
      }
      sets(o, keys[i], !strcmp(input, "-") ? "" : input);
    }
    setn(s, "prompt", 0);
    setn(s, "dirty", 1);
    editor_menu(ch);
    return true;
  }
  if (stage == 0)
  {
    i = atoi(word);
    if (i == 1 || i == 2)
    {
      setn(s, "category", i);
      setn(s, "stage", 1);
    }
  }
  else if (stage == 1)
  {
    i = atoi(word);
    if (!strcasecmp(word, "b"))
      setn(s, "stage", 0);
    else if (i >= 1 && i <= (number(s, "category") == 1 ? 5 : 8))
    {
      setn(s, "rank", i);
      setn(s, "page", 0);
      setn(s, "stage", 2);
    }
  }
  else if (stage == 2)
  {
    if (!strcasecmp(word, "b"))
      setn(s, "stage", 1);
    else if (!strcasecmp(word, "f"))
    {
      setn(s, "filter", MAX(0, n));
      setn(s, "page", 0);
    }
    else if (!strcasecmp(word, "n"))
      setn(s, "page", number(s, "page") + 1);
    else if (!strcasecmp(word, "v"))
      setn(s, "page", MAX(0, number(s, "page") - 1));
    else if (!strcasecmp(word, "c"))
    {
      coverage(ch, n);
      return true;
    }
    else if (!strcasecmp(word, "a"))
    {
      o = json_object_new_object();
      setn(o, "category", number(s, "category"));
      setn(o, "rank", number(s, "rank"));
      setn(o, "position", -1);
      json_object_object_add(s, "entry", o);
      setn(s, "dirty", 1);
      setn(s, "stage", 3);
    }
    else if (!strcasecmp(word, "e") || !strcasecmp(word, "d"))
    {
      o = load(n, true);
      if (!o || number(o, "category") != number(s, "category") ||
          number(o, "rank") != number(s, "rank"))
      {
        if (o)
          json_object_put(o);
        send_to_char(ch, "No entry in this category and rank.\r\n");
        return true;
      }
      json_object_object_add(s, "entry", o);
      setn(s, "dirty", 0);
      setn(s, "stage", !strcasecmp(word, "d") ? 4 : 3);
    }
  }
  else if (stage == 3)
  {
    i = atoi(word);
    if (i == 9)
    {
      preview(ch, o);
      return true;
    }
    if (i >= 1 && i <= 8)
    {
      setn(s, "prompt", i);
      if (i == 8)
      {
        sets(s, "inspection_buffer", "");
        send_to_char(ch, "Enter inspection text on multiple lines; @ finishes, - clears.\r\n");
        return true;
      }
      send_to_char(ch, "\r\n\tCEnter field %d\tn (single line; '-' clears optional text):\r\n", i);
      if (i == 1)
      {
        int cat = number(o, "category");
        if (cat == 2)
        {
          for (i = 1; i < NUM_DEITIES; i++)
            if (religion_deity_valid(i))
              send_to_char(ch, "%d %s\r\n", i, deity_list[i].name);
        }
        else
          for (i = 0; i < (int)num_of_clans; i++)
            send_to_char(ch, "%d %s\r\n", clan_list[i].vnum, CLAN_NAME(i));
      }
      if (number(s, "prompt") == 3)
        for (i = 0; i < (int)(sizeof(positions) / sizeof(*positions)); i++)
          send_to_char(ch, "%d %s\r\n", i, positions[i]);
      if (number(s, "prompt") == 4)
      {
        send_to_char(ch, "\tGarmor <0..3>\tn  Clothing / light / medium / heavy\r\n"
                         "\tGsymbol <0..3>\tn Neck style\r\n"
                         "\tGshield <subtype>\tn Shield subtype listed below\r\n"
                         "\tGweapon <family>\tn Weapon family listed below\r\n"
                         "\tGother <item-type>\tn Other item type\r\n\r\n");
        for (i = SPEC_ARMOR_TYPE_BUCKLER; i <= SPEC_ARMOR_TYPE_TOWER_SHIELD; i++)
          send_to_char(ch, "shield %d: %s\r\n", i, armor_list[i].name);
        for (i = 0; i < NUM_WEAPON_FAMILIES; i++)
          send_to_char(ch, "weapon %d: %s\r\n", i, weapon_family[i]);
      }
      return true;
    }
    if (!strcasecmp(word, "b"))
      setn(s, "stage", number(s, "dirty") ? 5 : 2);
    if (!strcasecmp(word, "s") || !strcasecmp(word, "p"))
    {
      bool pub = !strcasecmp(word, "p");
      if (pub && !valid(o))
      {
        if (number(o, "value") < 0)
          send_to_char(ch, "Cannot publish: matcher number is missing or invalid. Choose 4 and "
                           "enter a matcher and number, for example: symbol 0.\r\n");
        else
          send_to_char(ch, "Cannot publish: organization, rank, position, matcher or required text "
                           "is invalid.\r\n");
        return true;
      }
      if (number(o, "category") == 1 && !can_edit_clan(ch, number(o, "org")))
      {
        send_to_char(ch, "You cannot edit that clan.\r\n");
        return true;
      }
      if (store(ch, o, pub, false))
      {
        setn(s, "dirty", 0);
        send_to_char(ch, "Saved.\r\n");
      }
      else
        send_to_char(ch, "Save failed or stale; reopen the entry.\r\n");
    }
  }
  else if (stage == 4)
  {
    if (!strcasecmp(word, "yes"))
    {
      if (number(o, "category") == 1 && !can_edit_clan(ch, number(o, "org")))
      {
        send_to_char(ch, "You cannot edit that clan.\r\n");
        return true;
      }
      send_to_char(ch, store(ch, o, false, true) ? "Deleted; audit retained.\r\n"
                                                 : "Delete failed.\r\n");
    }
    setn(s, "stage", 2);
  }
  else if (stage == 5)
  {
    if (!strcasecmp(word, "yes") && number(s, "quit"))
    {
      restring_cleanup(ch);
      send_to_char(ch, "Restring editor closed.\r\n");
      return true;
    }
    setn(s, "quit", 0);
    setn(s, "stage", !strcasecmp(word, "yes") ? 2 : 3);
  }
  editor_menu(ch);
  return true;
}
static struct obj_data *owned_id(struct char_data *ch, long id)
{
  struct obj_data *obj;
  int i;
  for (obj = ch->carrying; obj; obj = obj->next_content)
    if (obj->id == id)
      return obj;
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i) && GET_EQ(ch, i)->id == id)
      return GET_EQ(ch, i);
  return NULL;
}
static int standing(struct char_data *ch, int cat, int org)
{
  int i;
  if (cat == 2)
    return GET_DEITY(ch) == org ? religion_rank(ch) : 0;
  for (i = 5; i >= 1; i--)
    if (eligibility(ch, cat, org, i))
      return i;
  return 0;
}
static bool unchanged(struct char_data *ch, struct obj_data *obj, struct json_object *q)
{
  char *packed;
  bool same;
  if (!obj || (obj->carried_by != ch && obj->worn_by != ch) ||
      number(q, "room") != (int)GET_ROOM_VNUM(IN_ROOM(ch)) ||
      number(q, "position") != (obj->worn_by ? obj->worn_on : -1) ||
      price(obj) != number(q, "price"))
    return false;
  packed = estate_pack(obj);
  same = packed && !strcmp(packed, text(q, "object"));
  free(packed);
  return same;
}
static bool bindings_allow(struct char_data *ch, struct obj_data *obj, struct json_object *e)
{
  struct clan_requirement c = obj->clan_requirement;
  struct religion_requirement r = obj->religion_requirement;
  if (number(e, "category") == 1)
  {
    c.clan = number(e, "org");
    c.favor = number(e, "rank");
  }
  else
  {
    r.deity = number(e, "org");
    r.rank = number(e, "rank");
  }
  return clan_access_check(ch, &c, false) && religion_purchase_allows(ch, r.deity, r.rank, false);
}
static bool noop(struct obj_data *obj, struct json_object *e)
{
  bool binding = number(e, "category") == 1
                     ? obj->clan_requirement.clan == number(e, "org") &&
                           obj->clan_requirement.favor == number(e, "rank")
                     : obj->religion_requirement.deity == number(e, "org") &&
                           obj->religion_requirement.rank == number(e, "rank");
  return binding && !obj->ex_description &&
         !strcmp(obj->name ? obj->name : "", text(e, "keywords")) &&
         !strcmp(obj->short_description ? obj->short_description : "", text(e, "short")) &&
         !strcmp(obj->description ? obj->description : "", text(e, "room")) &&
         !strcmp(obj->restring_inspection, text(e, "inspection"));
}
static void free_old_text(struct obj_data *obj, char *name, char *shortdesc, char *room,
                          struct extra_descr_data *extras)
{
  struct obj_data *proto = VALID_OBJ_RNUM(obj) ? &obj_proto[GET_OBJ_RNUM(obj)] : NULL;
  if (name && (!proto || name != proto->name))
    free(name);
  if (shortdesc && (!proto || shortdesc != proto->short_description))
    free(shortdesc);
  if (room && (!proto || room != proto->description))
    free(room);
  free_ex_descriptions_shared(extras, proto ? proto->ex_description : NULL);
}
static void apply(struct char_data *ch, struct obj_data *obj, struct json_object *e, int cost)
{
  unsigned long long escrow, old_delivery;
  char *name = obj->name, *shortdesc = obj->short_description, *room = obj->description;
  char old_inspection[sizeof(obj->restring_inspection)];
  bool old_clear = obj->restring_extras_cleared;
  struct extra_descr_data *extras = obj->ex_description;
  struct clan_requirement clan = obj->clan_requirement;
  struct religion_requirement faith = obj->religion_requirement;
  bool committed = false;
  if (noop(obj, e))
  {
    send_to_char(
        ch, "That appearance and its requirements are already applied; no fee is charged.\r\n");
    return;
  }
  /* Checkpoint the outgoing instance before changing any text. The transaction
  * commits its removal, new appearance and one fee-bearing delivery together.
  * The live instance becomes that delivery, preserving its scripts and id.
  * After a crash the native codec restores the same durable item identity. */
  PS(ch)->restring_pending = true; /* Persisted before the first escrow checkpoint. */
  escrow = estate_prepare_escrow(ch, obj);
  if (!escrow)
  {
    PS(ch)->restring_pending = false;
    send_to_char(ch, "Could not checkpoint this item; no application was accepted.\r\n");
    return;
  }
  old_delivery = obj->estate_delivery_id;
  strlcpy(old_inspection, obj->restring_inspection, sizeof(old_inspection));
  obj->name = strdup(text(e, "keywords"));
  obj->short_description = strdup(text(e, "short"));
  obj->description = strdup(text(e, "room"));
  obj->ex_description = NULL;
  obj->restring_extras_cleared = true;
  strlcpy(obj->restring_inspection, text(e, "inspection"), sizeof(obj->restring_inspection));
  if (number(e, "category") == 1)
  {
    obj->clan_requirement.clan = number(e, "org");
    obj->clan_requirement.favor = number(e, "rank");
  }
  else
  {
    obj->religion_requirement.deity = number(e, "org");
    obj->religion_requirement.rank = number(e, "rank");
  }
  if (estate_query("START TRANSACTION") && estate_commit_escrow(escrow) &&
      estate_queue_delivery(ch, obj, cost))
    committed = estate_query("COMMIT");
  if (!committed)
  {
    estate_query("ROLLBACK");
    free(obj->name);
    free(obj->short_description);
    free(obj->description);
    obj->name = name;
    obj->short_description = shortdesc;
    obj->description = room;
    obj->ex_description = extras;
    obj->clan_requirement = clan;
    obj->religion_requirement = faith;
    obj->restring_extras_cleared = old_clear;
    strlcpy(obj->restring_inspection, old_inspection, sizeof(obj->restring_inspection));
    obj->estate_delivery_id = old_delivery;
    /* An ambiguous COMMIT is resolved by the existing idempotent journal before
   * any more commands can expose or spend this item. */
    PS(ch)->restring_pending = true;
    estate_recover(ch);
    send_to_char(ch, "Application was not acknowledged. Persistent storage will reconcile it "
                     "before further commands.\r\n");
    return;
  }
  free_old_text(obj, name, shortdesc, room, extras);
#ifdef RELIGION_TASK_TEST
  if (restring_test_stop_after_commit)
    return;
#endif
  PS(ch)->restring_pending = true;
  estate_recover(ch);
  if (estate_player_ready(ch))
  {
    PS(ch)->restring_pending = false;
    send_to_char(ch, "Appearance saved for %d gold. All old extra descriptions were erased.\r\n",
                 cost);
  }
}
static void choose(struct char_data *ch, struct json_object *q, int id)
{
  struct json_object *e = load(id, false), *options = field(q, "options");
  struct obj_data *obj = owned_id(ch, (long)json_object_get_int64(field(q, "item")));
  int i;
  bool offered = false;
  for (i = 0; i < (int)json_object_array_length(options); i++)
    if (json_object_get_int(json_object_array_get_idx(options, i)) == id)
      offered = true;
  if (!e || !offered || !unchanged(ch, obj, q) || !matches(obj, e) ||
      !eligibility(ch, number(e, "category"), number(e, "org"), number(e, "rank")) ||
      !service(ch, number(e, "category")))
  {
    send_to_char(ch,
                 "That selection is no longer available. Request a fresh appearance preview.\r\n");
    if (e)
      json_object_put(e);
    return;
  }
  setn(q, "entry", id);
  setn(q, "revision", number(e, "revision"));
  setn(q, "standing", standing(ch, number(e, "category"), number(e, "org")));
  send_to_char(ch, "Item id %ld, level %d: %s\r\n", obj->id, GET_OBJ_LEVEL(obj),
               obj->short_description);
  preview(ch, e);
  send_to_char(
      ch,
      "Fee: %d gold. All existing extra descriptions on this item will be erased.\r\nResulting "
      "clan: %d, officer gate: %d, Favor gate: %d; religion: %d, temple rank: %d.\r\nUse "
      "restringapply confirm or restringapply cancel within two minutes.\r\n",
      number(q, "price"),
      number(e, "category") == 1 ? number(e, "org") : obj->clan_requirement.clan,
      obj->clan_requirement.rank,
      number(e, "category") == 1 ? number(e, "rank") : obj->clan_requirement.favor,
      number(e, "category") == 2 ? number(e, "org") : obj->religion_requirement.deity,
      number(e, "category") == 2 ? number(e, "rank") : obj->religion_requirement.rank);
  json_object_put(e);
}
static char *last_word(char *input)
{
  char *end = input + strlen(input), *start;
  while (end > input && isspace((unsigned char)end[-1]))
    --end;
  *end = '\0';
  start = end;
  while (start > input && !isspace((unsigned char)start[-1]))
    --start;
  if (start == input)
    return NULL;
  start[-1] = '\0';
  return start;
}
ACMD(do_restringapply)
{
  char input[MAX_INPUT_LENGTH], a[MAX_INPUT_LENGTH], *last, *before, *item;
  const char *rest;
  struct obj_data *obj = NULL;
  struct json_object *q, *e, *options;
  MYSQL_RES *r;
  MYSQL_ROW row;
  int cat, rank, n = 1, count = 0, only = 0, cost, i;
  if (IS_NPC(ch) || !PS(ch) || !ch->desc || ch->desc->original)
    return;
  rest = one_argument(argument, a, sizeof(a));
  q = PS(ch)->restring_quote;
  if (!strcmp(a, "cancel"))
  {
    if (q)
      json_object_put(q);
    PS(ch)->restring_quote = NULL;
    send_to_char(ch, "Appearance preview cancelled.\r\n");
    return;
  }
  if (!ready() || !estate_player_ready(ch))
  {
    send_to_char(ch, "Appearance storage is unavailable or a transfer is pending.\r\n");
    return;
  }
  if (!strcmp(a, "choose"))
  {
    if (q && json_object_get_int64(field(q, "until")) >= time(NULL))
      choose(ch, q, atoi(rest));
    else
      send_to_char(ch, "Request a fresh preview.\r\n");
    return;
  }
  if (!strcmp(a, "confirm"))
  {
    if (!q || json_object_get_int64(field(q, "until")) < time(NULL))
    {
      send_to_char(ch, "Request a fresh preview.\r\n");
      return;
    }
    PS(ch)->restring_quote = NULL;
    e = load(number(q, "entry"), false);
    obj = owned_id(ch, (long)json_object_get_int64(field(q, "item")));
    if (!e || number(e, "revision") != number(q, "revision") || !unchanged(ch, obj, q) ||
        !matches(obj, e) ||
        !eligibility(ch, number(e, "category"), number(e, "org"), number(e, "rank")) ||
        standing(ch, number(e, "category"), number(e, "org")) != number(q, "standing") ||
        !service(ch, number(e, "category")) || !bindings_allow(ch, obj, e) ||
        (GET_GOLD(ch) < number(q, "price") && !noop(obj, e)))
      send_to_char(ch, "The quote is stale, unavailable, unaffordable, or its resulting "
                       "requirements are not met. Preview again.\r\n");
    else
      apply(ch, obj, e, number(q, "price"));
    if (e)
      json_object_put(e);
    json_object_put(q);
    return;
  }
  if (!*a)
  {
    send_to_char(ch, "Use restringapply <owned item> clan|religion <rank>, then choose <id>, "
                     "confirm or cancel.\r\nPublished rewards you can request:\r\n");
    r = estate_select("SELECT id,published FROM restring_catalogue WHERE deleted=0 AND published "
                      "IS NOT NULL ORDER BY id");
    if (r)
    {
      while ((row = mysql_fetch_row(r)))
      {
        e = json_tokener_parse(row[1]);
        if (e && valid(e) &&
            eligibility(ch, number(e, "category"), number(e, "org"), number(e, "rank")))
          send_to_char(ch, "#%s %s %s rank %d %s: %s\r\n", row[0],
                       number(e, "category") == 1 ? "clan" : "religion",
                       orgname(number(e, "category"), number(e, "org")), number(e, "rank"),
                       positions[number(e, "position")], text(e, "short"));
        if (e)
          json_object_put(e);
      }
      mysql_free_result(r);
    }
    return;
  }
  strlcpy(input, argument, sizeof(input));
  last = last_word(input);
  if (!last)
    goto usage;
  rank = atoi(last);
  before = last_word(input);
  if (!before)
    goto usage;
  cat = !strcasecmp(before, "clan") ? 1 : !strcasecmp(before, "religion") ? 2 : 0;
  if (!cat || rank < 1 || rank > (cat == 1 ? 5 : 8) || strspn(last, "0123456789") != strlen(last))
    goto usage;
  if (!service(ch, cat))
  {
    send_to_char(
        ch, "Visit an authorized priest or clan quartermaster outside combat and crafting.\r\n");
    return;
  }
  item = input;
  while (*item == ' ')
    item++;
  i = (int)strlen(item);
  while (i > 0 && item[i - 1] == ' ')
    item[--i] = '\0';
  if (i >= 2 && ((*item == '"' && item[i - 1] == '"') || (*item == '\'' && item[i - 1] == '\'')))
  {
    item[i - 1] = '\0';
    item++;
  }
  n = get_number(&item);
  if (n <= 0)
  {
    send_to_char(ch, "Choose a positive item number.\r\n");
    return;
  }
  obj = get_obj_in_list_vis(ch, item, &n, ch->carrying);
  if (!obj)
    obj = get_obj_in_equip_vis(ch, item, &n, ch->equipment);
  if (!obj)
  {
    send_to_char(ch, "You must carry or wear the exact item.\r\n");
    return;
  }
  cost = price(obj);
  if (cost < 0)
  {
    send_to_char(ch, "This item's level or service price is invalid.\r\n");
    return;
  }
  if (q)
    json_object_put(q);
  PS(ch)->restring_quote = q = json_object_new_object();
  options = json_object_new_array();
  json_object_object_add(q, "options", options);
  setn(q, "item", obj->id);
  setn(q, "until", time(NULL) + 120);
  setn(q, "position", obj->worn_by ? obj->worn_on : -1);
  setn(q, "room", GET_ROOM_VNUM(IN_ROOM(ch)));
  setn(q, "price", cost);
  {
    char *packed = estate_pack(obj);
    if (!packed)
    {
      json_object_put(q);
      PS(ch)->restring_quote = NULL;
      return;
    }
    sets(q, "object", packed);
    free(packed);
  }
  r = estate_select("SELECT id,published FROM restring_catalogue WHERE deleted=0 AND published IS "
                    "NOT NULL ORDER BY id");
  if (r)
  {
    while ((row = mysql_fetch_row(r)))
    {
      e = json_tokener_parse(row[1]);
      if (e && number(e, "category") == cat && number(e, "rank") == rank &&
          eligibility(ch, cat, number(e, "org"), rank) && matches(obj, e))
      {
        only = atoi(row[0]);
        json_object_array_add(options, json_object_new_int(only));
        count++;
        send_to_char(ch, "#%d %s %s %d: %s\r\n", only, positions[number(e, "position")],
                     text(e, "kind"), number(e, "value"), text(e, "short"));
      }
      if (e)
        json_object_put(e);
    }
    mysql_free_result(r);
  }
  if (count == 1)
    choose(ch, q, only);
  else if (count > 1)
    send_to_char(ch, "Select a position/style with restringapply choose <id>. Only authored "
                     "matches are listed.\r\n");
  else
  {
    send_to_char(
        ch, "No authored appearance is available for that item, organization and exact rank.\r\n");
    json_object_put(q);
    PS(ch)->restring_quote = NULL;
  }
  return;
usage:
  send_to_char(ch, "Use restringapply <item> clan|religion <rank>.\r\n");
}
