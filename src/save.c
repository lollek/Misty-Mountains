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
#include "wizard.h"

#include "save.h"

char save_file_name[MAXSTR];

/* Write the saved game on the file */
static bool
save_file(FILE* savef)
{
  chmod(save_file_name, 0400);

#ifdef NDEBUG
  if (wizard)
  {
    io_msg_clear();
    io_msg("Cannot save as a wizard");
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
  io_msg_clear();
  while (savef == NULL)
  {
    io_msg("save to file? ");
    if (readstr(save_file_name) != 0)
    {
      io_msg_clear();
      return false;
    }
    io_msg_clear();

    /* test to see if the file exists */
    struct stat sbuf;
    if (stat(save_file_name, &sbuf) >= 0)
    {
      int c;
      io_msg("File exists.  Do you wish to overwrite it? ");
      while ((c = readchar(true)) != 'y' && c != 'Y')
      {
        if (c == KEY_ESCAPE || c == 'n' || c == 'N')
        {
          io_msg_clear();
          return false;
        }
      }
      unlink(save_file_name);
    }

    savef = fopen(save_file_name, "w");
    if (savef == NULL)
      io_msg(strerror(errno));
  }

  bool did_save = save_file(savef);
  if (did_save)
  {
    endwin();
    exit(0);
  }
  else
    io_msg("Error while saving");
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
  strcpy(save_file_name, get_homedir());
  strcat(save_file_name, ".rogue14_rescue");
  unlink(save_file_name);
  FILE* savef = fopen(save_file_name, "w");
  if (savef != NULL)
    did_save = save_file(savef);

  endwin();
  if (did_save)
  {
    printf("Autosaved to %s\n", save_file_name);
    exit(0);
  }
  else
  {
    puts("Failed to autosave");
    exit(1);
  }
}

