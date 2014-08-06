#ifndef _ROGUE14_PACK_H_
#define _ROGUE14_PACK_H_

#include "rogue.h"

/* Pick up an object and add it to the pack.  If the argument is
 * non-null use it as the linked_list pointer instead of gettting
 * it off the ground. */
void add_pack(THING *obj, bool silent);

/* take an item out of the pack */
THING *leave_pack(THING *obj, bool newobj, bool all);

/* Add something to characters pack */
void pick_up(char ch); /* TODO: Maybe move to command.c */

/* Allow player to inventory a single item */
void picky_inven(void); /* TODO: Maybe move to command.c */

/* Pick something out of a pack for a purpose */
THING *get_item(const char *purpose, int type);

/* Return the character at hero's position, taking see_floor into account */
char floor_at(void);

/* Reset the last command when the current one is aborted */
void reset_last(void); /* TODO: Why is this even here? */

/* Check if she's carrying anything */
#define players_inventory_is_empty() (player.t_pack == NULL)

/* Counts how many items she is carrying */
#define items_in_pack() items_in_pack_of_type(0)

/* Counts how many items she is carrying of a certain type */
unsigned items_in_pack_of_type(int type);

bool player_has_amulet(void);
bool print_inventory(int type);
void clear_inventory(void);
size_t evaluate_players_inventory(void);


#endif /* _ROGUE14_PACK_H_ */
