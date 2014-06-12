/*
 * Defines for things used in mach_dep.c
 *
 * @(#)extern.h 4.35 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

/*
 * Don't change the constants, since they are used for sizes in many
 * places in the program.
 */

#include <stdlib.h>

#define MAXSTR 1024 /* maximum length of strings */
#define MAXLINES 32 /* maximum number of screen lines used */
#define MAXCOLS  80 /* maximum number of screen columns used */

#undef CTRL
#define CTRL(c) (c & 037)

/* Now all the global variables */

extern int potential_wizard;
extern int wizard;
extern char prbuf[], whoami[];

/* Function types */

void come_down();
void doctor();
void end_line();
void land();
void leave(int);
void nohaste();
void print_disc(char);
void quit(int);
void rollwand();
void runners();
void sight();
void stomach();
void swander();
void unconfuse();
void unsee();
void visuals();

char add_line(char *fmt, char *arg);

char *md_getusername();
int md_hasclreol();

