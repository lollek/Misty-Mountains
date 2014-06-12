/*
 * save and restore routines
 *
 * @(#)save.c	4.33 (Berkeley) 06/01/83
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <curses.h>
#include "rogue.h"
#include "score.h"

/* FIXME: highscore should NOT be in the local folder */
#define SCOREFILE ".rogue14_highscore"

static FILE *scoreboard = NULL; /* File descriptor for score file */

typedef struct stat STAT;

extern char version[], encstr[];

static STAT sbuf;


/** open_score_and_drop_setuid_setgid:
 * Open up the score file for future use */
void
open_score_and_drop_setuid_setgid()
{
  /* We drop setgid privileges after opening the score file, so subsequent
   * open()'s will fail.  Just reuse the earlier filehandle. */

  scoreboard = fopen(SCOREFILE, "r+");

  if (scoreboard == NULL && errno == ENOENT)
  {
    scoreboard = fopen(SCOREFILE, "w+");
    chmod(SCOREFILE, 0664);
  }

  if (scoreboard == NULL) {
    fprintf(stderr, "Could not open %s for writing: %s\n",
            SCOREFILE, strerror(errno));
    fflush(stderr);
  }

  /* Drop setuid/setgid */
  {
    gid_t realgid = getgid();
    uid_t realuid = getuid();

    if (setregid(realgid, realgid) != 0) {
      perror("Could not drop setgid privileges.  Aborting.");
      exit(1);
    }
    if (setreuid(realuid, realuid) != 0) {
      perror("Could not drop setuid privileges.  Aborting.");
      exit(1);
    }
  }
}


/*
 * save_game:
 *	Implement the "save game" command
 */

void
save_game()
{
    FILE *savef;
    int c;
    auto char buf[MAXSTR];

    /*
     * get file name
     */
    mpos = 0;
over:
    if (file_name[0] != '\0')
    {
	for (;;)
	{
	    msg("save file (%s)? ", file_name);
	    c = readchar();
	    mpos = 0;
	    if (c == ESCAPE)
	    {
		msg("");
		return;
	    }
	    else if (c == 'n' || c == 'N' || c == 'y' || c == 'Y')
		break;
	    else
		msg("please answer Y or N");
	}
	if (c == 'y' || c == 'Y')
	{
	    addstr("Yes\n");
	    refresh();
	    strcpy(buf, file_name);
	    goto gotfile;
	}
    }

    do
    {
	mpos = 0;
	msg("file name: ");
	buf[0] = '\0';
	if (get_str(buf, stdscr) == QUIT)
	{
quit_it:
	    msg("");
	    return;
	}
	mpos = 0;
gotfile:
	/*
	 * test to see if the file exists
	 */
	if (stat(buf, &sbuf) >= 0)
	{
	    for (;;)
	    {
		msg("File exists.  Do you wish to overwrite it?");
		mpos = 0;
		if ((c = readchar()) == ESCAPE)
		    goto quit_it;
		if (c == 'y' || c == 'Y')
		    break;
		else if (c == 'n' || c == 'N')
		    goto over;
		else
		    msg("Please answer Y or N");
	    }
	    msg("file name: %s", buf);
	    unlink(file_name);
	}
	strcpy(file_name, buf);
	if ((savef = fopen(file_name, "w")) == NULL)
	    msg(strerror(errno));
    } while (savef == NULL);

    save_file(savef);
    /* NOTREACHED */
}

/*
 * auto_save:
 *	Automatically save a file.  This is used if a HUP signal is
 *	recieved
 */

void
auto_save(int sig)
{
    FILE *savef;
    int i;

    /* Ignore all signals */
    for (i = 0; i < NSIG; i++)
      signal(i, SIG_IGN);

    NOOP(sig);

    if (file_name[0] != '\0' && ((savef = fopen(file_name, "w")) != NULL ||
	(unlink(file_name) >= 0 && 
         (savef = fopen(file_name, "w")) != NULL)))
	    save_file(savef);
    exit(0);
}

/*
 * save_file:
 *	Write the saved game on the file
 */

void
save_file(FILE *savef)
{
    char buf[80];
    mvcur(0, COLS - 1, LINES - 1, 0); 
    putchar('\n');
    endwin();
    chmod(file_name, 0400);
    encwrite(version, strlen(version)+1, savef);
    sprintf(buf,"%d x %d\n", LINES, COLS);
    encwrite(buf,80,savef);
    rs_save_file(savef);
    fflush(savef);
    fclose(savef);
    exit(0);
}

/*
 * restore:
 *	Restore a saved game from a file with elaborate checks for file
 *	integrity from cheaters
 */
bool
restore(char *file)
{
    FILE *inf;
    char buf[MAXSTR];

    if (strcmp(file, "-r") == 0)
        file = file_name;

    if ((inf = fopen(file,"r")) == NULL)
    {
        perror(file);
        return FALSE;
    }

    /* defeat multiple restarting from the same place
     * TODO: should these be removed? not hard to circumvent anyways */
    if (!wizard)
    {
      struct stat sbuf2;
      if (lstat(file, &sbuf2) == -1)
      {
        perror(file);
        return FALSE;
      }
      if ((sbuf2.st_mode & S_IFMT) != S_IFREG)
      {
        printf("Only normal files allowed (no symlinks, FIFOs, etc)\n");
        return FALSE;
      }
      if (sbuf2.st_nlink != 1)
      {
        printf("The savegame cannot be hardlinked, since that's cheating\n");
        return FALSE;
      }
    }

    fflush(stdout);
    encread(buf, (unsigned) strlen(version) + 1, inf);
    if (strcmp(buf, version) != 0)
    {
        printf("Sorry, saved game is out of date.\n");
        return FALSE;
    }
    encread(buf,80,inf);

    if (init_graphics() != 0)
      return FALSE;

    rs_restore_file(inf);
    /*
     * we do not close the file so that we will have a hold of the
     * inode for as long as possible
     */

    if (!wizard && unlink(file) < 0)
    {
        endwin();
        printf("Cannot unlink file\n");
        return FALSE;
    }
    mpos = 0;
    clearok(stdscr,TRUE);

    if (pstats.s_hpt <= 0)
    {
	endwin();
	printf("\n\"He's dead, Jim\"\n");
	return FALSE;
    }

    /* parse environment declaration of options */
    parse_opts(getenv("ROGUEOPTS"));

    strcpy(file_name, file);
    clearok(curscr, TRUE);
    msg("file name: %s", file);
    return playit();
}

/*
 * encwrite:
 *	Perform an encrypted write
 */

size_t
encwrite(char *start, size_t size, FILE *outf)
{
    char *e1, *e2, fb;
    int temp;
    extern char statlist[];
    size_t o_size = size;
    e1 = encstr;
    e2 = statlist;
    fb = 0;

    while(size)
    {
	if (putc(*start++ ^ *e1 ^ *e2 ^ fb, outf) == EOF)
            break;

	temp = *e1++;
	fb = fb + ((char) (temp * *e2++));
	if (*e1 == '\0')
	    e1 = encstr;
	if (*e2 == '\0')
	    e2 = statlist;
	size--;
    }

    return(o_size - size);
}

/*
 * encread:
 *	Perform an encrypted read
 */
size_t
encread(char *start, size_t size, FILE *inf)
{
    char *e1, *e2, fb;
    int temp;
    size_t read_size;
    extern char statlist[];

    fb = 0;

    if ((read_size = fread(start,1,size,inf)) == 0)
	return(read_size);

    e1 = encstr;
    e2 = statlist;

    while (size--)
    {
	*start++ ^= *e1 ^ *e2 ^ fb;
	temp = *e1++;
	fb = fb + (char)(temp * *e2++);
	if (*e1 == '\0')
	    e1 = encstr;
	if (*e2 == '\0')
	    e2 = statlist;
    }

    return(read_size);
}

static char scoreline[100];
/*
 * read_scrore
 *	Read in the score file
 */
void
rd_score(SCORE *top_ten)
{
    unsigned int i;

	if (scoreboard == NULL)
		return;

	rewind(scoreboard); 

	for(i = 0; i < numscores; i++)
    {
        encread(top_ten[i].sc_name, MAXSTR, scoreboard);
        encread(scoreline, 100, scoreboard);
        sscanf(scoreline, " %u %d %u %hu %d %x \n",
            &top_ten[i].sc_uid, &top_ten[i].sc_score,
            &top_ten[i].sc_flags, &top_ten[i].sc_monster,
            &top_ten[i].sc_level, &top_ten[i].sc_time);
    }

	rewind(scoreboard); 
}

/*
 * write_scrore
 *	Read in the score file
 */
void
wr_score(SCORE *top_ten)
{
    unsigned int i;

	if (scoreboard == NULL)
		return;

	rewind(scoreboard);

    for(i = 0; i < numscores; i++)
    {
          memset(scoreline,0,100);
          encwrite(top_ten[i].sc_name, MAXSTR, scoreboard);
          sprintf(scoreline, " %u %d %u %hu %d %x \n",
              top_ten[i].sc_uid, top_ten[i].sc_score,
              top_ten[i].sc_flags, top_ten[i].sc_monster,
              top_ten[i].sc_level, top_ten[i].sc_time);
          encwrite(scoreline,100,scoreboard);
    }

	rewind(scoreboard); 
}
