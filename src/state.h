#ifndef _ROGUE14_STATE_H_
#define _ROGUE14_STATE_H_

#include <stdint.h>

int state_save_index(void *fd, const char **start, int max, const char *str);
int state_load_index(void *fd, const char **start, int max, const char **str);

int state_save_obj_info(void *savef, const struct obj_info *i, int count);
int state_load_obj_info(void *inf,         struct obj_info *i, int count);

int state_save_file(FILE *savef);
int state_load_file(FILE *inf);

bool state_save_int8(void *fd, int8_t data);
bool state_load_int8(void *fd, int8_t *data);

int state_save_list(void *fd, const THING *list);
int state_load_list(void *fd, THING **list);

#endif /* _ROGUE14_STATE_H_ */
