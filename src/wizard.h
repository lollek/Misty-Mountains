#pragma once

extern bool wizard;            /* True if allows wizard commands (debug mode) */
extern bool wizard_dicerolls;  // True if we want to see dicerolls

/* List possible potions, scrolls, etc. for wizard. */
int wizard_list_items();

/* wizard command for getting anything he wants */
void wizard_create_item();

/* Print out the map for the wizard */
void wizard_show_map();

void wizard_levels_and_gear();
