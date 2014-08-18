#ifndef _ROGUE14_STATUS_EFFECTS_H_
#define _ROGUE14_STATUS_EFFECTS_H_

#include "rogue.h" /* THING */
#include <stdbool.h>

/* TODO: Make getters/setters for all status effects */

/* Duration of effects */
#define HUHDURATION     spread(20)  /* Confusion */
#define MFINDDURATION   spread(20)  /* Monster find */
#define HASTEDURATION   rnd(4)+4    /* Haste */
#define SEEDURATION     spread(850) /* See invisible / blind / hallucinating */
#define LEVITDUR        spread(30)  /* Levitation */
#define SLEEPTIME       spread(7)   /* Sleep */
#define STUCKTIME       spread(3)   /* Stuck */

/** Status getters
 * _t should be a (THING *) */
#define is_blind(_t)         is_status(_t, ISBLIND) /* Is blind */
#define is_cancelled(_t)     is_status(_t, ISCANC) /* Ability cancelled */
#define is_confused(_t)      is_status(_t, ISHUH)   /* Confused */
#define is_confusing(_t)     is_status(_t, CANHUH) /* Attack confuses */
#define is_found(_t)         is_status(_t, ISFOUND) /* Has been seen */
#define is_hallucinating(_t) is_status(_t, ISHALU) /* Is tripping */
#define is_invisible(_t)     is_status(_t, ISINVIS) /* Invisible */
#define is_levitating(_t)    is_status(_t, ISLEVIT) /* Is floating */
#define is_true_seeing(_t)   is_status(_t, CANSEE) /* Can see invisible */

#define is_status(_t, _f)    ((_t)->t_flags & _f)

/** Status setters
 * _t should be a (THING *)
 * _v should be a (bool) */
#define set_blind(_t, _v)          set_status(_t, _v, ISBLIND)
#define set_cancelled(_t, _v)      set_status(_t, _v, ISCANC)
#define set_confused(_t, _v)       set_status(_t, _v, ISHUH)
#define set_confusing(_t, _v)      set_status(_t, _v, CANHUH)
#define set_cursed(_t, _v)         set_status(_t, _v, ISCURSED)
#define set_found(_t, _v)          set_status(_t, _v, ISFOUND)
#define set_hallucinating(_t, _v)  set_status(_t, _v, ISHALU)
#define set_invisible(_t, _v)      set_status(_t, _v, ISINVIS)
#define set_levitating(_t, _v)     set_status(_t, _v, ISLEVIT)
void set_true_seeing(THING *_t, bool _v, bool permanent);

#define set_status(_t, _v, _f) \
  (_t)->t_flags = _v ? (_t)->t_flags | _f : (_t)->t_flags & ~(_f)

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
void teleport(THING *thing, coord *target); /* Teleport someone somewhere */

#endif /* _ROGUE14_STATUS_EFFECTS_H_ */
