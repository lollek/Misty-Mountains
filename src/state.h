#ifndef _ROGUE14_STATE_H_
#define _ROGUE14_STATE_H_

#include <stdint.h>

bool state_save_index(const char **start, int max, const char *str);
bool state_load_index(const char **start, int max, const char **str);

bool state_save_obj_info(const struct obj_info *i, int count);
bool state_load_obj_info(      struct obj_info *i, int count);

bool state_save_file(FILE *savef);
bool state_load_file(FILE *inf);

bool state_save_int8(int8_t data);
bool state_load_int8(int8_t *data);

bool state_save_list(const THING *list);
bool state_load_list(THING **list);

#endif /* _ROGUE14_STATE_H_ */
