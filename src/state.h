#ifndef _ROGUE14_STATE_H_
#define _ROGUE14_STATE_H_

#include <stdint.h>

#define state_save_index rs_write_string_index
int state_save_index(void *fd, const char **start, int max, const char *str);
#define state_load_index rs_read_string_index
int state_load_index(void *fd, const char **start, int max, const char **str);

#define state_save_obj_info rs_write_obj_info
int state_save_obj_info(void *savef, const struct obj_info *i, int count);
#define state_load_obj_info rs_read_obj_info
int state_load_obj_info(void *inf,         struct obj_info *i, int count);

#define state_save_file rs_save_file
int state_save_file(FILE *savef);
#define state_restore_file rs_restore_file
int state_restore_file(FILE *inf);

bool state_save_int8(void *fd, int8_t data);
bool state_load_int8(void *fd, int8_t *data);

#define state_save_list rs_write_object_list
int state_save_list(void *fd, const THING *list);
#define state_load_list rs_read_object_list
int state_load_list(void *fd, THING **list);

#endif /* _ROGUE14_STATE_H_ */
