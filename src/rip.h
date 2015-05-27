#ifndef _ROGUE14_RIP_H_
#define _ROGUE14_RIP_H_

/* Figure score and post it.  */
void score(int amount, int flags, char monst);

/* Do something really fun when he dies */
void death(char monst) __attribute__((noreturn));

/* Code for a winner */
void total_winner(void) __attribute__((noreturn));

#endif /* _ROGUE14_RIP_H_ */
