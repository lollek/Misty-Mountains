#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>

#include "state.h"
#include "io.h"
#include "os.h"

#include "score.h"

#define LOCKFILE ".rogue14_lockfile"

static FILE *scoreboard = NULL; /* File descriptor for score file */
static FILE *lock = NULL;
static char scoreline[100];

static bool
lock_sc(void)
{
  int cnt;
  static struct stat sbuf;

  lock = fopen(LOCKFILE, "w+");
  if (lock != NULL)
    return true;

  for (cnt = 0; cnt < 5; cnt++)
  {
    sleep(1);
    lock = fopen(LOCKFILE, "w+");
    if (lock != NULL)
      return true;
  }

  if (stat(LOCKFILE, &sbuf) < 0)
  {
    lock = fopen(LOCKFILE, "w+");
    return true;
  }

  if (time(NULL) - sbuf.st_mtime > 10)
    return unlink(LOCKFILE) < 0
      ? false
      : lock_sc();

  printf("The score file is very busy.  Do you want to wait longer\n"
         "for it to become free so your score can get posted?\n"
         "If so, type \"y\"\n");

  return readchar(true) == 'y'
    ? lock_sc()
    : false;
}


static void
unlock_sc(void)
{
  if (lock != NULL)
    fclose(lock);
  lock = NULL;
  unlink(LOCKFILE);
}

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

void
score_read(SCORE *top_ten)
{
  unsigned int i;

  if (scoreboard == NULL)
    return;

  if (!lock_sc())
    return;

  rewind(scoreboard);

  for(i = 0; i < NUMSCORES; i++)
  {
    encread(top_ten[i].sc_name, MAXSTR, scoreboard);
    encread(scoreline, 100, scoreboard);
    sscanf(scoreline, " %u %d %d %d %d %x \n",
        &top_ten[i].sc_uid, &top_ten[i].sc_score,
        &top_ten[i].sc_flags, &top_ten[i].sc_monster,
        &top_ten[i].sc_level, &top_ten[i].sc_time);
  }

  rewind(scoreboard);

  unlock_sc();
}

void
score_write(SCORE *top_ten)
{
  unsigned int i;

  if (scoreboard == NULL)
    return;

  if (!lock_sc())
    return;

  rewind(scoreboard);

  for(i = 0; i < NUMSCORES; i++)
  {
    memset(scoreline,0,100);
    encwrite(top_ten[i].sc_name, MAXSTR, scoreboard);
    sprintf(scoreline, " %u %d %d %d %d %x \n",
        top_ten[i].sc_uid, top_ten[i].sc_score,
        top_ten[i].sc_flags, top_ten[i].sc_monster,
        top_ten[i].sc_level, top_ten[i].sc_time);
    encwrite(scoreline,100,scoreboard);
  }

  rewind(scoreboard);

  unlock_sc();
}

