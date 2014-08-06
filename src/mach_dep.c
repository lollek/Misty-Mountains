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

#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include "rogue.h"

#define LOCKFILE ".rogue14_lockfile"

/*
 * lock_sc:
 *	lock the score file.  If it takes too long, ask the user if
 *	they care to wait.  Return true if the lock is successful.
 */
static FILE *lfd = NULL;
bool
lock_sc(void)
{
    int cnt;
    static struct stat sbuf;
    char *lockfile = LOCKFILE;

over:
    if ((lfd=fopen(lockfile, "w+")) != NULL)
	return true;
    for (cnt = 0; cnt < 5; cnt++)
    {
	sleep(1);
	if ((lfd=fopen(lockfile, "w+")) != NULL)
	    return true;
    }
    if (stat(lockfile, &sbuf) < 0)
    {
	lfd=fopen(lockfile, "w+");
	return true;
    }
    if (time(NULL) - sbuf.st_mtime > 10)
    {
	if (unlink(lockfile) < 0)
	    return false;
	goto over;
    }
    else
    {
	char tmpbuf[MAXSTR];
	printf("The score file is very busy.  Do you want to wait longer\n");
	printf("for it to become free so your score can get posted?\n");
	printf("If so, type \"y\"\n");
	(void) fgets(tmpbuf, MAXSTR, stdin);
	if (tmpbuf[0] == 'y')
	    for (;;)
	    {
		if ((lfd=fopen(lockfile, "w+")) != 0)
		    return true;
		if (stat(lockfile, &sbuf) < 0)
		{
		    lfd=fopen(lockfile, "w+");
		    return true;
		}
		if (time(NULL) - sbuf.st_mtime > 10)
		{
		    if (unlink(lockfile) < 0)
			return false;
		}
		sleep(1);
	    }
	else
	    return false;
    }
}

/*
 * unlock_sc:
 *	Unlock the score file
 */

void
unlock_sc(void)
{
    if (lfd != NULL)
        fclose(lfd);
    lfd = NULL;
    unlink(LOCKFILE);
}
