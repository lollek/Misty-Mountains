#ifndef _ROGUE14_ROOMS_H_
#define _ROGUE14_ROOMS_H_

#include "coord.h"

struct room {
  coord r_pos;      /* Upper left corner */
  coord r_max;      /* Size of room */
  coord r_gold;     /* Where the gold is */
  int r_goldval;    /* How much the gold is worth */
  int r_flags;      /* info about the room */
  int r_nexits;     /* Number of exits */
  coord r_exit[12]; /* Where the exits are */
};

/* flags for rooms */
#define ISDARK	0000001		/* room is dark */
#define ISGONE	0000002		/* room is gone (a corridor) */
#define ISMAZE	0000004		/* room is gone (a corridor) */

/* Variables. TODO: Move these */
#define MAXROOMS  9 /* max rooms per level */
extern struct room* oldrp;			/* Roomin(&oldpos) */
extern struct room rooms[MAXROOMS];		/* One for each room -- A level */

/* Create rooms and corridors with a connectivity graph */
void rooms_create(void);

/* Find a valid floor spot in this room.  If rp is NULL, then
 * pick a new room each time around the loop.  */
bool room_find_floor(struct room* rp, coord* cp, int limit, bool monst);

/* Code that is executed whenver you appear in a room */
void room_enter(coord* cp);

/* Code for when we exit a room */
void room_leave(coord* cp);

/* Pick a room that is really there */
int room_random(void);

#endif /* _ROGUE14_ROOMS_H_ */
