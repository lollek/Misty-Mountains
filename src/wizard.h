#ifndef _ROGUE14_WIZARD_H_
#define _ROGUE14_WIZARD_H_

/* Print specific list of possible items to choose from */
void pr_spec(char ch);
/* List possible potions, scrolls, etc. for wizard. */
int pr_list(void);
/* wizard command for getting anything he wants */
void create_obj(void);
/* Print out the map for the wizard */
void show_map(void);

void wizard_levels_and_gear(void);

#endif /* _ROGUE14_WIZARD_H_ */
