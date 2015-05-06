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
#include "rogue.h"

#include "save.h"

static const char encstr[] = "\300k||`\251Y.'\305\321\201+\277~r\"]\240_\223=1\341)\222\212\241t;\t$\270\314/<#\201\254";
static const char statlist[] = "\355kl{+\204\255\313idJ\361\214=4:\311\271\341wK<\312\321\213,,7\271/Rk%\b\312\f\246";

/* Write the saved game on the file */
static bool
save_file(FILE *savef)
{
  int error = 0;
  char buf[80];

  chmod(file_name, 0400);

#ifdef NDEBUG
  if (wizard || potential_wizard)
  {
    mpos = 0;
    msg("Cannot save as a wizard");
    return false;
  }
#endif

  encwrite(GAME_VERSION, strlen(GAME_VERSION)+1, savef);
  sprintf(buf,"%d x %d\n", LINES, COLS);
  encwrite(buf,80,savef);
  error = rs_save_file(savef);

  fflush(savef);
  fclose(savef);
  return error == 0;
}

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

void
auto_save(int sig)
{
  FILE *savef;
  bool did_save = false;

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

  /* Always auto-save to ~/.rogue14_rescue */
  strcpy(file_name, get_homedir());
  strcat(file_name, ".rogue14_rescue");
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
