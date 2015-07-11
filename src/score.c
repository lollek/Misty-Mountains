#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "state.h"
#include "io.h"
#include "os.h"
#include "wizard.h"
#include "rogue.h"
#include "level.h"
#include "death.h"
#include "pack.h"

#include "score.h"

#define LOCKFILE ".rogue14_lockfile"

struct score {
  unsigned uid;
  int      score;
  int      flags;
  int      monster;
  char     name[MAXSTR];
  int      level;
  unsigned time;
};

static FILE* scoreboard = NULL; /* File descriptor for score file */
static FILE* lock = NULL;

static bool
lock_sc(void)
{
  lock = fopen(LOCKFILE, "w+");
  if (lock != NULL)
    return true;

  for (int cnt = 0; cnt < 5; cnt++)
  {
    sleep(1);
    lock = fopen(LOCKFILE, "w+");
    if (lock != NULL)
      return true;
  }

  struct stat sbuf;
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

  return io_readchar(true) == 'y'
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

static void
score_read(struct score* top_ten)
{
  if (scoreboard == NULL || !lock_sc())
    return;

  rewind(scoreboard);

  for (unsigned i = 0; i < SCORE_MAX; i++)
  {
    char buf[100];
    io_encread(top_ten[i].name, MAXSTR, scoreboard);
    io_encread(buf, sizeof(buf), scoreboard);
    sscanf(buf, " %u %d %d %d %d %x \n",
        &top_ten[i].uid, &top_ten[i].score,
        &top_ten[i].flags, &top_ten[i].monster,
        &top_ten[i].level, &top_ten[i].time);
  }

  rewind(scoreboard);

  unlock_sc();
}

static void
score_write(struct score* top_ten)
{
  if (scoreboard == NULL || !lock_sc())
    return;

  rewind(scoreboard);

  for(unsigned i = 0; i < SCORE_MAX; i++)
  {
    char buf[100];
    io_encwrite(top_ten[i].name, MAXSTR, scoreboard);
    memset(buf, '\0', sizeof(buf));
    sprintf(buf, " %u %d %d %d %d %x \n",
        top_ten[i].uid, top_ten[i].score,
        top_ten[i].flags, top_ten[i].monster,
        top_ten[i].level, top_ten[i].time);
    io_encwrite(buf, sizeof(buf), scoreboard);
  }

  rewind(scoreboard);

  unlock_sc();
}



static void
score_insert(struct score* top_ten, int amount, int flags, char monst)
{
  unsigned uid = getuid();
  for (unsigned i = 0; i < SCORE_MAX; ++i)
    if (amount > top_ten[i].score)
    {
      /* Move all scores a step down */
      size_t scores_to_move = SCORE_MAX - i - 1;
      memmove(&top_ten[i +1], &top_ten[i], sizeof(*top_ten) * scores_to_move);

      /* Add new scores */
      top_ten[i].score = amount;
      strcpy(top_ten[i].name, whoami);
      top_ten[i].flags = flags;
      top_ten[i].level = flags == 2
        ? level_max
        : level;
      top_ten[i].monster = monst;
      top_ten[i].uid = uid;

      /* Write score to disk */
      score_write(top_ten);
    }
}

static void
score_print(struct score* top_ten)
{
  char buf[2*MAXSTR];

  printf("Top %d %s:\n   Score Name\n", SCORE_MAX, "Scores");
  for (unsigned i = 0; i < SCORE_MAX; ++i)
  {
    if (!top_ten[i].score)
      break;

    printf("%2d %5d %s: "
        ,i + 1                   /* Position */
        ,top_ten[i].score     /* Score */
        ,top_ten[i].name      /* Name */
        );

    if (top_ten[i].flags == 0)
      printf("%s", death_reason(buf, top_ten[i].monster));
    else if (top_ten[i].flags == 1)
      printf("Quit");
    else if (top_ten[i].flags == 2)
      printf("A total winner");
    else if (top_ten[i].flags == 3)
      printf("%s while holding the amulet",
          death_reason(buf, top_ten[i].monster));

    printf(" on level %d.\n", top_ten[i].level);
  }
}

int
score_open_and_drop_setuid_setgid(void)
{
  /* Open file */
  scoreboard = fopen(SCOREPATH, "r+");

  if (scoreboard == NULL && errno == ENOENT) {
    scoreboard = fopen(SCOREPATH, "w+");
    if (scoreboard != NULL) {
      chmod(SCOREPATH, 0664);
      chown(SCOREPATH, geteuid(), getegid());
    }
  }

  if (scoreboard == NULL) {
    fprintf(stderr, "Could not open %s for writing: %s\n",
            SCOREPATH, strerror(errno));
    fflush(stderr);
    return 1;
  }

  if (os_drop_gid() != 0) {
    perror("Could not drop group privileges.  Aborting.");
    abort();
  }
  if (os_drop_uid() != 0) {
    perror("Could not drop user privileges.  Aborting.");
    abort();
  }
  return 0;
}

void
score_show_and_exit(int amount, int flags, char monst)
{
  char buf[2*MAXSTR];

  if (flags >= 0 || wizard)
  {
    mvaddstr(LINES - 1, 0 , "[Press return to continue]");
    refresh();
    wgetnstr(stdscr, buf, 80);
    endwin();
    putchar('\n');
  }

  struct score top_ten[SCORE_MAX];
  memset(top_ten, 0, SCORE_MAX * sizeof(*top_ten));
  score_read(top_ten);

  /* Insert her in list if need be */
  score_insert(top_ten, amount, flags, monst);

  /* Print the highscore */
  score_print(top_ten);

  exit(0);
}

void
score_win_and_exit(void)
{
  clear();
  addstr(
    "                                                               \n"
    "  @   @               @   @           @          @@@  @     @  \n"
    "  @   @               @@ @@           @           @   @     @  \n"
    "  @   @  @@@  @   @   @ @ @  @@@   @@@@  @@@      @  @@@    @  \n"
    "   @@@@ @   @ @   @   @   @     @ @   @ @   @     @   @     @  \n"
    "      @ @   @ @   @   @   @  @@@@ @   @ @@@@@     @   @     @  \n"
    "  @   @ @   @ @  @@   @   @ @   @ @   @ @         @   @  @     \n"
    "   @@@   @@@   @@ @   @   @  @@@@  @@@@  @@@     @@@   @@   @  \n"
    "                                                               \n"
    "     Congratulations, you have made it to the light of day!    \n"
    "\n"
    "You have joined the elite ranks of those who have escaped the\n"
    "Dungeons of Doom alive.  You journey home and sell all your loot at\n"
    "a great profit and are admitted to the Fighters' Guild.\n"
    );

  mvaddstr(LINES - 1, 0, "--Press space to continue--");
  refresh();
  io_wait_for_key(KEY_SPACE);
  pack_gold += pack_evaluate();
  score_show_and_exit(pack_gold, 2, ' ');
}


