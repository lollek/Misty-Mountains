/*
 * Various input/output functions
 *
 * @(#)io.c	4.32 (Berkeley) 02/05/99
 */

#include <ctype.h>
#include <string.h>

#include "rogue.h"

static chtype colorize(const chtype ch);

/** colorize_ch
 * Returns a ch in color (if enabled) */
static chtype
colorize(const chtype ch)
{
  if (!use_colors)
    return ch;

  /* NOTE: COLOR_WHITE is black and COLOR_BLACK is white, because reasons */

  switch (ch)
  {
    /* Dungeon */
    case HWALL: case VWALL: return ch | COLOR_PAIR(COLOR_WHITE) | A_BOLD;
    case PASSAGE: case FLOOR: case STAIRS: return ch | COLOR_PAIR(COLOR_YELLOW);
    case TRAP: return ch | COLOR_PAIR(COLOR_RED);

    /* Items */
    case GOLD: return ch | COLOR_PAIR(COLOR_YELLOW) | A_BOLD;

    /* Monsters */
    case 'B': return ch | COLOR_PAIR(COLOR_WHITE) | A_BOLD;
    case 'E': return ch | COLOR_PAIR(COLOR_MAGENTA);
    case 'H': return ch | COLOR_PAIR(COLOR_GREEN);
    case 'I': return ch | COLOR_PAIR(COLOR_CYAN);
    case 'R': return ch | COLOR_PAIR(COLOR_RED);
    case 'S': return ch | COLOR_PAIR(COLOR_GREEN);


    default: return ch | COLOR_PAIR(COLOR_BLACK);
  }
}

/** wincch
 * Returns a winch() without any color */
chtype
wincch(WINDOW *win)
{
  return winch(win) & A_CHARTEXT;
}

/** mvwincch
 * Returns a mvwinch() without any color */
chtype
mvwincch(WINDOW *win, int y, int x)
{
  return mvwinch(win, y, x) & A_CHARTEXT;
}

/** mvwaddcch (Window Move Add Colored Character)
 * Does a mvwaddch with color if enabled */
int
mvwaddcch(WINDOW *window, int y, int x, const chtype ch)
{
  return mvwaddch(window, y, x, colorize(ch));
}

/** waddcch (Window Add Colored Character)
 * Prints a character color if enabled */
int
waddcch(WINDOW *window, const chtype ch)
{
  return waddch(window, colorize(ch));
}

/*
 * msg:
 *	Display a message at the top of the screen.
 */
#define MAXMSG	(NUMCOLS - sizeof "--More--")

static char msgbuf[2*MAXMSG+1];
static int newpos = 0;

void
unsaved_msg(const char *fmt, ...)
{
  va_list args;
  char tmp[MAXSTR];

  strcpy(tmp, huh);
  va_start(args, fmt);
  doadd(fmt, args);
  va_end(args);
  endmsg();
  strcpy(huh, tmp);
}

/* VARARGS1 */
int
msg(const char *fmt, ...)
{
    va_list args;

    /*
     * if the string is "", just clear the line
     */
    if (*fmt == '\0')
    {
	move(0, 0);
	clrtoeol();
	mpos = 0;
	return ~KEY_ESCAPE;
    }
    /*
     * otherwise add to the message and flush it out
     */
    va_start(args, fmt);
    doadd(fmt, args);
    va_end(args);
    return endmsg();
}

/*
 * addmsg:
 *	Add things to the current message
 */
/* VARARGS1 */
void
addmsg(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    doadd(fmt, args);
    va_end(args);
}

/*
 * endmsg:
 *	Display a new msg (giving him a chance to see the previous one
 *	if it is up there with the --More--)
 */
int
endmsg()
{
    char ch;

    /* Save message in case player missed it */
    strcpy(huh, msgbuf);
    if (mpos)
    {
	look(false);
	mvaddstr(0, mpos, "--More--");
	refresh();
	if (!msg_esc)
	    wait_for(KEY_SPACE);
	else
	{
	    while ((ch = readchar()) != KEY_SPACE)
		if (ch == KEY_ESCAPE)
		{
		    msgbuf[0] = '\0';
		    mpos = 0;
		    newpos = 0;
		    msgbuf[0] = '\0';
		    return KEY_ESCAPE;
		}
	}
    }
    /*
     * All messages should start with uppercase, except ones that
     * start with a pack addressing character
     */
    if (islower(msgbuf[0]) && msgbuf[1] != ')')
	msgbuf[0] = (char) toupper(msgbuf[0]);
    mvaddstr(0, 0, msgbuf);
    clrtoeol();
    mpos = newpos;
    newpos = 0;
    msgbuf[0] = '\0';
    refresh();
    return ~KEY_ESCAPE;
}

/*
 * doadd:
 *	Perform an add onto the message buffer
 */
void
doadd(const char *fmt, va_list args)
{
    static char buf[MAXSTR];

    /*
     * Do the printf into buf
     */
    vsprintf(buf, fmt, args);
    if (strlen(buf) + newpos >= MAXMSG)
        endmsg(); 
    strcat(msgbuf, buf);
    newpos = (int) strlen(msgbuf);
}

/*
 * step_ok:
 *	Returns true if it is ok to step on ch
 */
int
step_ok(int ch)
{
  if (ch == SHADOW || ch == HWALL || ch == VWALL)
    return false;
  return !isalpha(ch);
}

/*
 * readchar:
 *	Reads and returns a character, checking for gross input errors
 */
char
readchar()
{
    char ch;

    ch = (char) getch();

    if (ch == 3)
    {
	quit(0);
        return(27);
    }

    return(ch);
}

/*
 * status:
 *	Display the important stats line.  Keep the cursor where it was.
 */
void
status(bool stat_msg)
{
    int oy, ox;
    int temp = cur_armor != NULL ? cur_armor->o_arm : pstats.s_arm;
    char *state_name[] = { "", "Hungry", "Weak", "Faint" };
    static int hpwidth = 0;
    static int s_hungry = 0;
    static int s_lvl = 0;
    static int s_pur = -1;
    static int s_hp = 0;
    static int s_arm = 0;
    static str_t s_str = 0;
    static int s_exp = 0;

    /* If nothing has changed since the last status, don't bother. */
    if (s_hp == pstats.s_hpt && s_exp == pstats.s_exp && s_pur == purse
	&& s_arm == temp && s_str == pstats.s_str && s_lvl == level
	&& s_hungry == hungry_state
	&& !stat_msg
	)
	    return;

    s_arm = temp;

    getyx(stdscr, oy, ox);
    if (s_hp != max_hp)
    {
	temp = max_hp;
	s_hp = max_hp;
	for (hpwidth = 0; temp; hpwidth++)
	    temp /= 10;
    }

    /* Save current status */
    s_lvl = level;
    s_pur = purse;
    s_hp = pstats.s_hpt;
    s_str = pstats.s_str;
    s_exp = pstats.s_exp; 
    s_hungry = hungry_state;

    if (stat_msg)
    {
	move(0, 0);
        msg("Level: %d  Gold: %-5d  Hp: %*d(%*d)  Str: %2d(%d)  Arm: %-2d  Exp: %d/%ld  %s",
	    level, purse, hpwidth, pstats.s_hpt, hpwidth, max_hp, pstats.s_str,
	    max_stats.s_str, 10 - s_arm, pstats.s_lvl, pstats.s_exp,
	    state_name[hungry_state]);
    }
    else
    {
	move(STATLINE, 0);
                
        printw("Level: %d  Gold: %-5d  Hp: %*d(%*d)  Str: %2d(%d)  Arm: %-2d  Exp: %d/%d  %s",
	    level, purse, hpwidth, pstats.s_hpt, hpwidth, max_hp, pstats.s_str,
	    max_stats.s_str, 10 - s_arm, pstats.s_lvl, pstats.s_exp,
	    state_name[hungry_state]);
    }

    clrtoeol();
    move(oy, ox);
}

/*
 * wait_for
 *	Sit around until the guy types the right key
 */
void
wait_for(int ch)
{
    register char c;

    if (ch == '\n')
        while ((c = readchar()) != '\n' && c != '\r')
	    continue;
    else
        while (readchar() != ch)
	    continue;
}

/*
 * show_win:
 *	Function used to display a window and wait before returning
 */
void
show_win(const char *message)
{
    WINDOW *win = hw;

    wmove(win, 0, 0);
    waddstr(win, message);
    touchwin(win);
    wmove(win, hero.y, hero.x);
    wrefresh(win);
    wait_for(KEY_SPACE);
    clearok(curscr, true);
    touchwin(stdscr);
}
