/* A church blessing is one durable, real-time package, never a tick affect.
 * SQL is authoritative for both expiry and the shared application cooldown. */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "religion.h"
#include "deities.h"
#include "estate.h"
#ifdef RELIGION_TASK_TEST
extern time_t religion_task_test_time(void);
#define time(unused) religion_task_test_time()
#endif
#define PS(ch) ((ch)->player_specials)
#define DAY 86400
static const int benefits[9][9] = {{0},
                                   {1, 0, 0, 0, 0, 0, 0, 0, 0},
                                   {1, 1, 0, 0, 0, 0, 0, 0, 0},
                                   {2, 1, 15, 0, 0, 0, 0, 0, 0},
                                   {2, 2, 20, 5, 5, 0, 0, 0, 0},
                                   {3, 2, 25, 5, 5, 1, 0, 0, 0},
                                   {3, 3, 30, 7, 10, 1, 1, 0, 0},
                                   {4, 3, 35, 7, 10, 2, 2, 50, 0},
                                   {4, 4, 40, 10, 15, 2, 3, 75, 2}};
static const char *krynn_names[22] = {
    "",          "Paladine", "Mishakal", "Kiri-Jolith", "Habbakuk", "Majere",
    "Branchala", "Solinari", "Gilean",   "Reorx",       "Zivilyn",  "Chislev",
    "Sirrion",   "Shinare",  "Lunitari", "Takhisis",    "Morgion",  "Chemosh",
    "Sargonnas", "Zeboim",   "Hiddukel", "Nuitari"};
static const char *blessing_names[22] = {"",
                                         "Shelter of the Platinum Dragon",
                                         "Mercy of the Blue Lady",
                                         "Oath of the Bison",
                                         "Renewal of the Blue Phoenix",
                                         "Stillness of the Patient Web",
                                         "Harmony of the Harp",
                                         "Light of the White Moon",
                                         "Witness of the Open Book",
                                         "Measure of the Forge",
                                         "Wisdom of the Living Tree",
                                         "Breath of the Wild",
                                         "Dance of the Living Flame",
                                         "Accord of the Griffin's Wing",
                                         "Reflection of the Red Moon",
                                         "Mantle of the Dark Queen",
                                         "Veil of the Hidden Face",
                                         "Vigil of the Silent Gate",
                                         "Resolve of the Red Condor",
                                         "Voice of the Storm Tide",
                                         "Whisper of the Broken Balance",
                                         "Silence of the Black Moon"};
static const char *ceremonies[22] = {
    "",
    "raises an open hand beneath the dragon's outspread wings",
    "traces the sign of infinity over your open palms",
    "holds a horn emblem upright while reciting an oath of honorable service",
    "passes a feather above a bowl of clear water",
    "observes a measured silence before touching the copper spider emblem",
    "sings a short rising phrase and lets its final note settle",
    "draws a silver crescent in the air with a white-robed hand",
    "opens a gray-bound book and speaks a line of remembrance",
    "strikes a small anvil once and waits for its ringing to fade",
    "sets a carved branch between you and a bowl of still water",
    "brushes a feather across a living sprig without breaking its stem",
    "turns a lamp so that its changing light falls across your hands",
    "balances two plain rings on a small set of scales",
    "turns a red disk between lamplight and shadow",
    "names the five heads in a low, formal invocation",
    "lowers a hood and speaks a quiet litany through its folds",
    "sets a skull emblem beside an unlit funerary candle",
    "folds a dark feather against a red condor emblem",
    "swirls salt water around a turtle emblem before letting it settle",
    "tips one pan of a broken scale and closes a hand around its chain",
    "cups a dark disk in both hands and finishes the invocation in a whisper"};
static bool krynn_theme(int deity)
{
  return deity > 0 && deity < 22 && religion_deity_valid(deity) &&
         !strcmp(deity_list[deity].name, krynn_names[deity]);
}
static bool ready(void)
{
  return estate_init();
}
static int active_rank(struct char_data *ch)
{
  int rank;
  if (!ch || IS_NPC(ch) || !PS(ch) || PS(ch)->saved.church_expires <= time(NULL) ||
      PS(ch)->saved.church_deity != GET_DEITY(ch) || !religion_deity_valid(GET_DEITY(ch)))
    return 0;
  rank = PS(ch)->saved.church_rank;
  return rank >= 1 && rank <= 8 ? MIN(rank, religion_rank(ch)) : 0;
}
int religion_blessing_value(struct char_data *ch, enum church_component component)
{
  /* Native player saves temporarily strip equipment/affects to write base
   * pools. Never bake a real-time church contribution into those base stats. */
  if (ch && ch->legendary_serializing)
    return 0;
  return component >= CHURCH_SAVE && component <= CHURCH_INIT ? benefits[active_rank(ch)][component]
                                                              : 0;
}
int religion_blessing_typed(struct char_data *ch, int location, int type)
{
  if (type == BONUS_TYPE_RESISTANCE &&
      (location == APPLY_SAVING_FORT || location == APPLY_SAVING_REFL ||
       location == APPLY_SAVING_WILL || location == APPLY_SAVING_POISON ||
       location == APPLY_SAVING_DEATH))
    return religion_blessing_value(ch, CHURCH_SAVE);
  return 0;
}
static void refresh(struct char_data *ch)
{
  int hp = GET_HIT(ch), move = GET_MOVE(ch);
  PS(ch)->church_runtime_rank = active_rank(ch);
  affect_total(ch);
  /* A reduced maximum cannot cause damage or refill a resource. */
  GET_HIT(ch) = MIN(hp, MAX(1, GET_MAX_HIT(ch)));
  GET_MOVE(ch) = MIN(move, MAX(0, GET_MAX_MOVE(ch)));
}
void religion_blessing_restore(struct char_data *ch)
{
  MYSQL_RES *r;
  MYSQL_ROW row;
  if (!ch || IS_NPC(ch) || !PS(ch))
    return;
  PS(ch)->saved.church_deity = PS(ch)->saved.church_rank = 0;
  PS(ch)->saved.church_expires = PS(ch)->saved.church_available = 0;
  if (ready() && (r = estate_select("SELECT deity,application_rank,expires_at,available_after FROM "
                                    "religion_blessings WHERE player_id=%ld",
                                    GET_IDNUM(ch))))
  {
    if ((row = mysql_fetch_row(r)))
    {
      PS(ch)->saved.church_deity = atoi(row[0]);
      PS(ch)->saved.church_rank = atoi(row[1]);
      PS(ch)->saved.church_expires = (time_t)strtoll(row[2], NULL, 10);
      PS(ch)->saved.church_available = (time_t)strtoll(row[3], NULL, 10);
    }
    mysql_free_result(r);
  }
  religion_blessing_tick(ch);
}
bool religion_blessing_remove(struct char_data *ch)
{
  if (!ch || IS_NPC(ch) || !PS(ch))
    return false;
  if (!ready() ||
      !estate_query(
          "UPDATE religion_blessings SET expires_at=0,application_rank=0 WHERE player_id=%ld",
          GET_IDNUM(ch)))
    return false;
  PS(ch)->saved.church_expires = 0;
  PS(ch)->saved.church_rank = 0;
  refresh(ch);
  return true;
}
void religion_blessing_tick(struct char_data *ch)
{
  int rank;
  if (!ch || IS_NPC(ch) || !PS(ch))
    return;
  rank = active_rank(ch);
  if (PS(ch)->saved.church_expires && (!rank || rank < PS(ch)->saved.church_rank))
  {
    /* Persist trimming: regaining a corrected rank cannot restore lost components. */
    if (ready())
      estate_query(
          "UPDATE religion_blessings SET application_rank=%d,expires_at=%lld WHERE player_id=%ld",
          rank, rank ? (long long)PS(ch)->saved.church_expires : 0LL, GET_IDNUM(ch));
    PS(ch)->saved.church_rank = rank;
    if (!rank)
      PS(ch)->saved.church_expires = 0;
  }
  if (PS(ch)->church_runtime_rank != rank)
    refresh(ch);
}
static void show(struct char_data *ch, int rank)
{
  const int *b = benefits[rank];
  send_to_char(ch,
               "Rank %d: saves +%d resistance, AC +%d deflection, maximum HP +%d, spell potency "
               "+%d%%, spell duration +%d%%, weapon attack +%d morale, DR %d/-, maximum movement "
               "+%d, initiative +%d.\r\n",
               rank, b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7], b[8]);
}
bool religion_blessing_command(struct char_data *ch, const char *action, const char *argument)
{
  int temple, deity, priest, rank, i;
  time_t now = time(NULL);
  MYSQL_RES *r;
  MYSQL_ROW row;
  if (strcmp(action, "bless") && strcmp(action, "blessing"))
    return false;
  religion_blessing_tick(ch);
  rank = religion_rank(ch);
  if (!strcmp(action, "blessing"))
  {
    if (religion_deity_valid(GET_DEITY(ch)))
      send_to_char(ch, "%s: %s\r\n", deity_list[GET_DEITY(ch)].name,
                   krynn_theme(GET_DEITY(ch)) ? blessing_names[GET_DEITY(ch)] : "Church blessing");
    if (rank)
    {
      send_to_char(ch, "Next application package:\r\n");
      show(ch, rank);
    }
    if (active_rank(ch))
    {
      send_to_char(ch, "Active blessing package:\r\n");
      show(ch, active_rank(ch));
    }
    send_to_char(ch, "Stronger bonuses of the same type prevail; native caps apply.\r\n");
    for (i = rank + 1; i <= 8; i++)
    {
      send_to_char(ch, "Locked: ");
      show(ch, i);
    }
    send_to_char(
        ch,
        "Active application rank: %d; remaining: %d seconds. Next application in: %d seconds.\r\n",
        active_rank(ch),
        active_rank(ch) ? MAX(0LL, (long long)PS(ch)->saved.church_expires - now) : 0,
        MAX(0LL, (long long)PS(ch)->saved.church_available - now));
    return true;
  }
  if (!strcmp(argument, "remove"))
  {
    send_to_char(ch, religion_blessing_remove(ch)
                         ? "The whole church blessing is removed; its cooldown remains.\r\n"
                         : "The blessing could not be removed; try again.\r\n");
    return true;
  }
  if (!strcmp(argument, "cancel"))
  {
    PS(ch)->church_preview_until = 0;
    send_to_char(ch, "Blessing preview cancelled.\r\n");
    return true;
  }
  if (!religion_service_context(ch, 0, &temple, &deity, &priest))
    return true;
  if (!rank || PS(ch)->saved.church_available > now)
  {
    send_to_char(ch, "Your church blessing is not available yet.\r\n");
    return true;
  }
  if (!strcmp(argument, "confirm"))
  {
    if (PS(ch)->church_preview_until < now || PS(ch)->church_preview_rank != rank ||
        PS(ch)->church_preview_deity != deity || PS(ch)->church_preview_temple != temple)
    {
      send_to_char(ch, "Preview the blessing again before confirming.\r\n");
      return true;
    }
    PS(ch)->church_preview_until = 0;
    if (!ready() || !estate_query("START TRANSACTION"))
      goto failed;
    if (!estate_query("INSERT IGNORE INTO religion_blessings VALUES(%ld,0,0,0,0)", GET_IDNUM(ch)) ||
        !(r = estate_select(
              "SELECT available_after FROM religion_blessings WHERE player_id=%ld FOR UPDATE",
              GET_IDNUM(ch))))
      goto rollback;
    row = mysql_fetch_row(r);
    bool available = row && strtoll(row[0], NULL, 10) <= now;
    mysql_free_result(r);
    if (!available)
      goto rollback;
    if (!estate_query(
            "UPDATE religion_blessings SET "
            "deity=%d,application_rank=%d,expires_at=%lld,available_after=%lld WHERE player_id=%ld",
            deity, rank, (long long)now + DAY, (long long)now + DAY, GET_IDNUM(ch)) ||
        !estate_query("COMMIT"))
      goto rollback;
    religion_blessing_restore(ch);
    refresh(ch);
    if (krynn_theme(deity))
      send_to_char(ch, "The priest %s. %s settles over you.\r\n", ceremonies[deity],
                   blessing_names[deity]);
    send_to_char(ch,
                 "The priest invokes %s over you. The whole blessing lasts 86400 real seconds; "
                 "your health and movement are not replenished.\r\n",
                 deity_list[deity].name);
    return true;
  rollback:
    estate_query("ROLLBACK");
  failed:
    religion_blessing_restore(ch);
    send_to_char(
        ch,
        "The blessing could not be confirmed. Check religion blessing before trying again.\r\n");
    return true;
  }
  if (*argument)
  {
    send_to_char(ch, "Use religion bless [confirm|cancel|remove].\r\n");
    return true;
  }
  PS(ch)->church_preview_until = now + 120;
  PS(ch)->church_preview_deity = deity;
  PS(ch)->church_preview_rank = rank;
  PS(ch)->church_preview_temple = temple;
  {
    char expiry[64];
    struct tm utc;
    time_t deadline = now + DAY;
    gmtime_r(&deadline, &utc);
    strftime(expiry, sizeof(expiry), "%Y-%m-%d %H:%M:%S UTC", &utc);
    send_to_char(ch, "Expiry if confirmed now: %s.\r\n", expiry);
  }
  show(ch, rank);
  send_to_char(
      ch, "One whole package for 86400 real seconds, renewable after 86400 seconds. No Standing "
          "cost. Use religion bless confirm within two minutes, or religion bless cancel.\r\n");
  return true;
}
