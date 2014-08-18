#ifndef _ROGUE14_COMMAND_PRIVATE_H_
#define _ROGUE14_COMMAND_PRIVATE_H_

#include "pack.h"

#include "command.h"

/* In command.c */
bool do_command(char ch);
bool do_wizard_command(char ch);

/* In command_sub.c */
bool change_dungeon_level(char up_or_down); /* Go to next/previous level */
bool fight_monster(bool fight_to_death); /* Attack and fight something */
bool give_item_nickname(void);       /* Call an item something */
bool identify_a_character(void);     /* Identify monster or item */
bool identify_trap(void);              /* Give the name of a trap          */
bool maybe_quit(void);                 /* Ask player if she wants to quit  */
bool pick_up_item_from_ground(void); /* Pick up something at her feet */
bool print_help(void);               /* Give command help */
bool repeat_last_command(void);
bool search(void);                /* Find traps, hidden doors and passages */
bool show_players_inventory(void);
bool take_off_players_equipment(enum equipment_pos pos);
bool toggle_wizard_mode(void);    /* Toggle wizard-mode on or off     */

#endif /* _ROGUE14_COMMAND_PRIVATE_H_ */
