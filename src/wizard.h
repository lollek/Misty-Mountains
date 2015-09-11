#ifndef ROGUE14_WIZARD_H
#define ROGUE14_WIZARD_H

extern int wizard; /* True if allows wizard commands (debug mode) */

/* List possible potions, scrolls, etc. for wizard. */
int wizard_list_items(void);

/* wizard command for getting anything he wants */
void wizard_create_item(void);

/* Print out the map for the wizard */
void wizard_show_map(void);

void wizard_levels_and_gear(void);

#endif /* ROGUE14_WIZARD_H */
