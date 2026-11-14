/* Exercise the real I3 initialization and heartbeat paths without a game server. */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "systems/intermud3/i3_client.h"
#include <assert.h>
#include <pthread.h>

struct descriptor_data *descriptor_list = NULL;
struct player_special_data dummy_mob;

void basic_mud_log(const char *format, ...) { (void)format; }
size_t send_to_char(struct char_data *ch, const char *message, ...)
{
  (void)ch; (void)message; abort();
}
struct char_data *get_player_vis(struct char_data *ch, char *name, int *number, int inroom)
{
  (void)ch; (void)name; (void)number; (void)inroom; abort();
}

int main(void)
{
  int i;
  i3_client_t client;
  pthread_mutex_t mutex;
  i3_event_t first, second;

  /* Missing config leaves I3 disabled; the heartbeat must remain safe. */
  assert(i3_initialize() == 0);
  assert(i3_client == NULL);
  for (i = 0; i < 1000; ++i)
  {
    i3_process_events();
    assert(i3_pop_event() == NULL);
  }
  assert(i3_connect() == -1);
  assert(i3_send_json(NULL) == -1);
  assert(i3_load_config("i3_config") == -1);
  assert(i3_save_config("must-not-be-written") == -1);
  i3_disconnect();
  i3_shutdown();

  /* The guard must preserve FIFO handling when the client is initialized. */
  memset(&client, 0, sizeof(client));
  memset(&first, 0, sizeof(first));
  memset(&second, 0, sizeof(second));
  assert(pthread_mutex_init(&mutex, NULL) == 0);
  client.event_mutex = &mutex;
  client.event_queue_head = &first;
  client.event_queue_tail = &second;
  client.event_queue_size = 2;
  first.next = &second;
  i3_client = &client;
  assert(i3_pop_event() == &first);
  assert(client.event_queue_size == 1);
  assert(i3_pop_event() == &second);
  assert(client.event_queue_head == NULL);
  assert(client.event_queue_tail == NULL);
  assert(client.event_queue_size == 0);
  i3_process_events();
  i3_client = NULL;
  assert(pthread_mutex_destroy(&mutex) == 0);
  return 0;
}
