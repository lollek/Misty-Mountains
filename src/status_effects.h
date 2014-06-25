#ifndef _ROGUE14_STATUS_EFFECTS_H_
#define _ROGUE14_STATUS_EFFECTS_H_

#include "rogue.h" /* THING */
#include <stdbool.h>

/* Duration of effects */
#define HUHDURATION	spread(20)  /* Confusion */
#define MFINDDURATION	spread(20)  /* Monster find */
#define HASTEDURATION	rnd(4)+4    /* Haste */
#define SEEDURATION	spread(850) /* See invisible / blind / hallucinating */
#define LEVITDUR	spread(30)  /* Levitation */
#define SLEEPTIME	spread(7)   /* Sleep */

/* Status getters */
inline bool is_confusing(THING *thing);
inline bool is_hallucinating(THING *thing);
inline bool is_blind(THING *thing);
inline bool is_levitating(THING *thing);
inline bool is_confused(THING *thing);
inline bool is_invisible(THING *thing);

/* Status setters */
inline void set_confusing(THING *thing, bool status);
inline void set_hallucinating(THING *thing, bool status);
inline void set_blind(THING *thing, bool status);
inline void set_levitating(THING *thing, bool status);
inline void set_confused(THING *thing, bool status);
inline void set_invisible(THING *thing, bool status);

/* Functions */
void fall_asleep();                         /* Take a unwilling powernap */
void become_restored();                     /* Remove bad status effects */
void become_poisoned();                     /* Add poisoned status effect */
void become_confused(bool permanent);       /* Add confused status effect */
void remove_confusion();                    /* Restore player's mental health */
void become_healed();                       /* Add healed status effect */
void become_extra_healed();                 /* Better healing */
void become_stronger();                     /* Add strength */
void become_monster_seeing(bool permanent); /* Add see-monster effect */
void become_tripping(bool permanent);       /* Add tripping effect */
void remove_tripping();
void become_true_seeing(bool permanent);    /* Add see-invisiable effect */
void remove_true_seeing();
void become_hasted(bool permanent);         /* Become quicker */
void remove_hasted();
void become_blind(bool permanent);
void cure_blindness();
void become_levitating(bool permanent);
void remove_levitating();
void raise_level();                         /* Level up */

#endif /* _ROGUE14_STATUS_EFFECTS_H_ */
