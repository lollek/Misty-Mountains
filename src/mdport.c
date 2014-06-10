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
