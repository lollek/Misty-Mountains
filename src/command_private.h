#pragma once

#include "pack.h"

#include "command.h"

/* In command.c */
bool command_do(char ch);
bool command_wizard_do(char ch);

/* In command_sub.c */
bool command_attack(bool fight_to_death); /* Attack and fight something */
bool command_drop(void);                  /* Drop something */
bool command_eat(void);                   /* Eat something */
bool command_help(void);                  /* Give command help */
bool command_identify_character(void);    /* Identify monster or item */
bool command_identify_trap(void);         /* Give the name of a trap          */
bool command_name_item(void);             /* Call an item something */
bool command_pick_up(void);               /* Pick up something at her feet */
bool command_quit(void);                  /* Ask player if she wants to quit  */
bool command_rest(void);                  /* Rest until full hp */
bool command_run(char ch, bool cautiously); /* Run in a direction */
void command_shell(void);                 /* Let them escape for a while */
bool command_show_inventory(void);        /* Print player inventory */
bool command_take_off(enum equipment_pos pos); /* Unequip something */
bool command_throw(void);                 /* Throw or shoot something*/
bool command_use_stairs(char up_or_down); /* Go to next/previous level */
bool command_wield(void);                 /* Asks player for weapon to wield */
bool command_wear();                      /* Let player select something to wear */
bool command_ring_put_on();               /* Put on a ring */
bool command_ring_take_off();             /* Take off a ring */
