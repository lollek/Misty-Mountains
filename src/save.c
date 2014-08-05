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
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include "rogue.h"
#include "score.h"
#include "options.h"
#include "io.h"

static FILE *scoreboard = NULL; /* File descriptor for score file */

static const char encstr[] = "\300k||`\251Y.'\305\321\201+\277~r\"]\240_\223=1\341)\222\212\241t;\t$\270\314/<#\201\254";
static const char statlist[] = "\355kl{+\204\255\313idJ\361\214=4:\311\271\341wK<\312\321\213,,7\271/Rk%\b\312\f\246";

/** open_score_and_drop_setuid_setgid:
 * Open up the score file for future use
 * We drop setgid privileges after opening the score file, so subsequent
 * open()'s will fail.  Just reuse the earlier filehandle. */
int
open_score_and_drop_setuid_setgid(void)
{
  /* NB: SCOREDIR comes from Makefile */
  const char *scorefile = SCOREDIR "rogue14.highscore";

  /* Open file */
  scoreboard = fopen(scorefile, "r+");

  if (scoreboard == NULL && errno == ENOENT) {
    scoreboard = fopen(scorefile, "w+");
    if (scoreboard != NULL) {
      chmod(scorefile, 0664);
      chown(scorefile, geteuid(), getegid());
    }
  }

  if (scoreboard == NULL) {
    fprintf(stderr, "Could not open %s for writing: %s\n",
            scorefile, strerror(errno));
    fflush(stderr);
    return 1;
  }

  /* Drop setuid/setgid */
  {
    gid_t realgid = getgid();
    uid_t realuid = getuid();

    if (setregid(realgid, realgid) != 0) {
      perror("Could not drop setgid privileges.  Aborting.");
      return 1;
    }
    if (setreuid(realuid, realuid) != 0) {
      perror("Could not drop setuid privileges.  Aborting.");
      return 1;
    }
  }
  return 0;
}


/** save_game:
 * Implement the "save game" command */
bool
save_game(void)
{
  FILE *savef = NULL;
  bool did_save = false;

  after = false; /* This does not count as a move */
  mpos = 0;

  while (savef == NULL)
  {
    struct stat sbuf;
    msg("save to file? ");
    if (readstr(file_name) != 0)
    {
      msg("");
      return false;
    }
    mpos = 0;

    /* test to see if the file exists */
    if (stat(file_name, &sbuf) >= 0)
    {
      int c;
      msg("File exists.  Do you wish to overwrite it? ");
      while ((c = readchar()) != 'y' && c != 'Y')
      {
        if (c == KEY_ESCAPE || c == 'n' || c == 'N')
        {
          msg("");
          return false;
        }
      }
      unlink(file_name);
    }

    if ((savef = fopen(file_name, "w")) == NULL)
      msg(strerror(errno));
  }

  did_save = save_file(savef);
  if (did_save)
  {
    endwin();
    exit(0);
  }
  else
    msg("Error while saving");
  return false;
}

/** auto_save:
 * Automatically save a file.  This is used if a HUP signal is recieved */
void
auto_save(int sig)
{
    FILE *savef;
    bool did_save = false;
    int i;

    (void)sig;

    /* Ignore all signals */
    for (i = 0; i < NSIG; i++)
      signal(i, SIG_IGN);

    /* Always auto-save to ~/.rogue14_rescue */
    strcpy(stpcpy(file_name, get_homedir()), ".rogue14_rescue");
    unlink(file_name);
    if ((savef = fopen(file_name, "w")) != NULL)
      did_save = save_file(savef);

    endwin();
    if (did_save)
    {
      printf("Autosaved to %s\n", file_name);
      exit(0);
    }
    else
    {
      puts("Failed to autosave");
      exit(1);
    }
}

/*
 * save_file:
 *	Write the saved game on the file
 */

bool
save_file(FILE *savef)
{
    int error = 0;
    char buf[80];

    chmod(file_name, 0400);

    if (wizard || potential_wizard)
    {
      mpos = 0;
      msg("Cannot save as a wizard");
      return false;
    }

    encwrite(game_version, strlen(game_version)+1, savef);
    sprintf(buf,"%d x %d\n", LINES, COLS);
    encwrite(buf,80,savef);
    error = rs_save_file(savef);

    fflush(savef);
    fclose(savef);
    return error == 0;
}


/*
 * encwrite:
 *	Perform an encrypted write
 */

size_t
encwrite(const char *start, size_t size, FILE *outf)
{
    const char *e1 = encstr;
    const char *e2 = statlist;
    char fb = 0;
    size_t i;

    for (i = size; i > 0; --i)
    {
	if (putc(*start++ ^ *e1 ^ *e2 ^ fb, outf) == EOF)
            break;

	fb += *e1++ * *e2++;
	if (*e1 == '\0')
	    e1 = encstr;
	if (*e2 == '\0')
	    e2 = statlist;
    }

    return(size - i);
}

/*
 * encread:
 *	Perform an encrypted read
 */
size_t
encread(char *start, size_t size, FILE *inf)
{
    const char *e1 = encstr;
    const char *e2 = statlist;
    char fb = 0;
    size_t read_size;

    if ((read_size = fread(start,1,size,inf)) == 0)
	return(read_size);

    while (size--)
    {
	*start++ ^= *e1 ^ *e2 ^ fb;
	fb += *e1++ * *e2++;
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

	for(i = 0; i < NUMSCORES; i++)
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

    for(i = 0; i < NUMSCORES; i++)
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

