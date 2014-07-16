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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <term.h>
#include <limits.h>

#include "rogue.h"

int
md_hasclreol()
{
  return cur_term != NULL &&
         cur_term->type.Strings != NULL &&
         clr_eol != NULL &&
         *clr_eol != 0;
}


const char *
md_getusername()
{
#define USERNAME_MAXLEN 80
  static char login[USERNAME_MAXLEN] = { '\0' };

  if (*login == '\0')
  {
    struct passwd *pw = getpwuid(getuid());

    strncpy(login, pw == NULL ? "nobody" : pw->pw_name, USERNAME_MAXLEN);
    login[USERNAME_MAXLEN -1] = '\0';
  }
  return login;
#undef USERNAME_MAXLEN
}

char *
md_gethomedir()
{
  static char homedir[PATH_MAX] = { '\0' };

  /* If we've already checked for homedir, we should know it by now */
  if (*homedir == '\0')
  {
    size_t len;
    struct passwd *pw = getpwuid(getuid());
    char *h = pw == NULL ? NULL : pw->pw_dir;

    if (h == NULL || *h == '\0' || !strcmp(h, "/"))
      h = getenv("HOME");
    if (h == NULL || !strcmp(h, "/"))
      h = "";

    strncpy(homedir, h, PATH_MAX);
    homedir[PATH_MAX -1] = '\0';

    len = strlen(homedir);
    if (len > 0 && len < PATH_MAX -1 &&
        homedir[len -1] != '/')
    {
      homedir[len] = '/';
      homedir[len +1] = '\0';
    }
  }

  return homedir;
}
