/**************************************************************************
 *  File: asciimap.c                                   Part of LuminariMUD *
 *  Usage: Generates an ASCII map of the player's surroundings.            *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "house.h"
#include "constants.h"
#include "dg_scripts.h"
#include "asciimap.h"
#include "wilderness.h"
#include "protocol.h"
#include "modify.h" /* strip_colors */

#include <stdint.h>

/******************************************************************************
 * Begin Local (File Scope) Defines and Global Variables
 *****************************************************************************/
/* Do not blindly change these values, as many values cause the map to stop working - backup first */
#define CANVAS_HEIGHT 19
#define CANVAS_WIDTH 51
#define LEGEND_WIDTH 15

#define DEFAULT_MAP_SIZE CONFIG_MAP_SIZE

#define MAX_MAP_SIZE (CANVAS_WIDTH - 1) / 4
#define MAX_MAP CANVAS_WIDTH

#define MAX_MAP_DIR 10
#define MAX_MAP_FOLLOW 10

/* anything greater than num sect types */
#define SECT_EMPTY (NUM_ROOM_SECTORS + 1)
#define SECT_STRANGE (SECT_EMPTY + 1)
#define SECT_HERE (SECT_STRANGE + 1)

#define DOOR_NS -1
#define DOOR_EW -2
#define DOOR_UP -3
#define DOOR_DOWN -4
#define DOOR_DIAGNE -5
#define DOOR_DIAGNW -6
#define VDOOR_NS -7
#define VDOOR_EW -8
#define VDOOR_DIAGNE -9
#define VDOOR_DIAGNW -10
#define DOOR_UP_AND_NE -11
#define DOOR_DOWN_AND_SE -12
#define DOOR_NONE -13
#define NUM_DOOR_TYPES 13

#define MAP_CIRCLE 0
#define MAP_RECTANGLE 1

#define MAP_NORMAL 0
#define MAP_COMPACT 1

struct map_info_type
{
  int sector_type;
  char disp[20];
};

static struct map_info_type door_info[] = {{DOOR_NONE, "   "},
                                           {DOOR_DOWN_AND_SE, "\tr-\tn\\ "},
                                           {DOOR_UP_AND_NE, "\tr+\tn/ "},
                                           {VDOOR_DIAGNW, " \tm+\tn "},
                                           {VDOOR_DIAGNE, " \tm+\tn "},
                                           {VDOOR_EW, " \tm+\tn "},
                                           {VDOOR_NS, " \tm+\tn "},
                                           {DOOR_DIAGNW, " \\ "},
                                           {DOOR_DIAGNE, " / "},
                                           {DOOR_DOWN, "\tr-\tn  "},
                                           {DOOR_UP, "\tr+\tn  "},
                                           {DOOR_EW, " - "},
                                           {DOOR_NS, " | "}};

static struct map_info_type compact_door_info[] = {{DOOR_NONE, " "},
                                                   {DOOR_DOWN_AND_SE, "\tR\\\tn"},
                                                   {DOOR_UP_AND_NE, "\tR/\tn"},
                                                   {VDOOR_DIAGNW, "\tm+\tn"},
                                                   {VDOOR_DIAGNE, "\tm+\tn"},
                                                   {VDOOR_EW, " \tm+\tn "},
                                                   {VDOOR_NS, " \tm+\tn "},
                                                   {DOOR_DIAGNW, "\\"},
                                                   {DOOR_DIAGNE, "/"},
                                                   {DOOR_DOWN, "\tr-\tn"},
                                                   {DOOR_UP, "\tr+\tn"},
                                                   {DOOR_EW, "-"},
                                                   {DOOR_NS, " | "}};

/* Add new sector types below for both map_info and world_map_info     */
/* The last 3 MUST remain the same, although the symbol can be changed */
/* New sectors also need to be added to the perform_map function below */
static struct map_info_type map_info[] = {
    {SECT_INSIDE, "\tc[\tn.\tc]\tn"}, /* 0 */
    {SECT_CITY, "\tc[\twC\tc]\tn"},
    {SECT_FIELD, "\tc[\tg,\tc]\tn"},
    {SECT_FOREST, "\tc[\tG\t=Y\tn\tc]\tn"},
    {SECT_HILLS, "\tc[\ty^\tc]\tn"},
    {SECT_MOUNTAIN, "\tc[\trm\tc]\tn"}, /* 5 */
    {SECT_WATER_SWIM, "\tc[\tc\t=~\tn\tc]\tn"},
    {SECT_WATER_NOSWIM, "\tc[\tb\t==\tn\tc]\tn"},
    {SECT_FLYING, "\tc[\tC^\tc]\tn"},
    {SECT_UNDERWATER, "\tc[\tbU\tc]\tn"},
    {SECT_ZONE_START, "\tc[\tRX\tc]\tn"},    /* 10 */
    {SECT_ROAD_NS, "\tc[\tD|\tc]\tn"},       /* 11 */
    {SECT_ROAD_EW, "\tc[\tD-\tc]\tn"},       /* 12 */
    {SECT_ROAD_INT, "\tc[\tD+\tc]\tn"},      /* 13 */
    {SECT_DESERT, "\tc[\tY.\tc]\tn"},        // 14
    {SECT_OCEAN, "\tc[\tB\t=o\tn\tc]\tn"},   // 15
    {SECT_MARSHLAND, "\tc[\tM,\tc]\tn"},     // 16
    {SECT_HIGH_MOUNTAIN, "\tc[\tRM\tc]\tn"}, // 17
    {SECT_PLANES, "\tc[\tD.\tc]\tn"},        // 18
    {SECT_UD_WILD, "\tc[\tM\t=Y\tn\tc]\tn"},
    {SECT_UD_CITY, "\tc[\tmC\tc]\tn"}, // 20
    {SECT_UD_INSIDE, "\tc[\tm.\tc]\tn"},
    {SECT_UD_WATER, "\tc[\tm\t=~\tn\tc]\tn"},
    {SECT_UD_NOSWIM, "\tc[\tM\t==\tn\tc]\tn"},
    {SECT_UD_NOGROUND, "\tc[\tm^\tc]\tn"},
    {SECT_LAVA, "\tc[\tR.\tc]\tn"}, // 25
    {SECT_D_ROAD_NS, "\tc[\ty|\tc]\tn"},
    {SECT_D_ROAD_EW, "\tc[\ty-\tc]\tn"},
    {SECT_D_ROAD_INT, "\tc[\ty+\tc]\tn"},
    {SECT_CAVE, "\tc[\tMC\tc]\tn"},
    {SECT_JUNGLE, "\tc[\tg&\tc]\tn"},
    {SECT_TUNDRA, "\tc[\tW.\tc]\tn"},
    {SECT_TAIGA, "\tc[\tgA\tc]\tn"},
    {SECT_BEACH, "\tc[\ty:\tc]\tn"},
    {SECT_SEAPORT, "\tc[\tRS\tc]\tn"},     /* 34 */
    {SECT_INSIDE_ROOM, "\tc[\ty*\tc]\tn"}, /* 35 */
    {SECT_RIVER, "\tc[\tc\t=~\tn\tc]\tn"}, // 36

    {-1, ""},            /* RESERVED, NUM_ROOM_SECTORS */
    {SECT_EMPTY, "   "}, /* NUM_ROOM_SECTORS + 1 */
    {SECT_STRANGE, "\tc[\tR?\tc]\tn"},
    {SECT_HERE, "\tc[\tW&\tc]\tn"},
};

static struct map_info_type world_map_info[] = {
    {SECT_INSIDE, "\tn.\tn"}, /* 0 */
    {SECT_CITY, "\twC\tn"},
    {SECT_FIELD, "\tg,\tn"},
    {SECT_FOREST, "\tG\t=Y\tn"},
    {SECT_HILLS, "\ty^\tn"},
    {SECT_MOUNTAIN, "\trm\tn"}, /* 5 */
    {SECT_WATER_SWIM, "\tc\t=~\tn"},
    {SECT_WATER_NOSWIM, "\tb\t==\tn"},
    {SECT_FLYING, "\tC^\tn"},
    {SECT_UNDERWATER, "\tbU\tn"},
    {SECT_ZONE_START, "\tRX\tn"},    /* 10 */
    {SECT_ROAD_NS, "\tD|\tn"},       /* 11 */
    {SECT_ROAD_EW, "\tD-\tn"},       /* 12 */
    {SECT_ROAD_INT, "\tD+\tn"},      /* 13 */
    {SECT_DESERT, "\tY.\tn"},        /* 14 */
    {SECT_OCEAN, "\tB\t=o\tn"},      /* 15 */
    {SECT_MARSHLAND, "\tM,\tn"},     /* 16 */
    {SECT_HIGH_MOUNTAIN, "\tRM\tn"}, /* 17 */
    {SECT_PLANES, "\tM.\tn"},        /* 18 */
    {SECT_UD_WILD, "\tM\t=Y\tn"},
    {SECT_UD_CITY, "\tmC\tn"}, // 20
    {SECT_UD_INSIDE, "\tm.\tn"},
    {SECT_UD_WATER, "\tm\t=~\tn\tn"},
    {SECT_UD_NOSWIM, "\tM\t==\tn\tn"},
    {SECT_UD_NOGROUND, "\tm^\tn"},
    {SECT_LAVA, "\tR.\tc]\tn"}, // 25
    {SECT_D_ROAD_NS, "\ty|\tn"},
    {SECT_D_ROAD_EW, "\ty-\tn"},
    {SECT_D_ROAD_INT, "\ty+\tn"},
    {SECT_CAVE, "\tMC\tn"},
    {SECT_JUNGLE, "\tg&\tn"},
    {SECT_TUNDRA, "\tW.\tn"},
    {SECT_TAIGA, "\tgA\tn"},
    {SECT_BEACH, "\ty:\tn"},
    {SECT_SEAPORT, "\tRS\tn"},     /* 34 */
    {SECT_INSIDE_ROOM, "\ty*\tn"}, /* 35 */
    {SECT_RIVER, "\tc\t=~\tn"},    // 36

    {-1, ""}, /* RESERVED, NUM_ROOM_SECTORS */
    {SECT_EMPTY, " "},
    {SECT_STRANGE, "\tR?\tn"},
    {SECT_HERE, "\tW&\tn"},
};

static int map[MAX_MAP][MAX_MAP];
/*
static int offsets[4][2] ={ {-2, 0},{ 0, 2},{ 2, 0},{ 0, -2} };
static int offsets_worldmap[4][2] ={ {-1, 0},{ 0, 1},{ 1, 0},{ 0, -1} };
static int door_offsets[6][2] ={ {-1, 0},{ 0, 1},{ 1, 0},{ 0, -1},{ -1, 1},{ 1, 1} };
static int door_marks[6] = { DOOR_NS, DOOR_EW, DOOR_NS, DOOR_EW, DOOR_UP, DOOR_DOWN };
 */
static int offsets[10][2] = {{-2, 0}, {0, 2},   {2, 0},  {0, -2}, {0, 0},
                             {0, 0},  {-2, -2}, {-2, 2}, {2, 2},  {2, -2}};
static int offsets_worldmap[10][2] = {{-1, 0}, {0, 1},   {1, 0},  {0, -1}, {0, 0},
                                      {0, 0},  {-1, -1}, {-1, 1}, {1, 1},  {1, -1}};
static int door_offsets[10][2] = {{-1, 0}, {0, 1},   {1, 0},  {0, -1}, {-1, 1},
                                  {1, 1},  {-1, -1}, {-1, 1}, {1, 1},  {1, -1}};
static int door_marks[10] = {DOOR_NS,   DOOR_EW,     DOOR_NS,     DOOR_EW,     DOOR_UP,
                             DOOR_DOWN, DOOR_DIAGNW, DOOR_DIAGNE, DOOR_DIAGNW, DOOR_DIAGNE};
static int vdoor_marks[4] = {VDOOR_NS, VDOOR_EW, VDOOR_NS, VDOOR_EW};
/******************************************************************************
 * End Local (File Scope) Defines and Global Variables
 *****************************************************************************/

/******************************************************************************
 * Begin Local (File Scope) Function Prototypes
 *****************************************************************************/
static void MapArea(room_rnum room, struct char_data *ch, int x, int y, int min, int max,
                    sh_int xpos, sh_int ypos, bool worldmap);
static char *StringMap(int centre, int size);
static char *WorldMap(int centre, int size, int mapshape, int maptype);
static const char *CompactStringMap(int centre, int size);

/******************************************************************************
 * End Local (File Scope) Function Prototypes
 *****************************************************************************/

bool can_see_map(struct char_data *ch)
{
  /* Is the map funcionality disabled? */
  if (CONFIG_MAP == MAP_OFF)
    return FALSE;
  else if ((CONFIG_MAP == MAP_IMM_ONLY) && (GET_LEVEL(ch) < LVL_IMMORT))
    return FALSE;

  if (IN_ROOM(ch) != NOWHERE && GET_LEVEL(ch) < LVL_IMMORT &&
      ROOM_FLAGGED(IN_ROOM(ch), ROOM_FOG))
    return FALSE;

  return TRUE;
}

/* MapArea function - create the actual map */
static void MapArea(room_rnum room, struct char_data *ch, int x, int y, int min, int max,
                    sh_int xpos, sh_int ypos, bool worldmap)
{
  room_rnum prospect_room;
  struct room_direction_data *pexit;
  int door, ew_size = 0, ns_size = 0, x_exit_pos = 0, y_exit_pos = 0;
  sh_int prospect_xpos, prospect_ypos;

  if (map[x][y] < 0)
    return; /* this is a door */

  /* marks the room as visited */
  if (room == IN_ROOM(ch))
    map[x][y] = SECT_HERE;
  else
    map[x][y] = SECT(room);

  if ((x < min) || (y < min) || (x > max) || (y > max))
    return;

  /* Fog hides the exits beyond this room from mortal map viewers. */
  if (GET_LEVEL(ch) < LVL_IMMORT && ROOM_FLAGGED(room, ROOM_FOG))
    return;

  /* Check for exits */
  for (door = 0; door < MAX_MAP_DIR; door++)
  {
    if (door < MAX_MAP_FOLLOW && xpos + door_offsets[door][0] >= 0 &&
        xpos + door_offsets[door][0] <= ns_size && ypos + door_offsets[door][1] >= 0 &&
        ypos + door_offsets[door][1] <= ew_size)
    { /* Virtual exit */

      map[x + door_offsets[door][0]][y + door_offsets[door][1]] = vdoor_marks[door];
      if (map[x + offsets[door][0]][y + offsets[door][1]] == SECT_EMPTY)
        MapArea(room, ch, x + offsets[door][0], y + offsets[door][1], min, max,
                xpos + door_offsets[door][0], ypos + door_offsets[door][1], worldmap);
      continue;
    }

    if ((pexit = world[room].dir_option[door]) != NULL && (pexit->to_room > 0) &&
        (pexit->to_room != NOWHERE) && (!IS_SET(pexit->exit_info, EX_CLOSED)) &&
        (!IS_SET(pexit->exit_info, EX_HIDDEN) || PRF_FLAGGED(ch, PRF_HOLYLIGHT)))
    { /* A real exit */

      /* But is the door here... */
      switch (door)
      {
      case NORTH:
        if (xpos > 0 || ypos != y_exit_pos)
          continue;
        break;
      case SOUTH:
        if (xpos < ns_size || ypos != y_exit_pos)
          continue;
        break;
      case EAST:
        if (ypos < ew_size || xpos != x_exit_pos)
          continue;
        break;
      case WEST:
        if (ypos > 0 || xpos != x_exit_pos)
          continue;
        break;
      case NORTHWEST:
        if (xpos > 0 || ypos != y_exit_pos || ypos > 0 || xpos != x_exit_pos)
          continue;
        break;
      case NORTHEAST:
        if (xpos > 0 || ypos != y_exit_pos || ypos < ew_size || xpos != x_exit_pos)
          continue;
        break;
      case SOUTHEAST:
        if (xpos < ns_size || ypos != y_exit_pos || ypos < ew_size || xpos != x_exit_pos)
          continue;
        break;
      case SOUTHWEST:
        if (xpos < ns_size || ypos != y_exit_pos || ypos > 0 || xpos != x_exit_pos)
          continue;
        break;
      }

      /*     if ( (x < min) || ( y < min) || ( x > max ) || ( y > max) ) return;*/
      prospect_room = pexit->to_room;

      /* one way into area OR maze */
      if (world[prospect_room].dir_option[rev_dir[door]] &&
          world[prospect_room].dir_option[rev_dir[door]]->to_room != room)
      {
        map[x][y] = SECT_STRANGE;
        return;
      }

      if (!worldmap)
      {
        if ((map[x + door_offsets[door][0]][y + door_offsets[door][1]] == DOOR_NONE) ||
            (map[x + door_offsets[door][0]][y + door_offsets[door][1]] == SECT_EMPTY))
        {
          map[x + door_offsets[door][0]][y + door_offsets[door][1]] = door_marks[door];
        }
        else
        {
          if (((door == NORTHEAST) &&
               (map[x + door_offsets[door][0]][y + door_offsets[door][1]] == DOOR_UP)) ||
              ((door == UP) &&
               (map[x + door_offsets[door][0]][y + door_offsets[door][1]] == DOOR_DIAGNE)))
          {
            map[x + door_offsets[door][0]][y + door_offsets[door][1]] = DOOR_UP_AND_NE;
          }
          else if (((door == SOUTHEAST) &&
                    (map[x + door_offsets[door][0]][y + door_offsets[door][1]] == DOOR_DOWN)) ||
                   ((door == DOWN) &&
                    (map[x + door_offsets[door][0]][y + door_offsets[door][1]] == DOOR_DIAGNW)))
          {
            map[x + door_offsets[door][0]][y + door_offsets[door][1]] = DOOR_DOWN_AND_SE;
          }
        }
      }

      prospect_xpos = prospect_ypos = 0;
      switch (door)
      {
      case NORTH:
        prospect_xpos = ns_size;
      case SOUTH:
        prospect_ypos = world[prospect_room].dir_option[rev_dir[door]] ? y_exit_pos : ew_size / 2;
        break;
      case WEST:
        prospect_ypos = ew_size;
      case EAST:
        prospect_xpos = world[prospect_room].dir_option[rev_dir[door]] ? x_exit_pos : ns_size / 2;
        break;
      case NORTHEAST:
      case NORTHWEST:
      case SOUTHEAST:
      case SOUTHWEST:
        prospect_xpos = world[prospect_room].dir_option[rev_dir[door]] ? x_exit_pos : ns_size / 2;
        prospect_ypos = world[prospect_room].dir_option[rev_dir[door]] ? y_exit_pos : ew_size / 2;
        break;
      }

      if (worldmap)
      {
        if (door < MAX_MAP_FOLLOW &&
            map[x + offsets_worldmap[door][0]][y + offsets_worldmap[door][1]] == SECT_EMPTY)
          MapArea(pexit->to_room, ch, x + offsets_worldmap[door][0], y + offsets_worldmap[door][1],
                  min, max, prospect_xpos, prospect_ypos, worldmap);
      }
      else
      {
        if (door < MAX_MAP_FOLLOW && map[x + offsets[door][0]][y + offsets[door][1]] == SECT_EMPTY)
          MapArea(pexit->to_room, ch, x + offsets[door][0], y + offsets[door][1], min, max,
                  prospect_xpos, prospect_ypos, worldmap);
      }
    } /* end if exit there */
  }
  return;
}

/* Returns a string representation of the map */
static char *StringMap(int centre, int size)
{
  static char strmap[MAX_MAP * MAX_MAP * 11 + MAX_MAP * 2 + 1];
  char *mp = strmap;
  char *tmp;
  int x, y;

  /* every row */
  for (x = centre - CANVAS_HEIGHT / 2; x <= centre + CANVAS_HEIGHT / 2; x++)
  {
    /* every column */
    for (y = centre - CANVAS_WIDTH / 6; y <= centre + CANVAS_WIDTH / 6; y++)
    {
      if (abs(centre - x) <= size && abs(centre - y) <= size)
        tmp =
            (map[x][y] < 0) ? door_info[NUM_DOOR_TYPES + map[x][y]].disp : map_info[map[x][y]].disp;
      else
        tmp = map_info[SECT_EMPTY].disp;
      strcpy(mp, tmp);
      mp += strlen(tmp);
    }
    strcpy(mp, "\r\n");
    mp += 2;
  }
  *mp = '\0';
  return strmap;
}

/* public function to return a string representation of the map. */
const char *get_map_string(struct char_data *ch, room_vnum target_room)
{
  int size, centre, x, y, min, max;
  int ew_size = 0, ns_size = 0;
  bool worldmap;

  if (!can_see_map(ch) ||
      (GET_LEVEL(ch) < LVL_IMMORT && ROOM_FLAGGED(target_room, ROOM_FOG)))
    return "";

  worldmap = show_worldmap(ch);

  size = CONFIG_MINIMAP_SIZE;
  centre = MAX_MAP / 2;
  min = centre - 2 * size;
  max = centre + 2 * size;

  for (x = 0; x < MAX_MAP; ++x)
    for (y = 0; y < MAX_MAP; ++y)
      map[x][y] = (!(y % 2) && !worldmap) ? DOOR_NONE : SECT_EMPTY;

  /* starts the mapping with the center room */
  MapArea(target_room, ch, centre, centre, min, max, ns_size / 2, ew_size / 2, worldmap);
  map[centre][centre] = SECT_HERE;
  return CompactStringMap(centre, size);
}

static char *WorldMap(int centre, int size, int mapshape, int maptype)
{
  static char strmap[MAX_MAP * MAX_MAP * 4 + MAX_MAP * 2 + 1];
  char *mp = strmap;
  int x, y;
  int xmin, xmax, ymin, ymax;

  switch (maptype)
  {
  case MAP_COMPACT:
    xmin = centre - size;
    xmax = centre + size;
    ymin = centre - 2 * size;
    ymax = centre + 2 * size;
    break;
  default:
    xmin = centre - CANVAS_HEIGHT / 2;
    xmax = centre + CANVAS_HEIGHT / 2;
    ymin = centre - CANVAS_WIDTH / 2;
    ymax = centre + CANVAS_WIDTH / 2;
  }

  /* every row */
  /* for (x = centre - size; x <= centre + size; x++) { */
  for (x = xmin; x <= xmax; x++)
  {
    /* every column */
    /* for (y = centre - (2*size) ; y <= centre + (2*size) ; y++) {  */
    for (y = ymin; y <= ymax; y++)
    {
      if ((mapshape == MAP_RECTANGLE && abs(centre - y) <= size * 2 && abs(centre - x) <= size) ||
          ((mapshape == MAP_CIRCLE) &&
           (centre - x) * (centre - x) + (centre - y) * (centre - y) / 4 <= (size * size + 1)))
      {
        strcpy(mp, world_map_info[map[x][y]].disp);
        mp += strlen(world_map_info[map[x][y]].disp);
      }
      else
      {
        strcpy(mp++, " ");
      }
    }
    strcpy(mp, "\tn\r\n");
    mp += 4;
  }
  *mp = '\0';
  return strmap;
}

static const char *CompactStringMap(int centre, int size)
{
  static char strmap[MAX_MAP * MAX_MAP * 12 + MAX_MAP * 2 + 1];
  char *mp = strmap;
  int x, y;

  /* every row */
  for (x = centre - size; x <= centre + size; x++)
  {
    /* every column */
    for (y = centre - size; y <= centre + size; y++)
    {
      strcpy(mp, (map[x][y] < 0) ? compact_door_info[NUM_DOOR_TYPES + map[x][y]].disp
                                 : map_info[map[x][y]].disp);
      mp += strlen((map[x][y] < 0) ? compact_door_info[NUM_DOOR_TYPES + map[x][y]].disp
                                   : map_info[map[x][y]].disp);
    }
    strcpy(mp, "\r\n");
    mp += 2;
  }
  *mp = '\0';
  return strmap;
}

struct expanded_map_room
{
  room_rnum room;
  int x;
  int y;
};

static struct room_direction_data *expanded_map_exit(struct char_data *ch, room_rnum room,
                                                     int direction)
{
  struct room_direction_data *exit;

  if (!ch || !VALID_ROOM_RNUM(room) || direction < 0 || direction >= DIR_COUNT)
    return NULL;

  exit = world[room].dir_option[direction];
  if (!exit || exit->to_room == NOWHERE || !VALID_ROOM_RNUM(exit->to_room))
    return NULL;
  if (EXIT_FLAGGED(exit, EX_CLOSED))
    return NULL;
  if (EXIT_FLAGGED(exit, EX_HIDDEN) && !PRF_FLAGGED(ch, PRF_HOLYLIGHT))
    return NULL;

  return exit;
}

static char *build_expanded_room_map(struct char_data *ch, int radius)
{
  static const int map_directions[] = {NORTH, EAST, SOUTH, WEST, NORTHWEST,
                                       NORTHEAST, SOUTHEAST, SOUTHWEST};
  static const int map_offsets[][2] = {{0, 1},  {1, 0},  {0, -1}, {-1, 0},
                                       {-1, 1}, {1, 1}, {1, -1}, {-1, -1}};
  struct expanded_map_room *rooms;
  struct room_direction_data *exit;
  bool *visited;
  int *tiles;
  char *output;
  char *write_position;
  const char *symbol;
  size_t output_size;
  size_t symbol_length;
  size_t tile_count;
  int direction_index;
  int room_count;
  int room_index;
  int tile_index;
  int next_x;
  int next_y;
  int side;
  int x;
  int y;

  if (!ch || !VALID_ROOM_RNUM(IN_ROOM(ch)))
    return NULL;

  radius = URANGE(1, radius, MAX_MAP_RADIUS);
  side = radius * 2 + 1;
  tile_count = (size_t)side * (size_t)side;

  rooms = calloc(tile_count, sizeof(*rooms));
  tiles = malloc(tile_count * sizeof(*tiles));
  visited = calloc((size_t)top_of_world + 1, sizeof(*visited));
  if (!rooms || !tiles || !visited)
  {
    free(rooms);
    free(tiles);
    free(visited);
    return NULL;
  }

  for (tile_index = 0; tile_index < (int)tile_count; tile_index++)
    tiles[tile_index] = SECT_EMPTY;

  rooms[0].room = IN_ROOM(ch);
  rooms[0].x = 0;
  rooms[0].y = 0;
  room_count = 1;
  visited[IN_ROOM(ch)] = TRUE;
  tiles[radius * side + radius] = SECT_HERE;

  for (room_index = 0; room_index < room_count; room_index++)
  {
    for (direction_index = 0;
         direction_index < (int)(sizeof(map_directions) / sizeof(map_directions[0]));
         direction_index++)
    {
      exit = expanded_map_exit(ch, rooms[room_index].room, map_directions[direction_index]);
      if (!exit || visited[exit->to_room])
        continue;

      next_x = rooms[room_index].x + map_offsets[direction_index][0];
      next_y = rooms[room_index].y + map_offsets[direction_index][1];
      if (abs(next_x) > radius || abs(next_y) > radius)
        continue;

      tile_index = (radius - next_y) * side + radius + next_x;
      if (tiles[tile_index] != SECT_EMPTY)
        continue;

      if (room_count >= (int)tile_count)
        break;

      rooms[room_count].room = exit->to_room;
      rooms[room_count].x = next_x;
      rooms[room_count].y = next_y;
      room_count++;
      visited[exit->to_room] = TRUE;

      if (SECT(exit->to_room) >= 0 && SECT(exit->to_room) < NUM_ROOM_SECTORS)
        tiles[tile_index] = SECT(exit->to_room);
      else
        tiles[tile_index] = SECT_STRANGE;
    }
  }

  output_size = 64;
  for (tile_index = 0; tile_index < (int)tile_count; tile_index++)
  {
    symbol = world_map_info[tiles[tile_index]].disp;
    symbol_length = strlen(symbol);
    if (output_size > SIZE_MAX - symbol_length - 2)
    {
      free(rooms);
      free(tiles);
      free(visited);
      return NULL;
    }
    output_size += symbol_length;
  }
  output_size += (size_t)side * 2;

  output = malloc(output_size);
  if (!output)
  {
    free(rooms);
    free(tiles);
    free(visited);
    return NULL;
  }

  write_position = output;
  write_position += snprintf(write_position, output_size, "Map range: %d room%s\r\n", radius,
                             radius == 1 ? "" : "s");
  for (y = 0; y < side; y++)
  {
    for (x = 0; x < side; x++)
    {
      symbol = world_map_info[tiles[y * side + x]].disp;
      symbol_length = strlen(symbol);
      memcpy(write_position, symbol, symbol_length);
      write_position += symbol_length;
    }
    memcpy(write_position, "\r\n", 2);
    write_position += 2;
  }
  *write_position = '\0';

  free(rooms);
  free(tiles);
  free(visited);
  return output;
}

static bool display_expanded_map(struct char_data *ch, int radius)
{
  char *map_text;
  int diameter;

  if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_WILDERNESS))
  {
    diameter = radius * 2 + 1;
    map_text = gen_ascii_wilderness_map(diameter, X_LOC(ch), Y_LOC(ch), MAP_TYPE_WEATHER);
  }
  else
  {
    map_text = build_expanded_room_map(ch, radius);
  }

  if (!map_text)
  {
    send_to_char(ch, "The surrounding area is too difficult to map right now.\r\n");
    return FALSE;
  }

  if (ch->desc)
    page_string(ch->desc, map_text, TRUE);
  free(map_text);
  return TRUE;
}

/* Display a nicely formatted map with a legend */
void perform_map(struct char_data *ch, const char *argument, bool worldmap)
{
  int size = DEFAULT_MAP_SIZE;
  int centre, x, y, min, max;
  char arg1[MAX_INPUT_LENGTH] = {'\0'}, arg2[MAX_INPUT_LENGTH] = {'\0'},
       buf[MAX_STRING_LENGTH] = {'\0'}, buf1[MAX_STRING_LENGTH] = {'\0'},
       buf2[MAX_STRING_LENGTH] = {'\0'};
  int count = 0;
  int ew_size = 0, ns_size = 0;
  int mapshape = MAP_CIRCLE;

  two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
  if (*arg1)
  {
    size = atoi(arg1);
  }
  if (*arg2)
  {
    if (is_abbrev(arg2, "normal"))
      worldmap = FALSE;
    else if (is_abbrev(arg2, "world"))
      worldmap = TRUE;
    else
    {
      send_to_char(ch, "Usage: \tymap <distance> [ normal | world ]\tn");
      return;
    }
  }

  if (size < 0)
  {
    size = -size;
    mapshape = MAP_RECTANGLE;
  }
  size = URANGE(1, size, MAX_MAP_SIZE);

  centre = MAX_MAP / 2;

  if (worldmap)
  {
    min = centre - 2 * size;
    max = centre + 2 * size;
  }
  else
  {
    min = centre - size;
    max = centre + size;
  }

  /* Blank the map */
  for (x = 0; x < MAX_MAP; ++x)
    for (y = 0; y < MAX_MAP; ++y)
      map[x][y] = (!(y % 2) && !worldmap) ? DOOR_NONE : SECT_EMPTY;

  /* starts the mapping with the centre room */
  MapArea(IN_ROOM(ch), ch, centre, centre, min, max, ns_size / 2, ew_size / 2, worldmap);

  /* marks the center, where ch is */
  map[centre][centre] = SECT_HERE;

  send_to_char(ch, " \tb--\tB= \tCLuminari Map System \tB=\tb--\tn\r\n"
                   "\tD  .-.__--.,--.__.-.\tn\r\n");

  count += sprintf(buf + count, "\tn\tn\tn%s Up\\\\", door_info[NUM_DOOR_TYPES + DOOR_UP].disp);
  count += sprintf(buf + count, "\tn\tn\tn%s Down\\\\", door_info[NUM_DOOR_TYPES + DOOR_DOWN].disp);
  count += sprintf(buf + count, "\tn%s You\\\\", map_info[SECT_HERE].disp);
  count += sprintf(buf + count, "\tn%s Inside\\\\", map_info[SECT_INSIDE].disp);
  count += sprintf(buf + count, "\tn%s City\\\\", map_info[SECT_CITY].disp);
  count += sprintf(buf + count, "\tn%s Field\\\\", map_info[SECT_FIELD].disp);
  count += sprintf(buf + count, "\tn%s Forest\\\\", map_info[SECT_FOREST].disp);
  count += sprintf(buf + count, "\tn%s Hills\\\\", map_info[SECT_HILLS].disp);
  count += sprintf(buf + count, "\tn%s Mountain\\\\", map_info[SECT_MOUNTAIN].disp);
  count += sprintf(buf + count, "\tn%s Water\\\\", map_info[SECT_WATER_SWIM].disp);
  count += sprintf(buf + count, "\tn%s Deep Water\\\\", map_info[SECT_WATER_NOSWIM].disp);
  count += sprintf(buf + count, "\tn%s Air\\\\", map_info[SECT_FLYING].disp);
  count += sprintf(buf + count, "\tn%s Underwater\\\\", map_info[SECT_UNDERWATER].disp);
  count += sprintf(buf + count, "\tn%s Zone Entry\\\\", map_info[SECT_ZONE_START].disp);
  count += sprintf(buf + count, "\tn%s Road N-S\\\\", map_info[SECT_ROAD_NS].disp);
  count += sprintf(buf + count, "\tn%s Road E-W\\\\", map_info[SECT_ROAD_EW].disp);
  count += sprintf(buf + count, "\tn%s Intersect\\\\", map_info[SECT_ROAD_INT].disp);
  count += sprintf(buf + count, "\tn%s Desert\\\\", map_info[SECT_DESERT].disp);
  count += sprintf(buf + count, "\tn%s Ocean\\\\", map_info[SECT_OCEAN].disp);
  count += sprintf(buf + count, "\tn%s Marsh\\\\", map_info[SECT_MARSHLAND].disp);
  count += sprintf(buf + count, "\tn%s High Mount\\\\", map_info[SECT_HIGH_MOUNTAIN].disp);
  count += sprintf(buf + count, "\tn%s Planes\\\\", map_info[SECT_PLANES].disp);
  count += sprintf(buf + count, "\tn%s UD Wild\\\\", map_info[SECT_UD_WILD].disp);
  count += sprintf(buf + count, "\tn%s UD City\\\\", map_info[SECT_UD_CITY].disp);
  count += sprintf(buf + count, "\tn%s UD Inside\\\\", map_info[SECT_UD_INSIDE].disp);
  count += sprintf(buf + count, "\tn%s UD Water\\\\", map_info[SECT_UD_WATER].disp);
  count += sprintf(buf + count, "\tn%s UD D Water\\\\", map_info[SECT_UD_NOSWIM].disp);
  count += sprintf(buf + count, "\tn%s UD Air\\\\", map_info[SECT_UD_NOGROUND].disp);
  count += sprintf(buf + count, "\tn%s Lava\\\\", map_info[SECT_LAVA].disp);
  count += sprintf(buf + count, "\tn%s D Rd N-S\\\\", map_info[SECT_D_ROAD_NS].disp);
  count += sprintf(buf + count, "\tn%s D Rd E-W\\\\", map_info[SECT_D_ROAD_EW].disp);
  count += sprintf(buf + count, "\tn%s D Inters\\\\", map_info[SECT_D_ROAD_INT].disp);
  count += sprintf(buf + count, "\tn%s Cave\\\\", map_info[SECT_CAVE].disp);
  count += sprintf(buf + count, "\tn%s Sea Port\\\\", map_info[SECT_SEAPORT].disp);
  count += sprintf(buf + count, "\tn%s Inside Room\\\\", map_info[SECT_INSIDE_ROOM].disp);
  count += sprintf(buf + count, "\tn%s River\\\\", map_info[SECT_RIVER].disp);

  strcpy(buf, strfrmt(buf, LEGEND_WIDTH, CANVAS_HEIGHT + 2, FALSE, TRUE, TRUE));

  /* Start with an empty column */
  char empty_str[] = ""; // To not pass literal to avoid compiler warning
  strcpy(buf1, strfrmt(empty_str, 0, CANVAS_HEIGHT + 2, FALSE, FALSE, TRUE));

  /* Paste the legend */
  strcpy(buf2, strpaste(buf1, buf, "\tD | \tn"));

  /* Set up the map */
  memset(buf, ' ', CANVAS_WIDTH);
  count = (CANVAS_WIDTH);
  if (worldmap)
    count += sprintf(buf + count, "\r\n%s", WorldMap(centre, size, mapshape, MAP_NORMAL));
  else
    count += sprintf(buf + count, "\r\n%s", StringMap(centre, size));
  memset(buf + count, ' ', CANVAS_WIDTH);
  strcpy(buf + count + CANVAS_WIDTH, "\r\n");
  /* Paste it on */
  strcpy(buf2, strpaste(buf2, buf, "\tD | \tn"));
  /* Paste on the right border */
  strcpy(buf2, strpaste(buf2, buf1, "  "));
  /* Print it all out */
  send_to_char(ch, "%s", buf2);

  send_to_char(ch, "\tD `.-.__--.,-.__.-.-'\tn\r\n");
  return;
}

/* Display a string with the map beside it */
void str_and_map(char *str, struct char_data *ch, room_vnum target_room)
{
  int size, centre, x, y, min, max, char_size;
  int ew_size = 0, ns_size = 0;
  bool worldmap;
  // char *mapbuf = NULL;

  /* Check MUDs map config options - if disabled, just show room decsription */
  if (!can_see_map(ch))
  {
    send_to_char(ch, "%s", strfrmt(str, GET_SCREEN_WIDTH(ch), 1, FALSE, FALSE, FALSE));
    return;
  }

  worldmap = show_worldmap(ch);

  if (!PRF_FLAGGED(ch, PRF_AUTOMAP) ||
      (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_NOMAP) && GET_LEVEL(ch) < LVL_IMMORT))
  {
    send_to_char(ch, "%s", strfrmt(str, GET_SCREEN_WIDTH(ch), 1, FALSE, FALSE, FALSE));
    return;
  }

  size = CONFIG_MINIMAP_SIZE;
  centre = MAX_MAP / 2;
  min = centre - 2 * size;
  max = centre + 2 * size;

  for (x = 0; x < MAX_MAP; ++x)
    for (y = 0; y < MAX_MAP; ++y)
      map[x][y] = (!(y % 2) && !worldmap) ? DOOR_NONE : SECT_EMPTY;

  /* starts the mapping with the center room */
  MapArea(target_room, ch, centre, centre, min, max, ns_size / 2, ew_size / 2, worldmap);
  map[centre][centre] = SECT_HERE;

  /* char_size = rooms + doors + padding */
  if (worldmap)
    char_size = size * 4 + 5;
  else
    char_size = 3 * (size + 1) + (size) + 4;

  if (worldmap)
  {
    send_to_char(ch, "%s", WorldMap(centre, size, MAP_CIRCLE, MAP_COMPACT));
    //    send_to_char(ch, "%s", strpaste(strfrmt(str, GET_SCREEN_WIDTH(ch) - char_size, size*2 + 1,
    //	FALSE, TRUE, TRUE), WorldMap(centre, size, MAP_CIRCLE, MAP_COMPACT), " \tn"));
  }
  else
  {
    send_to_char(
        ch, "%s",
        strpaste(strfrmt(str, GET_SCREEN_WIDTH(ch) - char_size, size * 2 + 1, FALSE, TRUE, TRUE),
                 CompactStringMap(centre, size), " \tn"));
  }
}

bool show_worldmap(struct char_data *ch)
{
  room_rnum rm = IN_ROOM(ch);
  zone_rnum zn = GET_ROOM_ZONE(rm);

  if (ROOM_FLAGGED(rm, ROOM_WORLDMAP))
    return TRUE;
  if (ZONE_FLAGGED(zn, ZONE_WORLDMAP))
    return TRUE;

  return FALSE;
}

ACMD(do_map)
{
  char radius_argument[MAX_INPUT_LENGTH] = {'\0'};
  char extra_argument[MAX_INPUT_LENGTH] = {'\0'};
  char *endptr;
  time_t current_time;
  long requested_radius;
  long remaining;
  int radius;
  bool mortal;

  if (GET_LEVEL(ch) < LVL_IMMORT && ROOM_FLAGGED(IN_ROOM(ch), ROOM_FOG))
  {
    send_to_char(ch, "The fog in this room prevents you from seeing the map.\r\n");
    return;
  }

  if (!can_see_map(ch))
  {
    send_to_char(ch, "Sorry, the map is disabled!\r\n");
    return;
  }
  if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_NOMAP) && GET_LEVEL(ch) < LVL_IMMORT)
  {
    send_to_char(ch, "A magical force prevents you from seeing beyond this room.\r\n");
    return;
  }
  if (IS_DARK(IN_ROOM(ch)) && !CAN_SEE_IN_DARK(ch) && !has_blindsense(ch))
  {
    send_to_char(ch, "It is too dark to see the map.\r\n");
    return;
  }
  else if (AFF_FLAGGED(ch, AFF_BLIND) && GET_LEVEL(ch) < LVL_IMMORT && !has_blindsense(ch))
  {
    send_to_char(ch, "You can't see the map while blind!\r\n");
    return;
  }

  mortal = GET_LEVEL(ch) < LVL_IMMORT;
  current_time = time(NULL);

  if (mortal)
  {
    if (ch->char_specials.map_cooldown > current_time)
    {
      remaining = (long)(ch->char_specials.map_cooldown - current_time);
      send_to_char(ch, "You must wait %ld more second%s before surveying the area again.\r\n",
                   remaining, remaining == 1 ? "" : "s");
      return;
    }

    radius = URANGE(1, skill_roll(ch, ABILITY_SURVIVAL), MAX_MAP_RADIUS);
    send_to_char(ch, "Your Survival check reveals %d room%s in every direction.\r\n", radius,
                 radius == 1 ? "" : "s");
  }
  else
  {
    two_arguments(argument, radius_argument, sizeof(radius_argument), extra_argument,
                  sizeof(extra_argument));
    if (*extra_argument && !is_abbrev(extra_argument, "normal") &&
        !is_abbrev(extra_argument, "world"))
    {
      send_to_char(ch, "Usage: map [range] [normal | world] (maximum %d)\r\n",
                   MAX_MAP_RADIUS);
      return;
    }

    if (!*radius_argument)
    {
      radius = URANGE(1, DEFAULT_MAP_SIZE, MAX_MAP_RADIUS);
    }
    else
    {
      errno = 0;
      endptr = NULL;
      requested_radius = strtol(radius_argument, &endptr, 10);
      if (errno != 0 || !endptr || *endptr != '\0' || requested_radius < 1 ||
          requested_radius > MAX_MAP_RADIUS)
      {
        send_to_char(ch, "Map range must be a number from 1 to %d.\r\n", MAX_MAP_RADIUS);
        return;
      }
      radius = (int)requested_radius;
    }
  }

  if (display_expanded_map(ch, radius) && mortal)
    ch->char_specials.map_cooldown = current_time + MAP_COMMAND_COOLDOWN;
}

/*EOF*/
