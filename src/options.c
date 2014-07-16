/*
 * This file has all the code for the option command.  I would rather
 * this command were not necessary, but it is the only way to keep the
 * wolves off of my back.
 *
 * @(#)options.c	4.24 (Berkeley) 05/10/83
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "rogue.h"

#define NUM_OPTS (sizeof optlist / sizeof (OPTION))

/* description of an option and what to do with it */
typedef struct optstruct {
    char	*o_name;	/* option name */
    char	*o_prompt;	/* prompt for interactive entry */
    void 	*o_opt;		/* pointer to thing to set */
				/* function to print value */
    void 	(*o_putfunc)(void *opt);
				/* function to get value interactively */
    enum option_return	(*o_getfunc)(void *opt, WINDOW *win);
} OPTION;

void	pr_optname(OPTION *op);

OPTION	optlist[] = {
    {"terse",	 "Terse output",
		 &terse,	put_bool,	get_bool	},
    {"flush",	 "Flush typeahead during battle",
		 &fight_flush,	put_bool,	get_bool	},
    {"jump",	 "Show position only at end of run",
		 &jump,		put_bool,	get_bool	},
    {"seefloor", "Show the lamp-illuminated floor",
		 &see_floor,	put_bool,	get_sf		},
    {"passgo",	"Follow turnings in passageways",
		 &passgo,	put_bool,	get_bool	},
    {"tombstone", "Print out tombstone when killed",
		 &tombstone,	put_bool,	get_bool	},
    {"name",	 "Name",
		 whoami,	put_str,	get_str		},
    {"file",	 "Save file",
		 file_name,	put_str,	get_str		}
};

/*
 * option:
 *	Print and then set options from the terminal
 */

void
option()
{
    OPTION	*op;
    enum option_return	retval;

    wclear(hw);
    /*
     * Display current values of options
     */
    for (op = optlist; op <= &optlist[NUM_OPTS-1]; op++)
    {
	pr_optname(op);
	(*op->o_putfunc)(op->o_opt);
	waddch(hw, '\n');
    }
    /*
     * Set values
     */
    wmove(hw, 0, 0);
    for (op = optlist; op <= &optlist[NUM_OPTS-1]; op++)
    {
	pr_optname(op);
	retval = (*op->o_getfunc)(op->o_opt, hw);
	if (retval)
	{
	    if (retval == QUIT)
		break;
	    else if (op > optlist) {	/* MINUS */
		wmove(hw, (int)(op - optlist) - 1, 0);
		op -= 2;
	    }
	    else	/* trying to back up beyond the top */
	    {
		putchar('\007');
		wmove(hw, 0, 0);
		op--;
	    }
	}
    }
    /*
     * Switch back to original screen
     */
    wmove(hw, LINES - 1, 0);
    waddstr(hw, "--Press space to continue--");
    wrefresh(hw);
    wait_for(KEY_SPACE);
    clearok(curscr, true);
    touchwin(stdscr);
    after = false;
}

/*
 * pr_optname:
 *	Print out the option name prompt
 */

void
pr_optname(OPTION *op)
{
    wprintw(hw, "%s (\"%s\"): ", op->o_prompt, op->o_name);
}

/*
 * put_bool
 *	Put out a boolean
 */

void
put_bool(void *b)
{
    waddstr(hw, *(bool *) b ? "True" : "False");
}

/*
 * put_str:
 *	Put out a string
 */

void
put_str(void *str)
{
    waddstr(hw, (char *) str);
}

/*
 * get_bool:
 *	Allow changing a boolean option and print it out
 */
enum option_return
get_bool(void *vp, WINDOW *win)
{
    bool *bp = (bool *) vp;
    int oy, ox;
    bool op_bad;

    op_bad = true;
    getyx(win, oy, ox);
    waddstr(win, *bp ? "True" : "False");
    while (op_bad)	
    {
	wmove(win, oy, ox);
	wrefresh(win);
	switch (readchar())
	{
	    case 't':
	    case 'T':
		*bp = true;
		op_bad = false;
		break;
	    case 'f':
	    case 'F':
		*bp = false;
		op_bad = false;
		break;
	    case '\n':
	    case '\r':
		op_bad = false;
		break;
	    case KEY_ESCAPE:
		return QUIT;
	    case '-':
		return MINUS;
	    default:
		wmove(win, oy, ox + 10);
		waddstr(win, "(T or F)");
	}
    }
    wmove(win, oy, ox);
    waddstr(win, *bp ? "True" : "False");
    waddch(win, '\n');
    return NORMAL;
}

/*
 * get_sf:
 *	Change value and handle transition problems from see_floor to
 *	!see_floor.
 */
enum option_return
get_sf(void *vp, WINDOW *win)
{
    bool	*bp = (bool *) vp;
    bool	was_sf;
    enum option_return	retval;

    was_sf = see_floor;
    retval = get_bool(bp, win);
    if (retval == QUIT) return(QUIT);
    if (was_sf != see_floor)
    {
	if (!see_floor) {
	    see_floor = true;
	    erase_lamp(&hero, proom);
	    see_floor = false;
	}
	else
	    look(false);
    }
    return NORMAL;
}

/*
 * get_str:
 *	Set a string option
 */
#define MAXINP	50	/* max string to read from terminal or environment */

enum option_return
get_str(void *vopt, WINDOW *win)
{
    char *opt = (char *) vopt;
    char *sp;
    int oy, ox;
    int i;
    signed char c;
    static char buf[MAXSTR];

    getyx(win, oy, ox);
    wrefresh(win);
    /*
     * loop reading in the string, and put it in a temporary buffer
     */
    for (sp = buf; (c = readchar()) != '\n' && c != '\r' && c != KEY_ESCAPE;
	wclrtoeol(win), wrefresh(win))
    {
	if (c == -1)
	    continue;
	else if (c == erasechar())	/* process erase character */
	{
	    if (sp > buf)
	    {
		sp--;
		for (i = (int) strlen(unctrl(*sp)); i; i--)
		    waddch(win, '\b');
	    }
	    continue;
	}
	else if (c == killchar())	/* process kill character */
	{
	    sp = buf;
	    wmove(win, oy, ox);
	    continue;
	}
	else if (sp == buf)
	{
	    if (c == '-' && win != stdscr)
		break;
	    else if (c == '~')
	    {
		strcpy(buf, md_gethomedir());
		waddstr(win, md_gethomedir());
		sp += strlen(md_gethomedir());
		continue;
	    }
	}
	if (sp >= &buf[MAXINP] || !(isprint(c) || c == ' '))
	    putchar(CTRL('G'));
	else
	{
	    *sp++ = c;
	    waddstr(win, unctrl(c));
	}
    }
    *sp = '\0';
    if (sp > buf)	/* only change option if something has been typed */
	strucpy(opt, buf, (int) strlen(buf));
    mvwprintw(win, oy, ox, "%s\n", opt);
    wrefresh(win);
    if (win == stdscr)
	mpos += (int)(sp - buf);
    if (c == '-')
	return MINUS;
    else if (c == KEY_ESCAPE)
	return QUIT;
    else
	return NORMAL;
}

/*
 * get_num:
 *	Get a numeric option
 */
enum option_return
get_num(void *vp, WINDOW *win)
{
    short *opt = (short *) vp;
    enum option_return i;
    static char buf[MAXSTR];

    if ((i = get_str(buf, win)) == NORMAL)
	*opt = (short) atoi(buf);
    return i;
}

/*
 * strucpy:
 *	Copy string using unctrl for things
 */

void
strucpy(char *s1, const char *s2, int len)
{
    if (len > MAXINP)
	len = MAXINP;
    while (len--)
    {
	if (isprint(*s2) || *s2 == ' ')
	    *s1++ = *s2;
	s2++;
    }
    *s1 = '\0';
}
