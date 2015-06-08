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


PROGRAM  = rogue14
VERSION  = 1.3.1+
PREFIX   = /usr/local
SCOREPATH = $(PREFIX)/share/$(PROGRAM)/rogue14.highscore

CC       = cc
CFLAGS   = -O3 -Wall -Wextra -Werror -pedantic -std=c99
DFLAGS   = -DVERSION=\"$(VERSION)\" -DSCOREPATH=\"$(SCOREPATH)\"
LDFLAGS  = -lcurses

CFILES   = $(wildcard src/*.c)
OBJS     = $(addsuffix .o, $(basename $(CFILES)))
DOCSRC   = $(wildcard docsrc/*)
DOCS     = $(notdir $(DOCSRC))
MISC     = install CHANGELOG.TXT LICENSE.TXT rogue.png rogue.desktop

debug: CC      = clang
debug: CFLAGS += -Weverything -g3 -Wno-padded -Wno-disabled-macro-expansion
debug: $(PROGRAM)

.c.o:
	$(CC) $(CFLAGS) $(DFLAGS) -c -o $*.o $*.c

$(PROGRAM): $(OBJS)
	$(CC) -o $@ $(LDFLAGS) $(OBJS)

clean:
	$(RM) $(OBJS) $(PROGRAM) $(DOCS)

final: CFLAGS += -DNDEBUG
final: clean $(PROGRAM)

doc.nroff:
	tbl rogue.me | nroff -me | colcrt - > rogue.doc
	nroff -man rogue.6 | colcrt - > rogue.cat

doc.groff:
	groff -P-c -t -me -Tascii rogue.me | sed -e 's/.\x08//g' > rogue.doc
	groff -man rogue.6 | sed -e 's/.\x08//g' > rogue.cat

docs:
	$(foreach doc, $(wildcard docsrc/*), \
	  sed -e 's/@PROGRAM@/$(PROGRAM)/' -e 's/@SCOREFILE@/.rogue14_highscore/' \
	  $(doc) > $(notdir $(doc));)

install: $(PROGRAM) docs
	@PREFIX=$(PREFIX) ./install

remove:
	@PREFIX=$(PREFIX) ./remove

dist: final docs
	tar czf $(PROGRAM)-$(VERSION)-linux.tar.gz $(PROGRAM) $(DOCS) $(MISC)

lint:
	cppcheck --enable=all --std=c99 -inconlusive src 2>lint.txt

.PHONY: clean final doc.nroff doc.groff install remove dist
