/* Dynamic room reallocations must not leave paused DG scripts pointing at freed rooms. */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "dg_scripts.h"
#include "dg_event.h"
#include "genwld.h"
#include <assert.h>
struct room_data *world;
room_rnum top_of_world;
extern int runtime_room_mutation;
room_rnum add_room(struct room_data *room)
{
  struct room_data *old = world;
  assert(runtime_room_mutation);
  world = calloc(top_of_world + 2, sizeof(*world));
  memcpy(world, old, (top_of_world + 1) * sizeof(*world));
  world[++top_of_world] = *room;
  free(old);
  return top_of_world;
}
int delete_room(room_rnum room)
{
  struct room_data *old = world;
  assert(runtime_room_mutation && room == top_of_world);
  world = calloc(top_of_world, sizeof(*world));
  memcpy(world, old, top_of_world * sizeof(*world));
  top_of_world--;
  free(old);
  return TRUE;
}
int main(void)
{
  struct script_data script = {0};
  struct trig_data trigger = {0};
  struct event event = {0};
  struct wait_event_data wait = {0};
  struct room_data new_room = {0};
  top_of_world = 1;
  world = calloc(2, sizeof(*world));
  SCRIPT(&world[0]) = &script;
  TRIGGERS(&script) = &trigger;
  GET_TRIG_WAIT(&trigger) = &event;
  event.event_obj = &wait;
  wait.go = &world[0];
  new_room.number = 2000000;
  assert(add_runtime_room(&new_room) == 2);
  assert(wait.go == &world[0] && !runtime_room_mutation);
  assert(delete_runtime_room(2));
  assert(wait.go == &world[0] && !runtime_room_mutation && top_of_world == 1);
  free(world);
  puts("PASS: runtime housing allocation/release rebind paused DG room scripts");
  return 0;
}
