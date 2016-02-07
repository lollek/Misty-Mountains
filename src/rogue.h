#pragma once

/* Various constants */
#define WANDERTIME	spread(70)
#define BEFORE		spread(1)
#define AFTER		spread(2)
#define LEFT		0
#define RIGHT		1
#define BOLT_LENGTH	6
#define LAMPDIST	3

/* Save against things */
#define VS_POISON	00
#define VS_MAGIC	03


/* External variables */

extern bool door_stop;
extern bool to_death;

extern char dir_ch;
extern char runch;
