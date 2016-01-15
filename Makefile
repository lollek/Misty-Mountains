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
VERSION  = 1.3.2+
PREFIX   = /usr/local
SCOREPATH = $(PREFIX)/share/$(PROGRAM)/highscore

CXX      = c++
CXXFLAGS = -O2 -Wall -Wextra -Werror -pedantic -std=c++11
DFLAGS   = -DVERSION=\"$(VERSION)\" -DSCOREPATH=\"$(SCOREPATH)\"
LDFLAGS  = -lcurses

CXXFILES = $(wildcard src/*.cc)
OBJS     = $(addsuffix .o, $(basename $(CXXFILES)))
MISC     = install CHANGELOG.TXT LICENSE.TXT rogue.png rogue.desktop

debug: CXX       = clang++
debug: CXXFLAGS  = -Weverything -g3 -std=c++11 -Wno-c++98-compat-pedantic -Wno-c++11-extensions -Wno-padded -Wno-c++11-compat -ferror-limit=1
# Temporary:
debug: CXXFLAGS += -Wno-global-constructors -Wno-exit-time-destructors
debug: $(PROGRAM) ctags

.cc.o:
	$(CXX) $(CXXFLAGS) $(DFLAGS) -c -o $*.o $*.cc

$(PROGRAM): $(OBJS)
	$(CXX) -o $@ $(LDFLAGS) $(OBJS)

clean:
	$(RM) $(OBJS) $(PROGRAM)
.PHONY: clean

final: CXXFLAGS += -DNDEBUG
final: clean $(PROGRAM)
.PHONY: final

install: $(PROGRAM)
	@PREFIX=$(PREFIX) ./install
.PHONY: install

remove:
	@PREFIX=$(PREFIX) ./remove
.PHONY: remove

dist: final
	tar czf $(PROGRAM)-$(VERSION)-linux.tar.gz $(PROGRAM) $(MISC)
.PHONY: dist

lint:
	cppcheck --enable=all --std=c++11 -inconlusive src 2>lint.txt
.PHONY: lint

ctags:
	@type ctags >/dev/null 2>&1 && ctags -R * || true
.PHONY: ctags
