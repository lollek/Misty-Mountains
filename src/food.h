#ifndef ROGUE14_FOOD_H
#define ROGUE14_FOOD_H

#include <stdbool.h>

bool food_save_state(void);
bool food_load_state(void);

void food_eat(void);
void food_digest(void);

int food_nutrition_left(void);
char const* food_hunger_state(void);

#endif /* ROGUE14_FOOD_H */
