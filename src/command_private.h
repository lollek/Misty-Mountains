#pragma once

#include "pack.h"

#include "command.h"

// In command.c
bool command_do(char ch);
bool command_wizard_do(char ch);

// In command_sub.c
bool command_attack(bool fight_to_death); // Attack and fight something
bool command_close();                  // Close door
bool command_drop();                  // Drop something
bool command_eat();                   // Eat something
bool command_help();                  // Give command help
bool command_identify_trap();         // Give the name of a trap
bool command_name_item();             // Call an item something
bool command_open();                  // Open door
bool command_pick_up();               // Pick up something at her feet
bool command_quit();                  // Ask player if she wants to quit
bool command_rest();                  // Rest until full hp
bool command_run(char ch, bool cautiously); // Run in a direction
void command_shell();                 // Let them escape for a while
bool command_show_inventory();        // Print player inventory
bool command_show_equipment();        // Print player equpment
bool command_take_off(enum equipment_pos pos); // Unequip something
bool command_throw();                 // Throw or shoot something*/
bool command_use_stairs(char up_or_down); // Go to next/previous level
bool command_wield();                 // Asks player for weapon to wield
bool command_wear();                      // Let player select something to wear
bool command_ring_put_on();               // Put on a ring
bool command_ring_take_off();             // Take off a ring
bool command_read_scroll();               // Read a scroll

bool command_weapon_wield_last_used();    // Wield last used weapon
void command_weapon_set_last_used(Item* weapon); // Set weapon to last used
bool command_weapon_wield(Item* weapon);  // Wield given weapon
