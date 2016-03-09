#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "misty_mountains.h"
#include "game.h"
#include "io.h"
#include "os.h"
#include "wizard.h"
#include "rogue.h"
#include "level.h"
#include "death.h"
#include "player.h"

#include "score.h"
#include "score/encio.h"

#define LOCKFILE ".rogue14_lockfile"

#define MAXSTR 1024 // maximum length of strings

struct score {
  unsigned uid;
  int      score;
  int      flags;
  int      death_type;
  char     name[MAXSTR];
  int      level;
  unsigned time;
};

static FILE* scoreboard = nullptr; /* File descriptor for score file */
static FILE* lock = nullptr;

static bool
lock_sc(void)
{
  lock = fopen(LOCKFILE, "w+");
  if (lock != nullptr)
    return true;

  for (int cnt = 0; cnt < 5; cnt++)
  {
    sleep(1);
    lock = fopen(LOCKFILE, "w+");
    if (lock != nullptr)
      return true;
  }

  struct stat sbuf;
  if (stat(LOCKFILE, &sbuf) < 0)
  {
    lock = fopen(LOCKFILE, "w+");
    return true;
  }

  if (time(nullptr) - sbuf.st_mtime > 10)
    return unlink(LOCKFILE) < 0
      ? false
      : lock_sc();

  printf("The score file is very busy.  Do you want to wait longer\n"
         "for it to become free so your score can get posted?\n"
         "If so, type \"y\"\n");

  return Game::io->readchar(true) == 'y'
    ? lock_sc()
    : false;
}


static void
unlock_sc(void)
{
  if (lock != nullptr)
    fclose(lock);
  lock = nullptr;
  unlink(LOCKFILE);
}

static void
score_read(struct score* top_ten)
{
  if (scoreboard == nullptr || !lock_sc())
    return;

  rewind(scoreboard);

  for (unsigned i = 0; i < SCORE_MAX; i++)
  {
    char buf[100];
    encread(top_ten[i].name, MAXSTR, scoreboard);
    encread(buf, sizeof(buf), scoreboard);
    sscanf(buf, " %u %d %d %d %d %x \n",
        &top_ten[i].uid, &top_ten[i].score,
        &top_ten[i].flags, &top_ten[i].death_type,
        &top_ten[i].level, &top_ten[i].time);
  }

  rewind(scoreboard);

  unlock_sc();
}

static void
score_write(struct score* top_ten)
{
  if (scoreboard == nullptr || !lock_sc())
    return;

  rewind(scoreboard);

  for(unsigned i = 0; i < SCORE_MAX; i++)
  {
    char buf[100];
    encwrite(top_ten[i].name, MAXSTR, scoreboard);
    memset(buf, '\0', sizeof(buf));
    sprintf(buf, " %u %d %d %d %d %x \n",
        top_ten[i].uid, top_ten[i].score,
        top_ten[i].flags, top_ten[i].death_type,
        top_ten[i].level, top_ten[i].time);
    encwrite(buf, sizeof(buf), scoreboard);
  }

  rewind(scoreboard);

  unlock_sc();
}



static void
score_insert(struct score* top_ten, int amount, int death_type)
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
      strcpy(top_ten[i].name, Game::whoami->c_str());
      top_ten[i].flags = 0;
      top_ten[i].level = Game::current_level;
      top_ten[i].death_type = death_type;
      top_ten[i].uid = uid;

      /* Write score to disk */
      score_write(top_ten);
      break;
    }
}

static void
score_print(struct score* top_ten)
{
  Game::io->stop_curses();
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

    printf("%s on level %d.\n", death_reason(top_ten[i].death_type).c_str(),
                                top_ten[i].level);
  }
}

int
score_open(void)
{
  scoreboard = fopen(SCOREPATH, "r+");
  if (scoreboard == nullptr) {
    fprintf(stderr, "Could not open %s for writing: %s\n"
                    "Your highscore will not be saved if you die!\n"
                    "[Press return key to continue]",
            SCOREPATH, strerror(errno));
    getchar();
    return 1;
  }
  return 0;
}

void
score_show_and_exit(int death_type)
{
  int amount = 0;

  if (player != nullptr) {
    amount = static_cast<int>(player->pack_print_value());

    Game::io->print_string(0, IO::map_height -1, "[Press return to continue]");
    Game::io->force_redraw();
    Game::io->readchar(true);
    putchar('\n');
  }

  struct score top_ten[SCORE_MAX];
  memset(top_ten, 0, SCORE_MAX * sizeof(*top_ten));
  score_read(top_ten);

  /* Insert her in list if need be */
  if (!wizard) {
    score_insert(top_ten, amount, death_type);
  }

  /* Print the highscore */
  score_print(top_ten);

  Game::exit();
}

void
score_win_and_exit(void)
{
  Game::io->force_redraw();
  Game::io->print_string(
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

  Game::io->print_string(0, IO::map_height - 1, "--Press space to continue--");
  Game::io->force_redraw();
  Game::io->wait_for_key(KEY_SPACE);
  score_show_and_exit(WON);
}


