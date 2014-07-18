#ifndef _ROGUE14_COMMAND_PRIVATE_H_
#define _ROGUE14_COMMAND_PRIVATE_H_

#include "command.h"

/* In command.c */
bool do_command(char ch);
bool do_wizard_command(char ch);

/* In command_sub.c */
bool change_dungeon_level(char up_or_down); /* Go to next/previous level */
bool fight_monster(bool fight_to_death); /* Attack and fight something */
bool give_item_nickname();       /* Call an item something */
bool identify_a_character();     /* Identify monster or item */
bool identify_trap();              /* Give the name of a trap          */
bool maybe_quit();                 /* Ask player if she wants to quit  */
bool pick_up_item_from_ground(); /* Attempt to pick up something at her feet */
bool print_currently_wearing(char thing); /* Print weapon / armor info */
bool print_help();               /* Give command help */
bool repeat_last_command();
bool search();                /* Find traps, hidden doors and passages */
bool show_players_inventory();
bool toggle_wizard_mode();    /* Toggle wizard-mode on or off     */

#endif /* _ROGUE14_COMMAND_PRIVATE_H_ */
