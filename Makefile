#
# Makefile for rogue
# @(#)Makefile	4.21 (Berkeley) 02/04/99
#
# Rogue: Exploring the Dungeons of Doom
# Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
# All rights reserved.
#
# See the file LICENSE.TXT for full copyright and licensing information.
#


DISTNAME = rogue14-mod1
PROGRAM  = rogue14
O        = o
HDRS	 = $(wildcard src/*.h)
OBJS     = $(addsuffix .o, $(basename $(wildcard src/*.c)))
CFILES   = vers.c extern.c armor.c chase.c command.c daemon.c \
	   daemons.c fight.c init.c io.c list.c mach_dep.c \
	   main.c  mdport.c misc.c monsters.c move.c new_level.c \
	   options.c pack.c passages.c potions.c rings.c rip.c \
	   rooms.c save.c scrolls.c state.c sticks.c things.c \
	   weapons.c wizard.c xcrypt.c
MISC_C   = findpw.c scedit.c scmisc.c
DOCSRC   = rogue.me.in rogue.6.in rogue.doc.in rogue.html.in rogue.cat.in
DOCS     = $(PROGRAM).doc $(PROGRAM).html $(PROGRAM).cat $(PROGRAM).me \
           $(PROGRAM).6
AFILES   = configure Makefile.in configure.ac config.h.in config.sub config.guess \
           install-sh rogue.6.in rogue.me.in rogue.html.in rogue.doc.in rogue.cat.in
MISC     = Makefile.std LICENSE.TXT rogue54.sln rogue54.vcproj rogue.spec \
           rogue.png rogue.desktop
CC       = gcc
CPPFLAGS =
CFLAGS   = -O3 -Wall -Wextra -Werror -pedantic
LDFLAGS  =
LIBS     = -lcurses
RM       = rm -f
MAKEFILE = -f Makefile.std
OUTFLAG  = -o
EXE      =

.SUFFIXES: .obj

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $*.o $*.c
    
$(PROGRAM): $(HDRS) $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(LIBS) $(OUTFLAG)$@$(EXE)
 
clean:
	$(RM) $(OBJS)
	$(RM) core a.exe a.out a.exe.stackdump $(PROGRAM) $(PROGRAM).exe $(PROGRAM).lck
	$(RM) $(PROGRAM).tar $(PROGRAM).tar.gz $(PROGRAM).zip 
	$(RM) $(DISTNAME)/*
	$(RM) $(DOCS)
    
dist.src:
	$(MAKE) $(MAKEFILE) clean
	mkdir $(DISTNAME)
	cp $(CFILES) $(HDRS) $(MISC) $(AFILES) $(DISTNAME)
	tar cf $(DISTNAME)-src.tar $(DISTNAME)
	gzip -f $(DISTNAME)-src.tar
	rm -fr $(DISTNAME)

findpw: findpw.c xcrypt.o mdport.o xcrypt.o
	$(CC) -s -o findpw findpw.c xcrypt.o mdport.o -lcurses

scedit: scedit.o scmisc.o vers.o mdport.o xcrypt.o
	$(CC) -s -o scedit vers.o scedit.o scmisc.o mdport.o xcrypt.o -lcurses

scmisc.o scedit.o:
	$(CC) -O -c $(SF) $*.c

doc.nroff:
	tbl rogue.me | nroff -me | colcrt - > rogue.doc
	nroff -man rogue.6 | colcrt - > rogue.cat

doc.groff:
	groff -P-c -t -me -Tascii rogue.me | sed -e 's/.\x08//g' > rogue.doc
	groff -man rogue.6 | sed -e 's/.\x08//g' > rogue.cat

fixdocs:
	sed -e 's/@PROGRAM@/$(PROGRAM)/' -e 's/@SCOREFILE@/$(SCOREFILE)/' rogue.6.in > $(PROGRAM).6
	sed -e 's/@PROGRAM@/$(PROGRAM)/' -e 's/@SCOREFILE@/$(SCOREFILE)/' rogue.me.in > $(PROGRAM).me
	sed -e 's/@PROGRAM@/$(PROGRAM)/' -e 's/@SCOREFILE@/$(SCOREFILE)/' rogue.html.in > $(PROGRAM).html
	sed -e 's/@PROGRAM@/$(PROGRAM)/' -e 's/@SCOREFILE@/$(SCOREFILE)/' rogue.doc.in > $(PROGRAM).doc
	sed -e 's/@PROGRAM@/$(PROGRAM)/' -e 's/@SCOREFILE@/$(SCOREFILE)/' rogue.cat.in > $(PROGRAM).cat

dist.linux:
	$(MAKE) $(MAKEFILE) clean
	$(MAKE) $(MAKEFILE) $(PROGRAM)
	tar cf $(DISTNAME)-linux.tar $(PROGRAM) LICENSE.TXT $(DOCS)
	gzip -f $(DISTNAME)-linux.tar
