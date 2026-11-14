#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "handler.h"
#include "constants.h"
#include "spells.h"
#include "class.h"
#include "crafting_new.h"
#include "crafting_recipes.h"
#include "clan_web_content.h"
#include "clan_delivery.h"
#include "clan_delivery_runtime.h"
#include "clan_objective_runtime.h"
#include "clan_expeditions.h"
#include "clan_quartermaster.h"
#include "clan_buff_runtime.h"
#include <limits.h>
static struct json_object *get(struct json_object *o, const char *k)
{
  struct json_object *v = NULL;
  if (o)
    json_object_object_get_ex(o, k, &v);
  return v;
}
static int64_t num(struct json_object *o, const char *k)
{
  return json_object_get_int64(get(o, k));
}
static const char *str(struct json_object *o, const char *k)
{
  const char *s = json_object_get_string(get(o, k));
  return s ? s : "";
}
static void text(struct json_object *o, const char *k, const char *s)
{
  json_object_object_add(o, k, json_object_new_string(s));
}
static void integer(struct json_object *o, const char *k, int64_t n)
{
  json_object_object_add(o, k, json_object_new_int64(n));
}
static void flag(struct json_object *o, const char *k, bool b)
{
  json_object_object_add(o, k, json_object_new_boolean(b));
}
static void reason(struct json_object *a, const char *s)
{
  json_object_array_add(a, json_object_new_string(s));
}
static struct json_object *copy(struct json_object *o)
{
  return o ? json_tokener_parse(json_object_to_json_string_ext(o, JSON_C_TO_STRING_PLAIN))
           : json_object_new_array();
}
static bool contains(struct json_object *a, const char *s)
{
  size_t i;
  for (i = 0; i < json_object_array_length(a); i++)
    if (!strcmp(json_object_get_string(json_object_array_get_idx(a, i)), s))
      return true;
  return false;
}
static struct json_object *action(struct json_object *parent, const char *kind, const char *label,
                                  const char *id, struct json_object *why)
{
  struct json_object *o = json_object_new_object();
  text(o, "kind", kind);
  text(o, "label", label);
  text(o, "id", id ? id : "");
  flag(o, "enabled", json_object_array_length(why) == 0);
  json_object_object_add(o, "reasons", why);
  json_object_array_add(get(parent, "actions"), o);
  return o;
}
static struct json_object *choice(struct json_object *parent, const char *id, const char *name,
                                  const char *details)
{
  struct json_object *o = json_object_new_object();
  text(o, "id", id);
  text(o, "name", name);
  text(o, "details", details);
  json_object_object_add(o, "actions", json_object_new_array());
  json_object_array_add(get(parent, "choices"), o);
  return o;
}
static struct json_object *write_reasons(struct char_data *ch, struct json_object *base)
{
  struct json_object *why = copy(base);
  if (!has_quartermaster_in_room(ch))
    reason(why, "Visit a quartermaster.");
  if (FIGHTING(ch))
    reason(why, "Finish combat first.");
  if (GET_CRAFT(ch).craft_duration > 0)
    reason(why, "Finish timed crafting first.");
  if (*ch->player_specials->clan_delivery_lock || *ch->player_specials->clan_qm_lock ||
      *ch->player_specials->clan_objective_pending[0] ||
      *ch->player_specials->clan_objective_pending[1])
    reason(why, "A saved operation is awaiting recovery; use Retry recovery.");
  return why;
}
static void gates(struct json_object *s, const struct clan_progression_config *c,
                  enum clan_progression_feature feature, const char *clan, bool catalog)
{
  struct clan_progression_readiness ready = {true, true, true, true,
                                             CLAN_PROGRESSION_SCHEMA_VERSION};
  struct json_object *why = get(s, "reasons");
  if (!num(s, "ready"))
    reason(why, "Required storage is unavailable; progress has not been reset.");
  if (!catalog)
    reason(why, "World catalog is unavailable or invalid.");
  if (!clan_progression_feature_available(c, feature, &ready, clan))
    reason(why, "This service is disabled for your clan.");
}
static void level_reasons(struct json_object *why, struct char_data *ch, int clanlevel,
                          int needclan, int min, int max)
{
  if (clanlevel < needclan)
    reason(why, "Your clan level is too low.");
  if (GET_LEVEL(ch) < min || GET_LEVEL(ch) > max)
    reason(why, "Your character is outside the allowed level band.");
}
static void contracts(struct char_data *ch, struct json_object *root, struct json_object *content,
                      const struct clan_progression_config *config, int64_t now)
{
  struct clan_delivery_catalog deliveries;
  struct clan_objective_catalog objectives;
  bool dr = clan_delivery_catalog_load(&deliveries), orr = clan_objective_catalog_load(&objectives);
  size_t i, j;
  const char *clan = str(get(root, "clan"), "uid");
  int cl = num(get(root, "clan"), "level");
  if (dr)
    clan_delivery_catalog_rotate(&deliveries, now);
  if (orr)
    clan_objective_catalog_rotate(&objectives, now, GET_LEVEL(ch));
  for (i = 0; i < 3; i++)
  {
    struct json_object *s = json_object_array_get_idx(get(content, "contracts"), i),
                       *active = get(s, "active"), *base, *why, *o;
    const char *family = i == 0 ? "delivery" : i == 1 ? "gathering" : "bounty";
    char details[240], kind[40];
    gates(s, config, CLAN_FEATURE_CONTRACTS, clan, i == 0 ? dr : orr);
    base = get(s, "reasons");
    if (i == 0 && num(s, "recovery_pending"))
      reason(base, "A delivery is awaiting recovery; use Retry recovery.");
    reason(get(s, "notes"), "Base credit: 6.250 lifetime Influence, 6.250 spendable Influence and 12.500 Favor; account, clan "
                            "weekly and balance caps can reduce credit.");
    reason(get(s, "notes"), "Two different definitions per account and family each UTC week; each "
                            "family has its own slot.");
    snprintf(kind, sizeof(kind), "%s_list", family);
    action(s, kind, "List in text", "", copy(base));
    if (num(s, "ready"))
    {
      snprintf(kind, sizeof(kind), "%s_retry", family);
      action(s, kind, "Retry recovery", "", json_object_new_array());
    }
    if (active)
    {
      if (i == 0)
      {
        struct clan_delivery_definition d;
        if (clan_delivery_definition_parse(get(active, "definition"), &d))
          integer(active, "progress", clan_delivery_goods_count(ch, &d));
        text(active, "progress_label", "Eligible goods available");
      }
      else
        text(active, "progress_label",
             i == 1 ? "Saved fresh base units" : "Saved qualifying kills");
      if (strcmp(str(active, "state"), "active"))
        reason(
            get(s, "notes"),
            !strcmp(str(active, "state"), "expired")
                ? "This contract expired at its saved deadline. Uncommitted work cannot be claimed."
                : "The completed reward is pending; Retry recovery preserves its original identity "
                  "and week.");
      if (!num(active, "own_clan"))
        reason(get(s, "notes"), "This accepted contract belongs to a different clan incarnation; "
                                "its original membership requirements still apply.");
      snprintf(kind, sizeof(kind), "%s_active", family);
      action(s, kind, "Show saved contract", "", copy(base));
      if (i == 0)
      {
        why = write_reasons(ch, base);
        if (!num(active, "own_clan"))
          reason(why, "The contract requires its original clan.");
        if (strcmp(str(active, "state"), "active"))
          reason(why, "The contract is expired or awaiting recovery.");
        if (num(active, "progress") < num(active, "required"))
          reason(why, "You do not have enough eligible goods.");
        if (contains(get(s, "completed_ids"), str(active, "definition_id")) ||
            json_object_array_length(get(s, "completed_ids")) >= 2)
          reason(why, "Your account used this definition or family's weekly limit.");
        {
          struct clan_delivery_definition d;
          if (!clan_delivery_definition_parse(get(active, "definition"), &d) ||
              !clan_delivery_goods_available(ch, &d))
            reason(why, "Eligible goods must fit the existing turn-in bundle limit.");
        }
        action(s, "delivery_turnin", "Preview turn-in", "", why);
      }
    }
    if (i == 0 && dr)
      for (j = 0; j < deliveries.count; j++)
      {
        struct clan_delivery_definition *d = &deliveries.entries[j];
        if (d->kind == CLAN_DELIVERY_CRAFTED)
          snprintf(details,sizeof(details),"%d unenhanced crafted %.80s items, carried directly and unbound; clan level %d",d->quantity,crafting_recipes[d->resource].name,d->min_level);
        else if (d->kind == CLAN_DELIVERY_MATERIAL)
          snprintf(details, sizeof(details), "%d units of %.80s, exact grade %d; clan level %d",
                   d->quantity, crafting_materials[d->resource], d->grade, d->min_level);
        else
        {
          static const char *ammo[] = {"", "arrows", "bolts", "bullets"};
          snprintf(details, sizeof(details),
                   "%d finite %s shots, enhancement +%d or better; no infinite/Returning "
                   "ammunition; clan level %d",
                   d->quantity, ammo[d->resource], d->min_enhancement, d->min_level);
        }
        o = choice(s, d->id, d->name, details);
        why = write_reasons(ch, base);
        level_reasons(why, ch, cl, d->min_level, 1, 30);
        if (active && strcmp(str(active, "state"), "expired"))
          reason(why, "Your delivery slot is occupied.");
        if (contains(get(s, "completed_ids"), d->id))
          reason(why, "Your account already completed this definition this week.");
        if (json_object_array_length(get(s, "completed_ids")) >= 2)
          reason(why, "Your account used this family's weekly limit.");
        action(o, "delivery_accept", "Accept", d->id, why);
      }
    if (i > 0 && orr)
      for (j = 0; j < objectives.count; j++)
      {
        struct clan_objective_definition *d = &objectives.entries[j];
        if (d->family != (i == 1 ? 4 : 5))
          continue;
        snprintf(details, sizeof(details), "%d %s; clan level %d, character levels %d-%d",
                 d->required, i == 1 ? "fresh base harvest units" : "qualifying designated kills",
                 d->min_clan_level, d->min_player_level, d->max_player_level);
        o = choice(s, d->id, d->name, details);
        why = write_reasons(ch, base);
        level_reasons(why, ch, cl, d->min_clan_level, d->min_player_level, d->max_player_level);
        if (active && strcmp(str(active, "state"), "expired"))
          reason(why, "This family's contract slot is occupied.");
        if (contains(get(s, "completed_ids"), d->id))
          reason(why, "Your account already completed this definition this week.");
        if (json_object_array_length(get(s, "completed_ids")) >= 2)
          reason(why, "Your account used this family's weekly limit.");
        snprintf(kind, sizeof(kind), "%s_accept", family);
        action(o, kind, "Accept", d->id, why);
      }
    if (i == 0 && active)
    {
      struct json_object *q = json_tokener_parse(ch->player_specials->clan_delivery_quote);
      if (q && num(q, "confirm_before") > now && num(q, "player_id") == GET_IDNUM(ch) &&
          !strcmp(str(q, "clan_uid"), clan) &&
          clan_progression_uid_valid(str(q, "operation_uid")) &&
          !strcmp(str(q, "accepted_uid"), str(active, "uid")))
      {
        struct json_object *preview = json_object_new_object();
        char summary[240];
        struct json_object *delta = get(q, "delta");
        snprintf(summary, sizeof(summary),
                 "Turn in goods for %.3f lifetime Influence, %.3f Influence and %.3f Favor%s.",
                 num(delta, "xp_eighths") / 8.0, num(delta, "influence_eighths") / 8.0,
                 num(delta, "favor_eighths") / 8.0,
                 num(delta, "cap_flags") ? " (reduced by caps)" : "");
        text(preview, "summary", summary);
        integer(preview, "expires_at", num(q, "confirm_before"));
        json_object_object_add(preview, "actions", json_object_new_array());
        why = write_reasons(ch, base);
        if (!num(active, "own_clan") || strcmp(str(active, "state"), "active"))
          reason(why, "The original active contract is required.");
        if (num(active, "progress") < num(active, "required"))
          reason(why, "Eligible goods changed; preview again.");
        if (contains(get(s, "completed_ids"), str(active, "definition_id")) ||
            json_object_array_length(get(s, "completed_ids")) >= 2)
          reason(why, "Your account used this definition or family's weekly limit.");
        action(preview, "delivery_confirm", "Confirm turn-in", str(q, "operation_uid"), why);
        json_object_object_add(s, "preview", preview);
      }
      if (q)
        json_object_put(q);
    }
    if (active)
      json_object_object_del(active, "definition");
  }
}
static void expeditions(struct char_data *ch, struct json_object *root, struct json_object *s,
                        const struct clan_progression_config *config, int64_t now)
{
  struct clan_expedition_catalog catalog;
  struct clan_expedition_world refs = clan_expedition_world_refs(), structural = refs;
  struct json_object *file = json_object_from_file("clan/expeditions.json"), *base,
                     *active = get(s, "active"), *why, *o;
  size_t i, j;
  char details[240];
  bool ok;
  int cl = num(get(root, "clan"), "level");
  structural.available = structural.target;
  ok = file && clan_expedition_catalog_parse(file, &catalog, NULL, 0) &&
       clan_expedition_catalog_validate(&catalog, &structural, NULL, 0);
  if (file)
    json_object_put(file);
  gates(s, config, CLAN_FEATURE_EXPEDITIONS, str(get(root, "clan"), "uid"), ok);
  base = get(s, "reasons");
  reason(get(s, "notes"), "Two to six enrolled accounts; meaningful contribution by at least two "
                          "accounts per encounter. Personal completion needs half the counted "
                          "objectives, rounded up, and the final objective.");
  reason(get(s, "notes"),
         "Personal base: 25 Favor, once per account/family/UTC week. Shared base: 37.500 lifetime Influence and "
         "37.500 spendable Influence once per clan/family/week. Caps can reduce credit.");
  action(s, "expedition_list", "List in text", "", copy(base));
  action(s, "expedition_claims", "Show completion claims", "", copy(base));
  if (active)
  {
    struct json_object *def = get(active, "definition"), *objectives = get(def, "objectives"),
                       *list = get(active, "objectives");
    int total = 0;
    for (i = 0; i < 3; i++)
    {
      struct json_object *d = json_object_array_get_idx(objectives, i),
                         *obj = json_object_new_object(), *targets = json_object_new_array();
      integer(obj, "progress",
              json_object_get_int(json_object_array_get_idx(get(active, "counts"), i)));
      integer(obj, "required", num(d, "quota"));
      total += num(d, "quota");
      for (j = 0; j < json_object_array_length(get(d, "targets")); j++)
      {
        struct json_object *t = json_object_array_get_idx(get(d, "targets"), j);
        char label[200];
        snprintf(
            label, sizeof(label), "%.80s (%.80s)%s", clan_expedition_target_name(num(t, "mob")),
            clan_expedition_area_name(num(t, "area")),
            refs.available(num(t, "area"), num(t, "mob"), refs.context) ? "" : " [unavailable]");
        json_object_array_add(targets, json_object_new_string(label));
      }
      json_object_object_add(obj, "targets", targets);
      json_object_array_add(list, obj);
    }
    integer(active, "personal_required", (total + 1) / 2);
    action(s, "expedition_active", "Check roster / manage in text", "", copy(base));
    why = write_reasons(ch, base);
    level_reasons(why, ch, cl, num(def, "min_clan_level"), num(def, "min_player_level"),
                  num(def, "max_player_level"));
    if (num(active, "enrolled"))
      reason(why, "You are already enrolled.");
    if (num(active, "account_in_roster") && !num(active, "enrolled"))
      reason(why, "This account already has an enrollment in this outing.");
    if (num(active, "removed"))
      reason(why, "A removed enrollment cannot rejoin this outing.");
    if (num(active, "sealed"))
      reason(why, "The roster is sealed.");
    if (num(active, "roster_count") >= 6)
      reason(why, "The six-account roster is full.");
    if (!strcmp(str(active, "state"), "expired"))
      reason(why, "This outing expired.");
    action(s, "expedition_join", "Join outing", str(active, "uid"), why);
    json_object_object_del(active, "definition");
    json_object_object_del(active, "counts");
  }
  if (ok)
    for (i = 0; i < catalog.count; i++)
    {
      struct clan_expedition_definition *d = &catalog.entries[i];
      struct clan_expedition_catalog selected = {0};
      selected.count = 1;
      selected.entries[0] = *d;
      snprintf(details, sizeof(details), "%s; clan level %d, character levels %d-%d",
               d->family == 1 ? "20 defenders and two named objectives"
                              : "10 kills in each of three categories",
               d->min_clan_level, d->min_player_level, d->max_player_level);
      o = choice(s, d->id, d->name, details);
      why = write_reasons(ch, base);
      level_reasons(why, ch, cl, d->min_clan_level, d->min_player_level, d->max_player_level);
      if (active && strcmp(str(active, "state"), "expired"))
        reason(why, "Your clan already has a recruiting or active outing.");
      if (!clan_expedition_catalog_validate(&selected, &refs, NULL, 0))
        reason(why, "At least one objective has no live eligible alternative.");
      action(o, "expedition_create", "Create outing", d->id, why);
    }
  for (i = 0; i < json_object_array_length(get(s, "claims")); i++)
  {
    o = json_object_array_get_idx(get(s, "claims"), i);
    why = write_reasons(ch, base);
    if (!strcmp(str(o, "state"), "claimed"))
      reason(why, "This completion was already claimed.");
    if (!strcmp(str(o, "state"), "unclaimed"))
    {
      if (num(o, "removed"))
        reason(why, "Departure invalidated this unclaimed completion.");
      if (num(o, "expires_at") <= now)
        reason(why, "The original weekly claim deadline passed.");
      if (num(o, "account_claimed"))
        reason(why, "Your account already claimed this expedition family this week.");
    }
    action(o, "expedition_claim",
           !strcmp(str(o, "state"), "reward_pending") ? "Retry accepted claim"
                                                      : "Claim personal Favor",
           str(o, "uid"), why);
  }
}
static void quartermaster(struct char_data *ch, struct json_object *root, struct json_object *s,
                          const struct clan_progression_config *config, int64_t now)
{
  const char *clan = str(get(root, "clan"), "uid");
  gates(s, config, CLAN_FEATURE_QUARTERMASTER, clan, true);
  reason(get(s, "notes"), "Quartermasters offer crafting motes, gold-to-quest-point exchanges, and random weapon enchantments. Use clan quartermaster info for prices, weekly character limits and Favor unlocks.");
  action(s, "quartermaster_list", "Browse services in text", "", copy(get(s, "reasons")));
  action(s, "clan_status", "Retry recovery / check status", "", json_object_new_array());
  (void)ch; (void)now;
}
void clan_web_content_enrich(struct char_data *ch, struct json_object *root, int64_t now)
{
  struct json_object *content = get(root, "content");
  const struct clan_progression_config *config = clan_buff_runtime_policy();
  char note[320];
  if (!content || strcmp(str(root, "status"), "ready") || !ch || IS_NPC(ch))
    return;
  flag(content, "context_ready", true);
  contracts(ch, root, content, config, now);
  expeditions(ch, root, get(content, "expedition"), config, now);
  quartermaster(ch, root, get(content, "quartermaster"), config, now);
  json_object_object_add(content, "notes", json_object_new_array());
  snprintf(note, sizeof(note),
           "Clan contribution weights this week: %.3f. Weekly Influence: %.3f / %lld; "
           "stored maximum %lld Influence. Spending never lowers lifetime Influence or rank.",
           num(content, "weekly_xp_eighths") / 8.0,
           num(content, "weekly_influence_eighths") / 8.0,
           (long long)config->limits.clan_weekly_influence_points,
           (long long)config->limits.stored_influence_points);
  reason(get(content, "notes"),
         *str(content, "limits_warning") ? str(content, "limits_warning") : note);
  reason(get(content, "notes"),
         "Account XP and Favor caps are shown above. Reaching a cap may produce reduced or zero "
         "credit; it never makes repeat claims unlimited. Commands recheck current eligibility and "
         "exact credit.");
  if (strlen(json_object_to_json_string_ext(root, JSON_C_TO_STRING_PLAIN)) > CLAN_WEB_MAX_BYTES)
  {
    json_object_object_del(root, "content");
    content = json_object_new_object();
    integer(content, "version", 1);
    flag(content, "context_ready", false);
    text(content, "reason", "Content report exceeded its safe size; use text commands.");
    json_object_object_add(root, "content", content);
  }
}
