#ifndef _ROGUE14_COMMAND_PRIVATE_H_
#define _ROGUE14_COMMAND_PRIVATE_H_

#include "pack.h"

#include "command.h"

/* In command.c */
bool command_do(char ch);
bool command_wizard_do(char ch);

/* In command_sub.c */
bool command_use_stairs(char up_or_down); /* Go to next/previous level */
bool command_attack(bool fight_to_death); /* Attack and fight something */
bool command_name_item(void);             /* Call an item something */
bool command_identify_character(void);    /* Identify monster or item */
bool command_identify_trap(void);         /* Give the name of a trap          */
bool command_quit(void);                  /* Ask player if she wants to quit  */
bool command_pick_up(void);               /* Pick up something at her feet */
bool command_help(void);                  /* Give command help */
bool command_again(void);                 /* Repeat last command */
bool command_search(void);                /* Search for hidden things */
void command_shell(void);                 /* Let them escape for a while */
bool command_show_inventory(void);        /* Print player inventory */
bool command_take_off(enum equipment_pos pos); /* Unequip something */
bool command_toggle_wizard(void);         /* Toggle wizard-mode on or off     */
bool command_wield(void);                 /* Asks player for weapon to wield */

#endif /* _ROGUE14_COMMAND_PRIVATE_H_ */
