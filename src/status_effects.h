#ifndef _ROGUE14_STATUS_EFFECTS_H_
#define _ROGUE14_STATUS_EFFECTS_H_

#include "rogue.h" /* THING */
#include <stdbool.h>


/* Duration of effects */
#define HUHDURATION     spread(20)  /* Confusion */
#define MFINDDURATION   spread(20)  /* Monster find */
#define HASTEDURATION   rnd(4)+4    /* Haste */
#define SEEDURATION     spread(850) /* See invisible / blind / hallucinating */
#define LEVITDUR        spread(30)  /* Levitation */
#define SLEEPTIME       spread(7)   /* Sleep */
#define STUCKTIME       spread(3)   /* Stuck */

/* Functions */
void fall_asleep(void);                     /* Take a unwilling powernap */
void become_stuck(void);                    /* Become immobile */
void become_restored(void);                 /* Remove bad status effects */
void become_poisoned(void);                 /* Add poisoned status effect */
void become_healed(void);                   /* Add healed status effect */
void become_extra_healed(void);             /* Better healing */
void become_stronger(void);                 /* Add strength */
void raise_level(void);                     /* Level up */
void teleport(THING *thing, coord *target); /* Teleport someone somewhere */

#endif /* _ROGUE14_STATUS_EFFECTS_H_ */
