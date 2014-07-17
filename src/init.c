/*
 * global variable initializaton
 *
 * @(#)init.c	4.31 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "rogue.h"
#include "potions.h"
#include "status_effects.h"
#include "scrolls.h"

/* init_new_game
 * Set up everything so we can start playing already */
bool
init_new_game()
{
  /* Parse environment opts */
  if (whoami[0] == '\0')
    strucpy(whoami, md_getusername(), (int) strlen(md_getusername()));

  if (wizard)
    printf("Hello %s, welcome to dungeon #%d", whoami, seed);
  else
    printf("Hello %s, just a moment while I dig the dungeon...", whoami);
  fflush(stdout);

  /* Init Graphics */
  if (init_graphics() != 0)
    return false;
  idlok(stdscr, true);
  idlok(hw, true);

  /* Init stuff */
  init_probs();                         /* Set up prob tables for objects */
  init_player();                        /* Set up initial player stats */
  init_names();                         /* Set up names of scrolls */
  init_colors();                        /* Set up colors of potions */
  init_stones();                        /* Set up stone settings of rings */
  init_materials();                     /* Set up materials of wands */

  new_level();                          /* Draw current level */

  /* Start up daemons and fuses */
  start_daemon(runners, 0, AFTER);
  start_daemon(doctor, 0, AFTER);
  fuse(swander, 0, WANDERTIME, AFTER);
  start_daemon(stomach, 0, AFTER);

  return true;
}

/** init_old_game:
 * Restore a saved game from a file with elaborate checks for file
 * integrity from cheaters */
bool
init_old_game()
{
    FILE *inf = fopen(file_name, "r");
    char buf[MAXSTR];

    if (inf == NULL)
    {
        perror(file_name);
        return false;
    }

    /* defeat multiple restarting from the same place
     * TODO: should these be removed? not hard to circumvent anyways */
    if (!wizard)
    {
      struct stat sbuf2;
      if (lstat(file_name, &sbuf2) == -1)
      {
        perror(file_name);
        return false;
      }
      if ((sbuf2.st_mode & S_IFMT) != S_IFREG)
      {
        printf("Only normal files allowed (no symlinks, FIFOs, etc)\n");
        return false;
      }
      if (sbuf2.st_nlink != 1)
      {
        printf("The savegame cannot be hardlinked, since that's cheating\n");
        return false;
      }
    }

    if (encread_and_check_version(buf, inf) == false)
    {
      printf("Sorry, saved game is out of date.\n");
      return false;
    }

    encread(buf, 80, inf);

    if (init_graphics() != 0)
      return false;

    rs_restore_file(inf);
    /*
     * we do not close the file so that we will have a hold of the
     * inode for as long as possible
     */

    if (!wizard && unlink(file_name) < 0)
    {
        endwin();
        printf("Cannot unlink file\n");
        return false;
    }
    mpos = 0;
    clearok(stdscr,true);

    if (pstats.s_hpt <= 0)
    {
        endwin();
        printf("\n\"He's dead, Jim\"\n");
        return false;
    }

    clearok(curscr, true);
    msg("file name: %s", file_name);
    return true;
}

/*
 * init_graphics:
 * 	get curses running
 */
int
init_graphics()
{
  initscr();  /* Start up cursor package */

  /* Ncurses colors */
  if (use_colors)
  {
    if (start_color() == ERR)
    {
      endwin();
      fprintf(stderr, "Error: Failed to start colors. "
                      "Try restarting without colors enabled\n");
      return 1;
    }

    /* Because ncurses has defined COLOR_BLACK to 0 and COLOR_WHITE to 7,
     * and then decided that init_pair cannot change number 0 (COLOR_BLACK)
     * I use COLOR_WHITE for black text and COLOR_BLACK for white text */

    assume_default_colors(0, -1); /* Default is white text and any background */
    init_pair(COLOR_RED, COLOR_RED, -1);
    init_pair(COLOR_GREEN, COLOR_GREEN, -1);
    init_pair(COLOR_YELLOW, COLOR_YELLOW, -1);
    init_pair(COLOR_BLUE, COLOR_BLUE, -1);
    init_pair(COLOR_MAGENTA, COLOR_MAGENTA, -1);
    init_pair(COLOR_CYAN, COLOR_CYAN, -1);
    init_pair(COLOR_WHITE, COLOR_BLACK, -1);
  }

  if (LINES < NUMLINES || COLS < NUMCOLS)
  {
    endwin();
    printf("\nSorry, the screen must be at least %dx%d\n", NUMLINES, NUMCOLS);
    return 1;
  }

  raw();     /* Raw mode */
  noecho();  /* Echo off */
  hw = newwin(LINES, COLS, 0, 0);

  return 0;
}

/*
 * init_player:
 *	Roll her up
 */
void
init_player()
{
    THING *obj;

    pstats = max_stats;
    food_left = HUNGERTIME;
    /*
     * Give him some food
     */
    obj = new_item();
    obj->o_type = FOOD;
    obj->o_count = 1;
    add_pack(obj, true);
    /*
     * And his suit of armor
     */
    obj = new_item();
    obj->o_type = ARMOR;
    obj->o_which = RING_MAIL;
    obj->o_arm = a_class[RING_MAIL] - 1;
    obj->o_flags |= ISKNOW;
    obj->o_count = 1;
    cur_armor = obj;
    add_pack(obj, true);
    /*
     * Give him his weaponry.  First a mace.
     */
    obj = new_item();
    init_weapon(obj, MACE);
    obj->o_hplus = 1;
    obj->o_dplus = 1;
    obj->o_flags |= ISKNOW;
    add_pack(obj, true);
    cur_weapon = obj;
    /*
     * Now a +1 bow
     */
    obj = new_item();
    init_weapon(obj, BOW);
    obj->o_hplus = 1;
    obj->o_flags |= ISKNOW;
    add_pack(obj, true);
    /*
     * Now some arrows
     */
    obj = new_item();
    init_weapon(obj, ARROW);
    obj->o_count = rnd(15) + 25;
    obj->o_flags |= ISKNOW;
    add_pack(obj, true);
}

/*
 * Contains defintions and functions for dealing with things like
 * potions and scrolls
 */

char *rainbow[] = {
    "amber",
    "aquamarine",
    "black",
    "blue",
    "brown",
    "clear",
    "crimson",
    "cyan",
    "ecru",
    "gold",
    "green",
    "grey",
    "magenta",
    "orange",
    "pink",
    "plaid",
    "purple",
    "red",
    "silver",
    "tan",
    "tangerine",
    "topaz",
    "turquoise",
    "vermilion",
    "violet",
    "white",
    "yellow",
};

#define NCOLORS (sizeof rainbow / sizeof (const char *))
int cNCOLORS = NCOLORS;

static char *sylls[] = {
    "a", "ab", "ag", "aks", "ala", "an", "app", "arg", "arze", "ash",
    "bek", "bie", "bit", "bjor", "blu", "bot", "bu", "byt", "comp",
    "con", "cos", "cre", "dalf", "dan", "den", "do", "e", "eep", "el",
    "eng", "er", "ere", "erk", "esh", "evs", "fa", "fid", "fri", "fu",
    "gan", "gar", "glen", "gop", "gre", "ha", "hyd", "i", "ing", "ip",
    "ish", "it", "ite", "iv", "jo", "kho", "kli", "klis", "la", "lech",
    "mar", "me", "mi", "mic", "mik", "mon", "mung", "mur", "nej",
    "nelg", "nep", "ner", "nes", "nes", "nih", "nin", "o", "od", "ood",
    "org", "orn", "ox", "oxy", "pay", "ple", "plu", "po", "pot",
    "prok", "re", "rea", "rhov", "ri", "ro", "rog", "rok", "rol", "sa",
    "san", "sat", "sef", "seh", "shu", "ski", "sna", "sne", "snik",
    "sno", "so", "sol", "sri", "sta", "sun", "ta", "tab", "tem",
    "ther", "ti", "tox", "trol", "tue", "turs", "u", "ulk", "um", "un",
    "uni", "ur", "val", "viv", "vly", "vom", "wah", "wed", "werg",
    "wex", "whon", "wun", "xo", "y", "yot", "yu", "zant", "zeb", "zim",
    "zok", "zon", "zum",
};

STONE stones[] = {
    { "agate",		 25},
    { "alexandrite",	 40},
    { "amethyst",	 50},
    { "carnelian",	 40},
    { "diamond",	300},
    { "emerald",	300},
    { "germanium",	225},
    { "granite",	  5},
    { "garnet",		 50},
    { "jade",		150},
    { "kryptonite",	300},
    { "lapis lazuli",	 50},
    { "moonstone",	 50},
    { "obsidian",	 15},
    { "onyx",		 60},
    { "opal",		200},
    { "pearl",		220},
    { "peridot",	 63},
    { "ruby",		350},
    { "sapphire",	285},
    { "stibotantalite",	200},
    { "tiger eye",	 50},
    { "topaz",		 60},
    { "turquoise",	 70},
    { "taaffeite",	300},
    { "zircon",	 	 80},
};

#define NSTONES (sizeof stones / sizeof (STONE))
int cNSTONES = NSTONES;

char *wood[] = {
    "avocado wood",
    "balsa",
    "bamboo",
    "banyan",
    "birch",
    "cedar",
    "cherry",
    "cinnibar",
    "cypress",
    "dogwood",
    "driftwood",
    "ebony",
    "elm",
    "eucalyptus",
    "fall",
    "hemlock",
    "holly",
    "ironwood",
    "kukui wood",
    "mahogany",
    "manzanita",
    "maple",
    "oaken",
    "persimmon wood",
    "pecan",
    "pine",
    "poplar",
    "redwood",
    "rosewood",
    "spruce",
    "teak",
    "walnut",
    "zebrawood",
};

#define NWOOD (sizeof wood / sizeof (char *))
int cNWOOD = NWOOD;

char *metal[] = {
    "aluminum",
    "beryllium",
    "bone",
    "brass",
    "bronze",
    "copper",
    "electrum",
    "gold",
    "iron",
    "lead",
    "magnesium",
    "mercury",
    "nickel",
    "pewter",
    "platinum",
    "steel",
    "silver",
    "silicon",
    "tin",
    "titanium",
    "tungsten",
    "zinc",
};

#define NMETAL (sizeof metal / sizeof (char *))
int cNMETAL = NMETAL;
#define MAX3(a,b,c)	(a > b ? (a > c ? a : c) : (b > c ? b : c))

static bool used[MAX3(NCOLORS, NSTONES, NWOOD)];

/*
 * init_colors:
 *	Initialize the potion color scheme for this time
 */
void
init_colors()
{
    size_t i, j;

    for (i = 0; i < NCOLORS; i++)
	used[i] = false;
    for (i = 0; i < NPOTIONS; i++)
    {
	do
	    j = rnd(NCOLORS);
	until (!used[j]);
	used[j] = true;
	p_colors[i] = rainbow[j];
    }
}

/*
 * init_names:
 *	Generate the names of the various scrolls
 */
#define MAXNAME	40	/* Max number of characters in a name */

void
init_names()
{
    int nsyl;
    char *cp, *sp;
    int i, nwords;
    char tmpbuf[MAXSTR*2];

    for (i = 0; i < MAXSCROLLS; i++)
    {
	cp = tmpbuf;
	nwords = rnd(3) + 2;
	while (nwords--)
	{
	    nsyl = rnd(3) + 1;
	    while (nsyl--)
	    {
		sp = sylls[rnd((sizeof sylls) / (sizeof (char *)))];
		if (&cp[strlen(sp)] > &tmpbuf[MAXNAME])
			break;
		while (*sp)
		    *cp++ = *sp++;
	    }
	    *cp++ = ' ';
	}
	*--cp = '\0';
	s_names[i] = (char *) malloc((unsigned) strlen(tmpbuf)+1);
	strcpy(s_names[i], tmpbuf);
    }
}

/*
 * init_stones:
 *	Initialize the ring stone setting scheme for this time
 */
void
init_stones()
{
    size_t i, j;

    for (i = 0; i < NSTONES; i++)
	used[i] = false;
    for (i = 0; i < MAXRINGS; i++)
    {
	do
	    j = rnd(NSTONES);
	until (!used[j]);
	used[j] = true;
	r_stones[i] = stones[j].st_name;
	ring_info[i].oi_worth += stones[j].st_value;
    }
}

/*
 * init_materials:
 *	Initialize the construction materials for wands and staffs
 */
void
init_materials()
{
    size_t i, j;
    char *str;
    static bool metused[NMETAL];

    for (i = 0; i < NWOOD; i++)
	used[i] = false;
    for (i = 0; i < NMETAL; i++)
	metused[i] = false;
    for (i = 0; i < MAXSTICKS; i++)
    {
	for (;;)
	    if (rnd(2) == 0)
	    {
		j = rnd(NMETAL);
		if (!metused[j])
		{
		    ws_type[i] = "wand";
		    str = metal[j];
		    metused[j] = true;
		    break;
		}
	    }
	    else
	    {
		j = rnd(NWOOD);
		if (!used[j])
		{
		    ws_type[i] = "staff";
		    str = wood[j];
		    used[j] = true;
		    break;
		}
	    }
	ws_made[i] = str;
    }
}

# define	NT	NUMTHINGS, "things"
# define	MP	NPOTIONS, "potions"
# define	MS	MAXSCROLLS, "scrolls"
# define	MR	MAXRINGS, "rings"
# define	MWS	MAXSTICKS, "sticks"
# define	MW	MAXWEAPONS, "weapons"
# define	MA	MAXARMORS, "armor"

/*
 * sumprobs:
 *	Sum up the probabilities for items appearing
 */
void
sumprobs(struct obj_info *info, int bound , char *name)
{
  struct obj_info *start = info;
  struct obj_info *endp;

  endp = info + bound;
  while (++info < endp)
    info->oi_prob += (info - 1)->oi_prob;
    if (wizard)
      badcheck(name, start, bound);
}

/*
 * init_probs:
 *	Initialize the probabilities for the various items
 */
void
init_probs()
{
    sumprobs(things, NT);
    sumprobs(pot_info, MP);
    sumprobs(scr_info, MS);
    sumprobs(ring_info, MR);
    sumprobs(ws_info, MWS);
    sumprobs(weap_info, MW);
    sumprobs(arm_info, MA);
}

/*
 * badcheck:
 *	Check to see if a series of probabilities sums to 100
 */
void
badcheck(const char *name, struct obj_info *info, int bound)
{
    struct obj_info *end;

    if (info[bound - 1].oi_prob == 100)
	return;
    printf("\nBad percentages for %s (bound = %d):\n", name, bound);
    for (end = &info[bound]; info < end; info++)
	printf("%3d%% %s\n", info->oi_prob, info->oi_name);
    printf("[hit RETURN to continue]");
    fflush(stdout);
    while (getchar() != '\n')
	continue;
}

/*
 * pick_color:
 *	If he is halucinating, pick a random color name and return it,
 *	otherwise return the given color.
 */
const char *
pick_color(const char *col)
{
    return (is_hallucinating(&player) ? rainbow[rnd(NCOLORS)] : col);
}
