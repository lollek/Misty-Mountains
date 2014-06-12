/*
 * Various installation dependent routines
 *
 * @(#)mach_dep.c	4.37 (Berkeley) 05/23/83
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

/*
 * The various tuneable defines are:
 *
 *	NUMSCORES	Number of scores in the score file (default 10).
 *	NUMNAME		String version of NUMSCORES (first character
 *			should be capitalized) (default "Ten").
 */

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <curses.h>
#include "rogue.h"

#define LOCKFILE ".rogue14_lockfile"

# ifndef NUMSCORES
#	define	NUMSCORES	10
#	define	NUMNAME		"Ten"
# endif

unsigned int numscores = NUMSCORES;
char *Numname = NUMNAME;

/*
 * lock_sc:
 *	lock the score file.  If it takes too long, ask the user if
 *	they care to wait.  Return TRUE if the lock is successful.
 */
static FILE *lfd = NULL;
bool
lock_sc()
{
    int cnt;
    static struct stat sbuf;
    char *lockfile = LOCKFILE;

over:
    if ((lfd=fopen(lockfile, "w+")) != NULL)
	return TRUE;
    for (cnt = 0; cnt < 5; cnt++)
    {
	sleep(1);
	if ((lfd=fopen(lockfile, "w+")) != NULL)
	    return TRUE;
    }
    if (stat(lockfile, &sbuf) < 0)
    {
	lfd=fopen(lockfile, "w+");
	return TRUE;
    }
    if (time(NULL) - sbuf.st_mtime > 10)
    {
	if (unlink(lockfile) < 0)
	    return FALSE;
	goto over;
    }
    else
    {
	printf("The score file is very busy.  Do you want to wait longer\n");
	printf("for it to become free so your score can get posted?\n");
	printf("If so, type \"y\"\n");
	(void) fgets(prbuf, MAXSTR, stdin);
	if (prbuf[0] == 'y')
	    for (;;)
	    {
		if ((lfd=fopen(lockfile, "w+")) != 0)
		    return TRUE;
		if (stat(lockfile, &sbuf) < 0)
		{
		    lfd=fopen(lockfile, "w+");
		    return TRUE;
		}
		if (time(NULL) - sbuf.st_mtime > 10)
		{
		    if (unlink(lockfile) < 0)
			return FALSE;
		}
		sleep(1);
	    }
	else
	    return FALSE;
    }
}

/*
 * unlock_sc:
 *	Unlock the score file
 */

void
unlock_sc()
{
    if (lfd != NULL)
        fclose(lfd);
    lfd = NULL;
    unlink(LOCKFILE);
}

/*
 * flush_type:
 *	Flush typeahead for traps, etc.
 */

void
flush_type()
{
    flushinp();
}
