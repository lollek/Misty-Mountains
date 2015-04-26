/*
 * File for the fun ends
 * Death or a total win
 *
 * @(#)rip.c	4.57 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <ctype.h>
#include <unistd.h>

#include "score.h"
#include "potions.h"
#include "scrolls.h"
#include "io.h"
#include "armor.h"
#include "pack.h"
#include "rogue.h"

static char *rip[] = {
"                       __________\n",
"                      /          \\\n",
"                     /    REST    \\\n",
"                    /      IN      \\\n",
"                   /     PEACE      \\\n",
"                  /                  \\\n",
"                  |                  |\n",
"                  |                  |\n",
"                  |   killed by a    |\n",
"                  |                  |\n",
"                  |       1980       |\n",
"                 *|     *  *  *      | *\n",
"         ________)/\\\\_//(\\/(/\\)/\\//\\/|_)_______\n",
    0
};

/** center:
 * Return the index to center the given string */
static int
center(const char *str)
{
  return 28 - (((int)strlen(str) + 1) / 2);
}


/** killname:
 * Convert a code to a monster name */
static char *
killname(char monst, bool doart)
{
  char *sp;
  bool article;
  static struct h_list nlist[] = {
    {'a',	"arrow",		true},
    {'b',	"bolt",			true},
    {'d',	"dart",			true},
    {'h',	"hypothermia",		false},
    {'s',	"starvation",		false},
    {'\0'}
  };

  if (isupper(monst))
  {
    sp = monsters[monst-'A'].m_name;
    article = true;
  }
  else
  {
    struct h_list *hp;
    sp = "Wally the Wonder Badger";
    article = false;
    for (hp = nlist; hp->h_ch; hp++)
      if (hp->h_ch == monst)
      {
        sp = hp->h_desc;
        article = hp->h_print;
        break;
      }
  }

  if (doart && article)
    sprintf(prbuf, "a%s ", vowelstr(sp));
  else
    prbuf[0] = '\0';
  strcat(prbuf, sp);
  return prbuf;
}



/*
 * score:
 *	Figure score and post it.
 */
/* VARARGS2 */

void
score(int amount, int flags, char monst)
{
    SCORE *scp;
    int i;
    SCORE *sc2;
    SCORE *top_ten, *endp;
    int prflags = 0;
    void (*fp)(int);
    unsigned int uid;
    static char *reason[] = {
	"killed",
	"quit",
	"A total winner",
	"killed with Amulet"
    };

 if (flags >= 0 || wizard)
    {
	mvaddstr(LINES - 1, 0 , "[Press return to continue]");
        refresh();
        wgetnstr(stdscr,prbuf,80);
 	endwin();
        printf("\n");
	/*
	 * free up space to "guarantee" there is space for the top_ten
	 */
	delwin(stdscr);
	delwin(curscr);
	if (hw != NULL)
	    delwin(hw);
    }

    top_ten = (SCORE *) malloc(NUMSCORES * sizeof (SCORE));
    endp = &top_ten[NUMSCORES];
    for (scp = top_ten; scp < endp; scp++)
    {
	scp->sc_score = 0;
	for (i = 0; i < MAXSTR; i++)
	    scp->sc_name[i] = (unsigned char) rnd(255);
	scp->sc_flags = rand_r(&seed);
	scp->sc_level = rand_r(&seed);
	scp->sc_monster = rand_r(&seed);
	scp->sc_uid = rand_r(&seed);
    }

    signal(SIGINT, SIG_DFL);

    if (wizard && strcmp(prbuf, "edit") == 0)
      prflags = 2;

    rd_score(top_ten);
    /*
     * Insert her in list if need be
     */
    sc2 = NULL;
    if (!potential_wizard)
    {
	uid = getuid();
	for (scp = top_ten; scp < endp; scp++)
	    if (amount > scp->sc_score)
		break;
	if (scp < endp)
	{
	    sc2 = endp - 1;
	    while (sc2 > scp)
	    {
		*sc2 = sc2[-1];
		sc2--;
	    }
	    scp->sc_score = amount;
	    strncpy(scp->sc_name, whoami, MAXSTR);
	    scp->sc_flags = flags;
	    if (flags == 2)
		scp->sc_level = max_level;
	    else
		scp->sc_level = level;
	    scp->sc_monster = monst;
	    scp->sc_uid = uid;
	    sc2 = scp;
	}
    }
    /*
     * Print the list
     */
    if (flags != -1)
	putchar('\n');
    printf("Top %s %s:\n", NUMNAME, "Scores");
    printf("   Score Name\n");
    for (scp = top_ten; scp < endp; scp++)
    {
	if (scp->sc_score) {
	    printf("%2d %5d %s: %s on level %d",
                (int) (scp - top_ten + 1), scp->sc_score, scp->sc_name,
                reason[scp->sc_flags], scp->sc_level);
	    if (scp->sc_flags == 0 || scp->sc_flags == 3)
		printf(" by %s", killname((char) scp->sc_monster, true));
	    if (prflags == 2)
	    {
		fflush(stdout);
		(void) fgets(prbuf,10,stdin);
		if (prbuf[0] == 'd')
		{
		    for (sc2 = scp; sc2 < endp - 1; sc2++)
			*sc2 = *(sc2 + 1);
		    sc2 = endp - 1;
		    sc2->sc_score = 0;
		    for (i = 0; i < MAXSTR; i++)
			sc2->sc_name[i] = (char) rnd(255);
		    sc2->sc_flags = rand_r(&seed);
		    sc2->sc_level = rand_r(&seed);
		    sc2->sc_monster = rand_r(&seed);
		    scp--;
		}
	    }
	    else
                printf(".");
            putchar('\n');
	}
	else
	    break;
    }
    /*
     * Update the list file
     */
    if (sc2 != NULL)
    {
	if (lock_sc())
	{
	    fp = signal(SIGINT, SIG_IGN);
	    wr_score(top_ten);
	    unlock_sc();
	    signal(SIGINT, fp);
	}
    }
}

/*
 * death:
 *	Do something really fun when he dies
 */

void
death(char monst)
{
    char **dp, *killer;
    struct tm *lt;
    static time_t date;
    struct tm *localtime();

    signal(SIGINT, SIG_IGN);
    purse -= purse / 10;
    signal(SIGINT, leave);
    clear();
    killer = killname(monst, false);
    if (!tombstone)
    {
	mvprintw(LINES - 2, 0, "Killed by ");
	killer = killname(monst, false);
	if (monst != 's' && monst != 'h')
	    printw("a%s ", vowelstr(killer));
	printw("%s with %d gold", killer, purse);
    }
    else
    {
	time(&date);
	lt = localtime(&date);
	move(8, 0);
	dp = rip;
	while (*dp)
	    addstr(*dp++);
	mvaddstr(17, center(killer), killer);
	if (monst == 's' || monst == 'h')
	    mvaddcch(16, 32, ' ');
	else
	    mvaddstr(16, 33, vowelstr(killer));
	mvaddstr(14, center(whoami), whoami);
	sprintf(prbuf, "%d Au", purse);
	move(15, center(prbuf));
	addstr(prbuf);
	sprintf(prbuf, "%4d", 1900+lt->tm_year);
	mvaddstr(18, 26, prbuf);
    }
    mvprintw(LINES -1, 0, "[Press return to continue]");
    refresh();
    wait_for(KEY_ENTER);
    pack_evaluate();
    score(purse, pack_contains_amulet() ? 3 : 0, monst);
    exit(0);
}

/*
 * total_winner:
 *	Code for a winner
 */

void
total_winner(void)
{
    clear();
    standout();
    addstr("                                                               \n");
    addstr("  @   @               @   @           @          @@@  @     @  \n");
    addstr("  @   @               @@ @@           @           @   @     @  \n");
    addstr("  @   @  @@@  @   @   @ @ @  @@@   @@@@  @@@      @  @@@    @  \n");
    addstr("   @@@@ @   @ @   @   @   @     @ @   @ @   @     @   @     @  \n");
    addstr("      @ @   @ @   @   @   @  @@@@ @   @ @@@@@     @   @     @  \n");
    addstr("  @   @ @   @ @  @@   @   @ @   @ @   @ @         @   @  @     \n");
    addstr("   @@@   @@@   @@ @   @   @  @@@@  @@@@  @@@     @@@   @@   @  \n");
    addstr("                                                               \n");
    addstr("     Congratulations, you have made it to the light of day!    \n");
    standend();
    addstr("\nYou have joined the elite ranks of those who have escaped the\n");
    addstr("Dungeons of Doom alive.  You journey home and sell all your loot at\n");
    addstr("a great profit and are admitted to the Fighters' Guild.\n");
    mvaddstr(LINES - 1, 0, "--Press space to continue--");
    refresh();
    wait_for(KEY_SPACE);
    purse += pack_evaluate();
    score(purse, 2, ' ');
    exit(0);
}


