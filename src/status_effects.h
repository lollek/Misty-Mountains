#ifndef _ROGUE14_STATUS_EFFECTS_H_
#define _ROGUE14_STATUS_EFFECTS_H_

#include "rogue.h" /* THING */
#include <stdbool.h>

/* TODO: Make getters/setters for all status effects */

/* Duration of effects */
#define HUHDURATION	spread(20)  /* Confusion */
#define MFINDDURATION	spread(20)  /* Monster find */
#define HASTEDURATION	rnd(4)+4    /* Haste */
#define SEEDURATION	spread(850) /* See invisible / blind / hallucinating */
#define LEVITDUR	spread(30)  /* Levitation */
#define SLEEPTIME	spread(7)   /* Sleep */
#define STUCKTIME	spread(3)   /* Stuck */

/* Status getters */
inline bool is_confusing(THING *thing);    /* Causes confusion on attack */
inline bool is_true_seeing(THING *thing);  /* Can see invisible creatures */
inline bool is_blind(THING *thing);        /* Creature is blind */
inline bool is_cancelled(THING *thing);    /* Creature's ability is cancelled */
inline bool is_levitating(THING *thing);   /* Creature is levitating */
inline bool is_found(THING *thing);        /* Creature has been seen */
inline bool is_confused(THING *thing);     /* Creature is confused */
inline bool is_invisible(THING *thing);    /* Creature is invisible */
inline bool is_hallucinating(THING *thing);/* Creature is tripping on acid */

/* Status setters */
inline void set_confusing(THING *thing, bool status);
void set_true_seeing(THING *thing, bool status, bool permanent);
inline void set_blind(THING *thing, bool status);
inline void set_cancelled(THING *thing, bool status);
inline void set_levitating(THING *thing, bool status);
inline void set_found(THING *thing, bool status);
inline void set_confused(THING *thing, bool status);
inline void set_invisible(THING *thing, bool status);
inline void set_hallucinating(THING *thing, bool status);

/* Daemon helpers */
void daemon_remove_true_seeing(void);

/* Functions */
void fall_asleep(void);                     /* Take a unwilling powernap */
void become_stuck(void);                    /* Become immobile */
void become_restored(void);                 /* Remove bad status effects */
void become_poisoned(void);                 /* Add poisoned status effect */
void become_confused(bool permanent);       /* Add confused status effect */
void remove_confusion(void);                /* Restore player's mental health */
void become_healed(void);                   /* Add healed status effect */
void become_extra_healed(void);             /* Better healing */
void become_stronger(void);                 /* Add strength */
void become_monster_seeing(bool permanent); /* Add see-monster effect */
void become_tripping(bool permanent);       /* Add tripping effect */
void remove_tripping(void);
void become_hasted(bool permanent);         /* Become quicker */
void remove_hasted(void);
void become_blind(bool permanent);
void cure_blindness(void);
void become_levitating(bool permanent);
void remove_levitating(void);
void raise_level(void);                     /* Level up */

#endif /* _ROGUE14_STATUS_EFFECTS_H_ */
