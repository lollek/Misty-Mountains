#ifndef _ROGUE14_COMMAND_SUB_H_
#define _ROGUE14_COMMAND_SUB_H_

/* These are used in change_dungeon_level */
#define UP 1
#define DOWN 0

/* Functions called by command.c */

bool change_dungeon_level(bool up_or_down); /* Go to next/previous level */
bool identify_a_character();     /* Identify monster or item */
bool pick_up_item_from_ground(); /* Attempt to pick up something at her feet */
bool print_help();               /* Give command help */

#endif /* _ROGUE14_COMMAND_SUB_H_ */
