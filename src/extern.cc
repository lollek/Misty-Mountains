#include <string>

#include "rogue.h"

bool door_stop = false;			/* Stop running when we pass a door */
bool to_death = false;			/* Fighting is to the death! */

char dir_ch;				/* Direction from last get_dir() call */
char runch;				/* Direction player is running */
std::string whoami;			/* Name of player */

