#ifndef _ROGUE14_STATE_H_
#define _ROGUE14_STATE_H_

/* We're gonna rename these for real ... later */
#define state_save_index rs_write_string_index
#define state_load_index rs_read_string_index

#define state_save_obj_info rs_write_obj_info
#define state_load_obj_info rs_read_obj_info

int state_save_index(void *fd, const char **start, int max, const char *str);
int state_load_index(void *fd, const char **start, int max, const char **str);

int state_save_obj_info(void *savef, const struct obj_info *i, int count);
int state_load_obj_info(void *inf,         struct obj_info *i, int count);

#endif /* _ROGUE14_STATE_H_ */
