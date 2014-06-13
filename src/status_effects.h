#ifndef _ROGUE14_STATUS_EFFECTS_H_
#define _ROGUE14_STATUS_EFFECTS_H_

#include <stdbool.h>

/* Duration of effects */
#define HUHDURATION	spread(20)
#define MFINDDURATION	spread(20)
#define HASTEDURATION	rnd(4)+4

/* Status macros */
#define is_hallucinating(thing) ((bool)(((thing).t_flags & ISHALU) != 0))

/* Functions */
void become_restored();                     /* Remove bad status effects */
void become_poisoned();                     /* Add poisoned status effect */
void become_confused(bool permanent);       /* Add confused status effect */
void become_healed();                       /* Add healed status effect */
void become_extra_healed();                 /* Better healing */
void become_stronger();                     /* Add strength */
void become_monster_seeing(bool permanent); /* Add see-monster effect */
void become_tripping(bool permanent);       /* Add tripping effect */
void become_true_seeing(bool permanent);    /* Add see-invisiable effect */
void become_hasted(bool permanent);         /* Become quicker */
void become_blind(bool permanent);
void become_levitating(bool permanent);
void raise_level();                         /* Level up */

#endif /* _ROGUE14_STATUS_EFFECTS_H_ */
