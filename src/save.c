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
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "score.h"
#include "options.h"
#include "io.h"
#include "state.h"
#include "rogue.h"

#include "save.h"

/* Write the saved game on the file */
static bool
save_file(FILE* savef)
{
  chmod(file_name, 0400);

#ifdef NDEBUG
  if (wizard)
  {
    mpos = 0;
    msg("Cannot save as a wizard");
    return false;
  }
#endif

  int error = state_save_file(savef);

  fflush(savef);
  fclose(savef);
  return error == 0;
}

bool
save_game(void)
{
  FILE *savef = NULL;
  mpos = 0;
  while (savef == NULL)
  {
    msg("save to file? ");
    if (readstr(file_name) != 0)
    {
      clearmsg();
      return false;
    }
    mpos = 0;

    /* test to see if the file exists */
    struct stat sbuf;
    if (stat(file_name, &sbuf) >= 0)
    {
      int c;
      msg("File exists.  Do you wish to overwrite it? ");
      while ((c = readchar(true)) != 'y' && c != 'Y')
      {
        if (c == KEY_ESCAPE || c == 'n' || c == 'N')
        {
          clearmsg();
          return false;
        }
      }
      unlink(file_name);
    }

    savef = fopen(file_name, "w");
    if (savef == NULL)
      msg(strerror(errno));
  }

  bool did_save = save_file(savef);
  if (did_save)
  {
    endwin();
    exit(0);
  }
  else
    msg("Error while saving");
  return false;
}

void
save_auto(int sig)
{
  (void)sig;

  /* Ignore all signals that might have sent us here */
  signal(SIGHUP, SIG_IGN);
  signal(SIGILL, SIG_IGN);
  signal(SIGTRAP, SIG_IGN);
  signal(SIGIOT, SIG_IGN);
  signal(SIGFPE, SIG_IGN);
  signal(SIGBUS, SIG_IGN);
  signal(SIGSEGV, SIG_IGN);
  signal(SIGSYS, SIG_IGN);
  signal(SIGTERM, SIG_IGN);

  bool did_save = false;

  /* Always auto-save to ~/.rogue14_rescue */
  strcpy(file_name, get_homedir());
  strcat(file_name, ".rogue14_rescue");
  unlink(file_name);
  FILE* savef = fopen(file_name, "w");
  if (savef != NULL)
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

