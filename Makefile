
PROGRAM  = misty_mountains
PREFIX   = /usr/local
SCOREPATH = $(PREFIX)/share/$(PROGRAM)/highscore

CXX      = c++
CXXFLAGS = -O2 -Wall -Wextra -Werror -pedantic -std=c++11
DFLAGS   = -DSCOREPATH=\"$(SCOREPATH)\"
LDFLAGS  = -lcurses

CXXFILES = $(wildcard src/*.cc)
OBJS     = $(addsuffix .o, $(basename $(CXXFILES)))
MISC     = install CHANGELOG.TXT LICENSE.TXT

debug: CXX       = clang++
debug: CXXFLAGS  = -Weverything -Werror -g3 -std=c++11 -Wno-c++98-compat-pedantic -Wno-padded -Wno-c++11-compat -ferror-limit=1
debug: $(PROGRAM) ctags

.cc.o:
	$(CXX) $(CXXFLAGS) $(DFLAGS) -c -o $*.o $*.cc

$(PROGRAM): $(OBJS)
	$(CXX) -o $@ $(LDFLAGS) $(OBJS)

analyze:
	clang++ --analyze -Xanalyzer -analyzer-output=text $(CXXFLAGS) $(DFLAGS) src/*.cc
	$(RM) *.plist
.PHONY: analyze

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
	tar czf $(PROGRAM).tar.gz $(PROGRAM) $(MISC)
.PHONY: dist

lint:
	cppcheck --enable=all --std=c++11 -inconlusive src 2>lint.txt
.PHONY: lint

ctags:
	@type ctags >/dev/null 2>&1 && ctags -R * || true
.PHONY: ctags
