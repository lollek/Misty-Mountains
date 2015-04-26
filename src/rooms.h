#ifndef _ROGUE14_ROOMS_H_
#define _ROGUE14_ROOMS_H_

/* Create rooms and corridors with a connectivity graph */
void rooms_create(void);

/* Find a valid floor spot in this room.  If rp is NULL, then
 * pick a new room each time around the loop.  */
bool room_find_floor(struct room *rp, coord *cp, int limit, bool monst);

/* Code that is executed whenver you appear in a room */
void room_enter(coord *cp);

/* Code for when we exit a room */
void room_leave(coord *cp);

/* Pick a room that is really there */
int room_random(void);

#endif /* _ROGUE14_ROOMS_H_ */
