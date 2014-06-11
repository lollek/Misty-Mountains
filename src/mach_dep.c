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
#include "extern.h"

#define SCOREFILE ".rogue14_highscore"
#define LOCKFILE ".rogue14_lockfile"

# ifndef NUMSCORES
#	define	NUMSCORES	10
#	define	NUMNAME		"Ten"
# endif

unsigned int numscores = NUMSCORES;
char *Numname = NUMNAME;

/*
 * open_score:
 *	Open up the score file for future use
 */

void
open_score_and_drop_setuid_setgid()
{
  /* FIXME: highscore should NOT be in the local folder */
  char *scorefile = SCOREFILE;

  /* We drop setgid privileges after opening the score file, so subsequent
   * open()'s will fail.  Just reuse the earlier filehandle. */

  if (scoreboard != NULL) {
    rewind(scoreboard);
    md_normaluser();
    return;
  }

  scoreboard = fopen(scorefile, "r+");

  if ((scoreboard == NULL) && (errno == ENOENT))
  {
    scoreboard = fopen(scorefile, "w+");
    chmod(scorefile,0664);
  }

  if (scoreboard == NULL) {
    fprintf(stderr, "Could not open %s for writing: %s\n",
            scorefile, strerror(errno));
    fflush(stderr);
  }

  /* Drop setuid/setgid after opening the scoreboard file.  */
  md_normaluser();
}

/*
 * is_symlink:
 *      See if the file has a symbolic link
  */
bool
is_symlink(char *sp)
{
    struct stat sbuf2;

    if (lstat(sp, &sbuf2) < 0)
        return FALSE;
    else
        return ((sbuf2.st_mode & S_IFMT) != S_IFREG);
}

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
