/*
    mdport.c - Machine Dependent Code for Porting Unix/Curses games

    Copyright (C) 2005 Nicholas J. Kisseberth
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
    3. Neither the name(s) of the author(s) nor the names of other contributors
       may be used to endorse or promote products derived from this software
       without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS ``AS IS'' AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
    OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
    OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
    SUCH DAMAGE.
*/

#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <curses.h>
#include <term.h>
#include <sys/wait.h>
#include <ctype.h>
#include <limits.h>
#include <sys/stat.h>

#include "extern.h"

#define NOOP(x) (x += 0)

int
md_hasclreol()
{
  return cur_term != NULL &&
         cur_term->type.Strings != NULL &&
         clr_eol != NULL &&
         *clr_eol != 0;
}

void
md_normaluser()
{
  gid_t realgid = getgid();
  uid_t realuid = getuid();

  if (setregid(realgid, realgid) != 0) {
    perror("Could not drop setgid privileges.  Aborting.");
    exit(1);
  }
  if (setreuid(realuid, realuid) != 0) {
    perror("Could not drop setuid privileges.  Aborting.");
    exit(1);
  }
}

char *
md_getusername()
{
  static char login[80];
  struct passwd *pw = getpwuid(getuid());

  strncpy(login, pw == NULL ? "nobody" : pw->pw_name, 80);
  login[79] = 0;
  return login;
}

char *
md_gethomedir()
{
  static char homedir[PATH_MAX];
  size_t len;
  struct passwd *pw = getpwuid(getuid());
  char *h = pw == NULL ? NULL : pw->pw_dir;

  if (h == NULL || *h == '\0' || !strcmp(h, "/"))
    h = getenv("HOME");
  if (h == NULL || !strcmp(h, "/"))
    h = "";

  strncpy(homedir, h, PATH_MAX);
  homedir[PATH_MAX -1] = '\0';

  if ((len = strlen(homedir)) > 0 && homedir[len -1] != '/')
  {
    homedir[len] = '/';
    homedir[len +1] = '\0';
  }

  return homedir;
}

int
md_shellescape()
{
  puts("Shell escape has been removed, use ^Z instead");
  return(0);
}

char *
md_getrealname(int uid)
{
  static char uidstr[20];
  struct passwd *pp = getpwuid(uid);

  if (pp == NULL)
  {
    sprintf(uidstr,"%d", uid);
    return uidstr;
  }
  else
    return pp->pw_name;
}

extern char *xcrypt(char *key, char *salt);

char *
md_crypt(char *key, char *salt)
{
  return( xcrypt(key,salt) );
}

#define M_NORMAL 0
#define M_ESC    1
#define M_KEYPAD 2
#define M_TRAIL  3

/* TODO: This needs cleanup */
int
md_readchar()
{
  int ch = 0;
  int lastch = 0;
  int mode = M_NORMAL;
  int mode2 = M_NORMAL;

  for(;;)
  {
    ch = getch();

    if (ch == ERR)    /* timed out waiting for valid sequence */
    {                 /* flush input so far and start over    */
      mode = M_NORMAL;
      nocbreak();
      raw();
      ch = 27;
      break;
    }

    if (mode == M_TRAIL)
    {
      if (ch == '^')		/* msys console  : 7,5,6,8: modified*/
        ch = CTRL( toupper(lastch) );

      if (ch == '~')		/* cygwin console: 1,5,6,4: normal  */
        ch = tolower(lastch);   /* windows telnet: 1,5,6,4: normal  */
      /* msys console  : 7,5,6,8: normal  */

      if (mode2 == M_ESC)		/* cygwin console: 1,5,6,4: modified*/
        ch = CTRL( toupper(ch) );

      break;
    }

    if (mode == M_ESC) 
    {
      if (ch == 27)
      {
        mode2 = M_ESC;
        continue;
      }

      if ((ch == 'F') || (ch == 'O') || (ch == '['))
      {
        mode = M_KEYPAD;
        continue;
      }


      switch(ch)
      {
        /* Cygwin Console   */
        /* PuTTY	    */
        case KEY_LEFT :	ch = CTRL('H'); break;
        case KEY_RIGHT: ch = CTRL('L'); break;
        case KEY_UP   : ch = CTRL('K'); break;
        case KEY_DOWN : ch = CTRL('J'); break;
        case KEY_HOME : ch = CTRL('Y'); break;
        case KEY_PPAGE: ch = CTRL('U'); break;
        case KEY_NPAGE: ch = CTRL('N'); break;
        case KEY_END  : ch = CTRL('B'); break;

        default: break;
      }

      break;
    }

    if (mode == M_KEYPAD)
    {
      switch(ch)
      {
        /* ESC F - Interix Console codes */
        case   '^': ch = CTRL('H'); break;	/* Shift-Left	    */
        case   '$': ch = CTRL('L'); break;	/* Shift-Right	    */

                    /* ESC [ - Interix Console codes */
        case   'H': ch = 'y'; break;		/* Home		    */
        case     1: ch = CTRL('K'); break;	/* Ctl-Keypad Up    */
        case     2: ch = CTRL('J'); break;	/* Ctl-Keypad Down  */
        case     3: ch = CTRL('L'); break;	/* Ctl-Keypad Right */
        case     4: ch = CTRL('H'); break;	/* Ctl-Keypad Left  */
        case   263: ch = CTRL('Y'); break;	/* Ctl-Keypad Home  */
        case    19: ch = CTRL('U'); break;	/* Ctl-Keypad PgUp  */
        case    20: ch = CTRL('N'); break;	/* Ctl-Keypad PgDn  */
        case    21: ch = CTRL('B'); break;	/* Ctl-Keypad End   */

                    /* ESC [ - Cygwin Console codes */
        case   'G': ch = '.'; break;		/* Keypad 5	    */
        case   '7': lastch = 'Y'; mode=M_TRAIL; break;	/* Ctl-Home */
        case   '5': lastch = 'U'; mode=M_TRAIL; break;	/* Ctl-PgUp */
        case   '6': lastch = 'N'; mode=M_TRAIL; break;	/* Ctl-PgDn */

                    /* ESC [ - Win32 Telnet, PuTTY */
        case   '1': lastch = 'y'; mode=M_TRAIL; break;	/* Home	    */
        case   '4': lastch = 'b'; mode=M_TRAIL; break;	/* End	    */

                    /* ESC O - PuTTY */
        case   'D': ch = CTRL('H'); break;
        case   'C': ch = CTRL('L'); break;
        case   'A': ch = CTRL('K'); break;
        case   'B': ch = CTRL('J'); break;
        case   't': ch = 'h'; break;
        case   'v': ch = 'l'; break;
        case   'x': ch = 'k'; break;
        case   'r': ch = 'j'; break;
        case   'w': ch = 'y'; break;
        case   'y': ch = 'u'; break;
        case   's': ch = 'n'; break;
        case   'q': ch = 'b'; break;
        case   'u': ch = '.'; break;
      }

      if (mode != M_KEYPAD)
        continue;
    }

    if (ch == 27)
    {
      halfdelay(1);
      mode = M_ESC;
      continue;
    }

    switch(ch)
    {
      case KEY_LEFT   : ch = 'h'; break;
      case KEY_DOWN   : ch = 'j'; break;
      case KEY_UP     : ch = 'k'; break;
      case KEY_RIGHT  : ch = 'l'; break;
      case KEY_HOME   : ch = 'y'; break;
      case KEY_PPAGE  : ch = 'u'; break;
      case KEY_END    : ch = 'b'; break;
      case KEY_LL     : ch = 'b'; break;
      case KEY_NPAGE  : ch = 'n'; break;

      case KEY_A1     : ch = 'y'; break;
      case KEY_A3     : ch = 'u'; break;
      case KEY_C1     : ch = 'b'; break;
      case KEY_C3     : ch = 'n'; break;
      case KEY_B2     : ch = '.'; break;

      case KEY_SRIGHT  : ch = CTRL('L'); break;
      case KEY_SLEFT   : ch = CTRL('H'); break;
      case KEY_SHOME   : ch = CTRL('Y'); break;
      case KEY_SPREVIOUS:ch = CTRL('U'); break;
      case KEY_SEND    : ch = CTRL('B'); break;
      case KEY_SNEXT   : ch = CTRL('N'); break;
      case 0x146       : ch = CTRL('K'); break; 	/* Shift-Up	*/
      case 0x145       : ch = CTRL('J'); break; 	/* Shift-Down	*/

      case KEY_EOL     : ch = CTRL('B'); break;

                         /* MSYS rxvt console */
      case 511	     : ch = CTRL('J'); break; /* Shift Dn */
      case 512         : ch = CTRL('J'); break; /* Ctl Down */
      case 514	     : ch = CTRL('H'); break; /* Ctl Left */
      case 516	     : ch = CTRL('L'); break; /* Ctl Right*/
      case 518	     : ch = CTRL('K'); break; /* Shift Up */
      case 519	     : ch = CTRL('K'); break; /* Ctl Up   */

                       /* NCURSES in Keypad mode sends this for Ctrl-H */
      case KEY_BACKSPACE: ch = CTRL('H'); break;
    }

    break;
  }

  nocbreak();	    /* disable halfdelay mode if on */
  raw();

  return(ch & 0x7F);
}

void
md_ignoreallsignals()
{
  int i;

  for (i = 0; i < NSIG; i++)
    signal(i, SIG_IGN);
}

void
md_tstphold()
{
#ifdef SIGTSTP
    /*
     * If a process can be suspended, this code wouldn't work
     */
# ifdef SIG_HOLD
    signal(SIGTSTP, SIG_HOLD);
# else
    signal(SIGTSTP, SIG_IGN);
# endif
#endif
}

void
md_tstpresume()
{
#ifdef SIGTSTP
    signal(SIGTSTP, tstp);
#endif
}

void
md_tstpsignal()
{
#ifdef SIGTSTP
    kill(0, SIGTSTP);		/* send actual signal and suspend process */
#endif
}

#if defined(CHECKTIME)
void
md_start_checkout_timer(int time)
{
    int  checkout();

#if defined(HAVE_ALARM) && defined(SIGALRM)
    signal(SIGALRM, checkout);
	alarm(time);
#endif
}

void
md_stop_checkout_timer()
{
#if defined(SIGALRM)
    signal(SIGALRM, SIG_IGN);
#endif
}

#endif
