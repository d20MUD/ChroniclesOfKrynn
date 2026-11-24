/* Church service boards reuse native quest credit, with durable weekly rewards. */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "quest.h"
#include "religion.h"
#include "deities.h"
#include "estate.h"
#include "oasis.h"

#ifdef RELIGION_TASK_TEST
extern time_t religion_task_test_time(void);
#define time(unused) religion_task_test_time()
#endif

#define TASK_WEEK_SECONDS 604800
#define TASK_PREVIEW_SECONDS 120
static char rank_titles[RELIGION_DEITY_CAPACITY][9][100];

/* Unix day zero was Thursday; a Monday calendar week begins four days later. */
static long task_week(time_t now) { return ((long)now + 3 * 86400) / TASK_WEEK_SECONDS; }
static time_t task_reset(time_t now) { return (time_t)((task_week(now) + 1) * TASK_WEEK_SECONDS - 3 * 86400); }

const char *religion_rank_title(int deity, int rank)
{
  if (!religion_deity_valid(deity) || rank < 1 || rank > 8) return religion_rank_name(rank);
  if (!*rank_titles[deity][rank])
    snprintf(rank_titles[deity][rank], sizeof(rank_titles[deity][rank]), "%s of %s",
             religion_rank_name(rank), deity_list[deity].name);
  return rank_titles[deity][rank];
}

void religion_tasks_boot(void)
{
  MYSQL_RES *res;
  MYSQL_ROW row;
  int deity, rank;
  memset(rank_titles, 0, sizeof(rank_titles));
  if (!estate_init()) return;
  res = estate_select("SELECT deity,rank_num,title FROM religion_rank_names");
  if (!res) return;
  while ((row = mysql_fetch_row(res))) {
    deity = atoi(row[0]); rank = atoi(row[1]);
    if (religion_deity_valid(deity) && rank >= 1 && rank <= 8)
      strlcpy(rank_titles[deity][rank], row[2], sizeof(rank_titles[deity][rank]));
  }
  mysql_free_result(res);
}

static bool task_number(const char *text, int min, int max, int *out)
{
  long n;
  if (!text || !*text || strlen(text) > 9 || strspn(text, "0123456789") != strlen(text)) return false;
  n = strtol(text, NULL, 10);
  if (n < min || n > max) return false;
  *out = (int)n; return true;
}

void religion_rank_admin(struct char_data *ch, const char *argument)
{
  char a[MAX_INPUT_LENGTH], b[MAX_INPUT_LENGTH], *q;
  const char *title;
  int deity, rank;
  if (IS_NPC(ch) || GET_LEVEL(ch) < LVL_BUILDER) return;
  argument = one_argument(argument, a, sizeof(a));
  title = one_argument(argument, b, sizeof(b));
  if (!task_number(a, 1, RELIGION_DEITY_CAPACITY-1, &deity) || !religion_deity_valid(deity) ||
      !task_number(b, 1, 8, &rank) || !*title || strlen(title) > 90 ||
      strspn(title, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ '-") != strlen(title)) {
    send_to_char(ch, "Usage: religionadmin rankname <deity-id> <rank 1..8> <title>\r\n"); return;
  }
  q = estate_quote(title);
  if (!q || !estate_init() || !estate_query("INSERT INTO religion_rank_names(deity,rank_num,title) "
      "VALUES(%d,%d,'%s') ON DUPLICATE KEY UPDATE title=VALUES(title)", deity, rank, q)) {
    free(q); send_to_char(ch, "The rank name could not be saved.\r\n"); return;
  }
  free(q);
  strlcpy(rank_titles[deity][rank], title, sizeof(rank_titles[deity][rank]));
  send_to_char(ch, "Rank name saved: %s. Character titles are unchanged.\r\n", title);
}

/* Church boards deliberately exclude native money/XP/perk/item rewards and
 * chains. Quest objectives and level/prerequisite checks remain native. */
static bool task_template(qst_rnum r)
{
  if (r == NOTHING || r >= total_quests || QST_TYPE(r) < 0 ||
      QST_TYPE(r) >= NUM_AQ_TYPES || QST_TYPE(r) == AQ_GIVE_GOLD ||
      QST_TIME(r) != -1 || QST_QUANTITY(r) < 1 || !QST_NAME(r) || !QST_INFO(r) || !QST_DESC(r) ||
      QST_MINLEVEL(r) < 1 || QST_MAXLEVEL(r) < QST_MINLEVEL(r) || !IS_SET(QST_FLAGS(r), AQ_REPEATABLE) ||
      QST_PREV(r) != NOTHING || QST_NEXT(r) != NOTHING || QST_DIAGN(r) != NOTHING ||
      QST_POINTS(r) || QST_GOLD(r) || QST_EXP(r) || QST_OBJ(r) != NOTHING ||
      QST_FOLLOWER(r) != (int)NOBODY || (QST_RACE(r) > RACE_UNDEFINED && QST_RACE(r) < NUM_EXTENDED_RACES))
    return false;
  return QST_TYPE(r) == AQ_ROOM_FIND || QST_TYPE(r) == AQ_MOB_FIND ||
         QST_TYPE(r) == AQ_MOB_KILL || QST_TYPE(r) == AQ_OBJ_FIND ||
         QST_TYPE(r) == AQ_OBJ_RETURN || QST_TYPE(r) == AQ_ROOM_CLEAR || QST_TYPE(r) == AQ_DIALOGUE;
}

/* A changed objective invalidates quotes/active credit, rather than rewarding
 * a different quest under an old assignment. Text edits are harmless. */
static unsigned long long task_hash(qst_rnum r)
{
  unsigned long long h = 1469598103934665603ULL;
  int v[] = {QST_NUM(r), QST_TYPE(r), QST_TARGET(r), QST_RETURNMOB(r), QST_QUANTITY(r),
    QST_MASTER(r), QST_MINLEVEL(r), QST_MAXLEVEL(r), QST_PREREQ(r), QST_DIPLM(r), QST_INTIM(r), QST_BLUFF(r)};
  size_t i;
  for (i = 0; i < sizeof(v)/sizeof(*v); ++i) { h ^= (unsigned int)v[i]; h *= 1099511628211ULL; }
  return h;
}

void religion_tasks_admin(struct char_data *ch, int temple, int deity, int priest, const char *argument)
{
  char a[MAX_INPUT_LENGTH], b[MAX_INPUT_LENGTH];
  const char *rest;
  int slot, quest;
  qst_rnum r;
  MYSQL_RES *res;
  MYSQL_ROW row;
  if (IS_NPC(ch) || GET_LEVEL(ch) < LVL_BUILDER || IN_ROOM(ch) == NOWHERE ||
      !can_edit_zone(ch, world[IN_ROOM(ch)].zone)) return;
  argument = one_argument(argument, a, sizeof(a));
  rest = one_argument(argument, b, sizeof(b));
  if (!*a) {
    res = estate_select("SELECT slot,quest_vnum,temple_id FROM religion_task_config WHERE deity=%d ORDER BY slot", deity);
    if (!res) { send_to_char(ch, "Task storage is unavailable.\r\n"); return; }
    while ((row = mysql_fetch_row(res))) send_to_char(ch, "Slot %s: quest %s at temple %s\r\n", row[0], row[1], row[2]);
    mysql_free_result(res); return;
  }
  if (!task_number(a, 1, 4, &slot) || !task_number(b, 0, INT_MAX, &quest) || *rest) goto usage;
  r = real_quest(quest);
  if (quest && (priest <= 0 || real_mobile(priest) == NOBODY || !task_template(r) || QST_MASTER(r) != (mob_vnum)priest)) {
    send_to_char(ch, "Use a repeatable, untimed standalone native quest with no economic rewards, "
                     "authored objectives and this temple's priest as questmaster.\r\n"); return;
  }
  if (!estate_init()) { send_to_char(ch, "Task storage is unavailable.\r\n"); return; }
  if (!estate_query("START TRANSACTION")) return;
  if (!estate_query("DELETE FROM religion_task_config WHERE deity=%d AND slot=%d", deity, slot) ||
      (quest && !estate_query("INSERT INTO religion_task_config(temple_id,slot,deity,quest_vnum) VALUES(%d,%d,%d,%d)",
                             temple, slot, deity, quest)) || !estate_query("COMMIT")) {
    estate_query("ROLLBACK");
    send_to_char(ch, "The task slot could not be saved. Each faith may offer a template only once.\r\n"); return;
  }
  send_to_char(ch, "Task configuration saved. Existing weekly boards remain unchanged until Monday 00:00 UTC.\r\n");
  return;
usage:
  send_to_char(ch, "Usage: religionadmin task %d <slot 1..4> <quest-vnum; 0 clears>\r\n", temple);
}

static bool task_storage(struct char_data *ch)
{
  if (!estate_init()) { send_to_char(ch, "Church task storage is unavailable; please try later.\r\n"); return false; }
  return true;
}

/* Never replace other ordinary quest slots. A missing/stale slot is reconstructed
 * from its durable enrollment; ready objectives need no native slot. */
void religion_tasks_restore(struct char_data *ch)
{
  MYSQL_RES *res;
  MYSQL_ROW row;
  int deity, standing, quest, ready;
  qst_rnum r;
  if (!ch || IS_NPC(ch) || !ch->player_specials || !estate_init()) return;
  res = estate_select("SELECT deity,standing FROM religion_standing WHERE player_id=%ld", GET_IDNUM(ch));
  if (!res) return;
  while ((row = mysql_fetch_row(res))) {
    deity = atoi(row[0]); standing = atoi(row[1]);
    if (deity > 0 && deity < RELIGION_DEITY_CAPACITY && standing >= 0 && standing <= 336)
      ch->player_specials->saved.religion_standing[deity] = standing;
  }
  mysql_free_result(res);
  res = estate_select("SELECT quest_vnum,ready FROM religion_task_active WHERE player_id=%ld", GET_IDNUM(ch));
  if (!res) return;
  row = mysql_fetch_row(res);
  if (!row) clear_quest(ch, RELIGION_QUEST_SLOT);
  else {
    quest = atoi(row[0]); ready = atoi(row[1]); r = real_quest(quest);
    if (ready || !task_template(r)) clear_quest(ch, RELIGION_QUEST_SLOT);
    else if (GET_QUEST(ch, RELIGION_QUEST_SLOT) != quest || GET_QUEST_COUNTER(ch, RELIGION_QUEST_SLOT) < 0 ||
             GET_QUEST_COUNTER(ch, RELIGION_QUEST_SLOT) > QST_QUANTITY(r)) set_quest(ch, r, RELIGION_QUEST_SLOT);
  }
  mysql_free_result(res);
}

static bool task_allowance(struct char_data *ch, int deity, int quest, bool *story)
{
  MYSQL_RES *res;
  MYSQL_ROW row;
  int count, duplicate;
  res = estate_select("SELECT COUNT(*),COALESCE(SUM(quest_vnum=%d),0) FROM religion_task_completions "
      "WHERE player_id=%ld AND week_id=%ld AND award>0", quest, GET_IDNUM(ch), task_week(time(NULL)));
  if (!res) return false;
  row = mysql_fetch_row(res);
  if (!row) { mysql_free_result(res); return false; }
  count = atoi(row[0]); duplicate = atoi(row[1]); mysql_free_result(res);
  *story = count >= 3 || duplicate || ch->player_specials->saved.religion_standing[deity] >= 336;
  return true;
}

static bool task_board(int deity)
{
  /* One atomic snapshot: configuration changes cannot reroll an existing week. */
  if (!estate_query("START TRANSACTION")) return false;
  if (!estate_query("INSERT IGNORE INTO religion_task_weeks(deity,week_id) VALUES(%d,%ld)", deity, task_week(time(NULL)))) goto fail;
  if (mysql_affected_rows(conn) && !estate_query("INSERT INTO religion_task_offers(temple_id,week_id,slot,deity,quest_vnum) "
      "SELECT temple_id,%ld,slot,deity,quest_vnum FROM religion_task_config WHERE deity=%d",
      task_week(time(NULL)), deity)) goto fail;
  if (estate_query("COMMIT")) return true;
fail:
  estate_query("ROLLBACK"); return false;
}

void religion_tasks_status(struct char_data *ch)
{
  MYSQL_RES *res;
  MYSQL_ROW row;
  time_t reset = task_reset(time(NULL));
  char stamp[64];
  struct tm utc;
  gmtime_r(&reset, &utc); strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M UTC", &utc);
  send_to_char(ch, "Church tasks reset Monday 00:00 UTC; next reset: %s.\r\n", stamp);
  if (!estate_init()) { send_to_char(ch, "Church task storage is unavailable.\r\n"); return; }
  res = estate_select("SELECT COUNT(*) FROM religion_task_completions WHERE player_id=%ld AND week_id=%ld AND award>0",
                     GET_IDNUM(ch), task_week(time(NULL)));
  if (res) {
    row = mysql_fetch_row(res);
    if (row) send_to_char(ch, "Rewarded church tasks this week: %s/3 (2 Standing each).\r\n", row[0]);
    mysql_free_result(res);
  }
  res = estate_select("SELECT quest_vnum,ready,story_only FROM religion_task_active WHERE player_id=%ld", GET_IDNUM(ch));
  if (!res) return;
  row = mysql_fetch_row(res);
  if (row) {
    qst_rnum r = real_quest(atoi(row[0]));
    send_to_char(ch, "Active church task: %s; %s%s.\r\n", r == NOTHING ? "an unavailable task" : QST_NAME(r),
                 atoi(row[1]) ? "return to the assigned priest" : "in progress",
                 atoi(row[2]) ? "; story-only, no Standing" : "");
  } else send_to_char(ch, "You have no active church task.\r\n");
  mysql_free_result(res);
}

void religion_task_objective_complete(struct char_data *ch)
{
  MYSQL_RES *res;
  MYSQL_ROW row;
  int quest = GET_QUEST(ch, RELIGION_QUEST_SLOT);
  qst_rnum r = real_quest(quest);
  unsigned long long expected;
  if (!task_template(r) || GET_QUEST_COUNTER(ch, RELIGION_QUEST_SLOT) > 0 || !task_storage(ch)) return;
  res = estate_select("SELECT objective_hash FROM religion_task_active WHERE player_id=%ld AND quest_vnum=%d",
                      GET_IDNUM(ch), quest);
  if (!res) return;
  row = mysql_fetch_row(res); expected = row ? strtoull(row[0], NULL, 10) : 0;
  mysql_free_result(res);
  if (expected != task_hash(r)) { send_to_char(ch, "This task's objective has changed. Ask a builder or abandon it.\r\n"); return; }
  if (!estate_query("UPDATE religion_task_active SET ready=1 WHERE player_id=%ld AND quest_vnum=%d", GET_IDNUM(ch), quest)) return;
  clear_quest(ch, RELIGION_QUEST_SLOT);
  save_char_checked(ch, 0);
  send_to_char(ch, "Your church task objective is complete. Return to its priest and use religion task turnin.\r\n");
}

static void task_turnin(struct char_data *ch)
{
  MYSQL_RES *res;
  MYSQL_ROW row;
  unsigned long long enrollment;
  int temple, deity, priest, quest, story, ready, standing, award, context, context_deity;
  bool limited;
  if (GET_QUEST(ch, RELIGION_QUEST_SLOT) != (int)NOTHING && GET_QUEST_COUNTER(ch, RELIGION_QUEST_SLOT) <= 0)
    religion_task_objective_complete(ch);
  if (!estate_query("START TRANSACTION")) goto unavailable;
  res = estate_select("SELECT enrollment,temple_id,deity,quest_vnum,story_only,ready FROM religion_task_active "
                      "WHERE player_id=%ld FOR UPDATE", GET_IDNUM(ch));
  if (!res) goto fail;
  row = mysql_fetch_row(res);
  if (!row) { mysql_free_result(res); estate_query("ROLLBACK"); send_to_char(ch, "You have no active church task.\r\n"); return; }
  enrollment = strtoull(row[0], NULL, 10); temple = atoi(row[1]); deity = atoi(row[2]);
  quest = atoi(row[3]); story = atoi(row[4]); ready = atoi(row[5]); mysql_free_result(res);
  if (!ready || deity != GET_DEITY(ch) || !religion_service_context(ch, temple, &context, &context_deity, &priest) || context_deity != deity) {
    estate_query("ROLLBACK"); send_to_char(ch, "Complete the objective and return to its temple as a member of that faith.\r\n"); return;
  }
  res = estate_select("SELECT standing FROM religion_standing WHERE player_id=%ld AND deity=%d FOR UPDATE", GET_IDNUM(ch), deity);
  if (!res) goto fail;
  row = mysql_fetch_row(res);
  if (!row) { mysql_free_result(res); goto fail; }
  standing = atoi(row[0]); mysql_free_result(res);
  if (standing < 0 || standing > 336 || !task_allowance(ch, deity, quest, &limited)) goto fail;
  award = (!story && !limited && standing < 336) ? MIN(2, 336-standing) : 0;
  if (!estate_query("INSERT INTO religion_task_completions(enrollment,player_id,deity,quest_vnum,week_id,award) "
      "VALUES(%llu,%ld,%d,%d,%ld,%d)", enrollment, GET_IDNUM(ch), deity, quest, task_week(time(NULL)), award) ||
      !estate_query("UPDATE religion_standing SET standing=standing+%d WHERE player_id=%ld AND deity=%d", award, GET_IDNUM(ch), deity) ||
      !estate_query("DELETE FROM religion_task_active WHERE player_id=%ld AND enrollment=%llu", GET_IDNUM(ch), enrollment) ||
      !estate_query("COMMIT")) goto fail;
  ch->player_specials->saved.religion_standing[deity] = standing + award;
  clear_quest(ch, RELIGION_QUEST_SLOT);
  save_char_checked(ch, 0);
  send_to_char(ch, "Church service completed: %d Standing. Current rank: %s.\r\n", award, religion_rank_title(deity, religion_rank(ch)));
  return;
fail:
  estate_query("ROLLBACK");
unavailable:
  send_to_char(ch, "The completion could not be confirmed. Please check your task status before trying again.\r\n");
}

void religion_tasks_command(struct char_data *ch, const char *argument)
{
  char verb[MAX_INPUT_LENGTH], text[MAX_INPUT_LENGTH], *reason = NULL;
  const char *rest;
  int temple, deity, priest, slot, quest, i;
  int *preview = ch->player_specials->religion_task_preview;
  qst_rnum r;
  bool story;
  MYSQL_RES *res;
  MYSQL_ROW row;
  unsigned long long fingerprint;
  if (IS_NPC(ch) || !task_storage(ch)) return;
  religion_tasks_restore(ch);
  rest = one_argument(argument, verb, sizeof(verb));
  if (!strcmp(verb, "abandon")) {
    if (!estate_query("DELETE FROM religion_task_active WHERE player_id=%ld", GET_IDNUM(ch))) return;
    clear_quest(ch, RELIGION_QUEST_SLOT); ch->player_specials->religion_task_preview_until = 0;
    save_char_checked(ch, 0);
    send_to_char(ch, "Church task abandoned. Weekly rewards and offerings are unchanged.\r\n"); return;
  }
  if (!strcmp(verb, "cancel")) { ch->player_specials->religion_task_preview_until = 0; send_to_char(ch, "Task preview cancelled.\r\n"); return; }
  if (!strcmp(verb, "status")) { religion_tasks_status(ch); return; }
  if (!strcmp(verb, "turnin")) { task_turnin(ch); return; }
  if (!religion_service_context(ch, 0, &temple, &deity, &priest)) return;
  if (!task_board(deity)) {
    send_to_char(ch, "The weekly service board is unavailable. Please try again later.\r\n"); return;
  }
  if (!*verb || !strcmp(verb, "list")) {
    religion_tasks_status(ch);
    res = estate_select("SELECT slot,quest_vnum FROM religion_task_offers WHERE temple_id=%d AND week_id=%ld AND deity=%d ORDER BY slot",
                        temple, task_week(time(NULL)), deity);
    if (!res) return;
    i = 0;
    while ((row = mysql_fetch_row(res))) {
      quest = atoi(row[1]); r = real_quest(quest);
      if (!task_template(r) || QST_MASTER(r) != (mob_vnum)priest || !task_allowance(ch, deity, quest, &story)) continue;
      send_to_char(ch, "%s) %s -- %s\r\n", row[0], QST_NAME(r), story ? "story-only; no Standing" : "2 Standing"); ++i;
    }
    mysql_free_result(res);
    if (!i) send_to_char(ch, "No authored tasks are available on this week's board.\r\n");
    send_to_char(ch, "religion task accept <slot> | confirm | cancel | status | turnin | abandon\r\n"); return;
  }
  if (!strcmp(verb, "confirm")) {
    if (!ch->player_specials->religion_task_preview_until || time(NULL) > ch->player_specials->religion_task_preview_until ||
        preview[0] != temple || preview[3] != deity || preview[4] != task_week(time(NULL))) goto stale;
    slot = preview[1]; quest = preview[2]; fingerprint = ch->player_specials->religion_task_preview_hash;
  } else if (!strcmp(verb, "accept")) {
    rest = one_argument(rest, text, sizeof(text));
    if (*rest || !task_number(text, 1, 4, &slot)) goto usage;
    quest = 0; fingerprint = 0;
    ch->player_specials->religion_task_preview_until = 0;
  } else goto usage;
  res = estate_select("SELECT quest_vnum FROM religion_task_offers WHERE temple_id=%d AND week_id=%ld AND slot=%d AND deity=%d",
                      temple, task_week(time(NULL)), slot, deity);
  if (!res) return;
  row = mysql_fetch_row(res); i = row ? atoi(row[0]) : 0; mysql_free_result(res);
  if (!i || (quest && quest != i)) goto stale;
  quest = i; r = real_quest(quest);
  if (!task_template(r) || QST_MASTER(r) != (mob_vnum)priest || (fingerprint && task_hash(r) != fingerprint)) goto stale;
  res = estate_select("SELECT 1 FROM religion_task_active WHERE player_id=%ld", GET_IDNUM(ch));
  if (!res) return;
  i = mysql_num_rows(res); mysql_free_result(res);
  if (i) { send_to_char(ch, "Complete or abandon your existing church task first.\r\n"); return; }
  if (!quest_church_eligible(ch, r, &reason)) {
    send_to_char(ch, "%s", reason ? reason : "You do not meet this quest's requirements.\r\n"); free(reason); return;
  }
  if (!task_allowance(ch, deity, quest, &story)) return;
  if (!strcmp(verb, "accept")) {
    preview[0]=temple; preview[1]=slot; preview[2]=quest; preview[3]=deity;
    preview[4]=task_week(time(NULL)); preview[5]=story;
    ch->player_specials->religion_task_preview_hash=task_hash(r);
    ch->player_specials->religion_task_preview_until=time(NULL)+TASK_PREVIEW_SECONDS;
    send_to_char(ch, "%s\r\n%s\r\nObjective: %s. Complete %d objective(s), then return to %s at this temple.\r\n"
                     "Level requirement: %d..%d. Prerequisite item: %s. Reward: %s.\r\n"
                     "Use religion task confirm within two minutes.\r\n", QST_NAME(r), QST_INFO(r),
                     QST_DESC(r), QST_QUANTITY(r), mob_proto[real_mobile(priest)].player.short_descr,
                     QST_MINLEVEL(r), QST_MAXLEVEL(r), QST_PREREQ(r) == NOTHING ? "none" : "required (see the quest instructions)", story ? "story-only, no Standing now or later" : "2 Standing (weekly cap applies)");
    return;
  }
  if (story != !!preview[5]) goto stale;
  if (!estate_query("START TRANSACTION")) return;
  if (!estate_query("INSERT IGNORE INTO religion_standing(player_id,deity,standing) VALUES(%ld,%d,%d)",
      GET_IDNUM(ch), deity, ch->player_specials->saved.religion_standing[deity]) ||
      !estate_query("INSERT INTO religion_task_active(player_id,temple_id,deity,quest_vnum,story_only,objective_hash) "
      "VALUES(%ld,%d,%d,%d,%d,%llu)", GET_IDNUM(ch), temple, deity, quest, story, task_hash(r)) || !estate_query("COMMIT")) {
    estate_query("ROLLBACK"); send_to_char(ch, "The task could not be confirmed. Check status before trying again.\r\n"); return;
  }
  ch->player_specials->religion_task_preview_until=0;
  set_quest(ch, r, RELIGION_QUEST_SLOT); save_char_checked(ch, 0);
  send_to_char(ch, "Church task accepted: %s.\r\n", QST_NAME(r)); return;
stale:
  ch->player_specials->religion_task_preview_until=0;
  send_to_char(ch, "That offering or preview is no longer valid. Request a fresh task preview.\r\n"); return;
usage:
  send_to_char(ch, "religion task list | accept <slot> | confirm | cancel | status | turnin | abandon\r\n");
}
