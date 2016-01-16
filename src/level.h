#pragma once

#include <list>

#include "monster.h"
#include "io.h"

/* Flags for level map */
#define F_PASS		0x80		/* is a passageway */
#define F_SEEN		0x40		/* have seen this spot before */
#define F_DROPPED	0x20		/* object was dropped here */
#define F_LOCKED	0x20		/* door is locked */
#define F_REAL		0x10		/* what you see is what you get */
#define F_PNUM		0x0f		/* passage number mask */
#define F_TMASK		0x07		/* trap number mask */

/* describe a place on the level map */
typedef struct {
    char     p_ch;
    char     p_flags;
    Monster* p_monst;
} PLACE;

class Level {
public:
  Level(int relative_level); /* +1 for stairs up, -1 if you go down */
  ~Level() = default;

  int static constexpr amulet_min_level = 26;
  int static           levels_without_food;
  int static           max_level_visited;
  int static           current_level;

private:
};

extern PLACE              level_places[MAXLINES*MAXCOLS];  /* level map */
extern Coordinate         level_stairs;                    /* Location of staircase */
extern std::list<Item*>   level_items;                     /* List of items on level */

char level_get_type(int y, int x);

PLACE* level_get_place(int y, int x);

Monster* level_get_monster(int y, int x);
void level_set_monster(int y, int x, Monster* monster);

char level_get_flags(int y, int x);
void level_set_flags(int y, int x, char flags);

char level_get_ch(int y, int x);
void level_set_ch(int y, int x, char ch);
