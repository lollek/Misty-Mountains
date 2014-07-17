#include "ctype.h"

#include "rogue.h"
#include "status_effects.h"

#include "command_sub.h"

bool
change_dungeon_level(bool up_or_down)
{
  if (is_levitating(&player))
    msg("You can't. You're floating off the ground!");

  else if (chat(hero.y, hero.x) != STAIRS)
    msg("You're not standing on any stairs");

  else if (up_or_down == DOWN)
  {
    level++;
    seenstairs = false;
    new_level();
  }

  else if (up_or_down == UP)
  {
    if (amulet)
    {
      level--;
      if (level == 0)
        total_winner();
      new_level();
      msg("you feel a wrenching sensation in your gut");
    }
    else
      msg("your way is magically blocked");
  }

  return false;
}

bool
identify_a_character()
{
  int ch;
  const struct h_list ident_list[] = {
    {VWALL,	"wall of a room",	false},
    {HWALL,	"wall of a room",	false},
    {GOLD,	"gold",			false},
    {STAIRS,	"a staircase",		false},
    {DOOR,	"door",			false},
    {FLOOR,	"room floor",		false},
    {PLAYER,	"you",			false},
    {PASSAGE,	"passage",		false},
    {TRAP,	"trap",			false},
    {POTION,	"potion",		false},
    {SCROLL,	"scroll",		false},
    {FOOD,	"food",			false},
    {WEAPON,	"weapon",		false},
    {SHADOW,	"solid rock",		false},
    {ARMOR,	"armor",		false},
    {AMULET,	"the Amulet of Yendor",	false},
    {RING,	"ring",			false},
    {STICK,	"wand or staff",	false},
    {'\0'}
  };

  msg("what do you want identified? ");
  ch = readchar();
  mpos = 0;

  if (ch == KEY_ESCAPE)
  {
    msg("");
    return false;
  }

  if (isalpha(ch))
  {
    ch = toupper(ch);
    msg("'%s': %s", unctrl(ch), monsters[ch - 'A'].m_name);
    return false;
  }
  else
  {
    const struct h_list *hp;
    for (hp = ident_list; hp->h_ch != '\0'; hp++)
      if (hp->h_ch == ch)
      {
        msg("'%s': %s", unctrl(ch), hp->h_desc);
        return false;
      }
  }

  msg("'%s': %s", unctrl(ch), "unknown character");
  return false;
}

bool
pick_up_item_from_ground()
{
  const THING *obj = NULL;

  if (is_levitating(&player))
    msg("You can't. You're floating off the ground!");

  for (obj = lvl_obj; obj != NULL; obj = next(obj))
    if (obj->o_pos.y == hero.y && obj->o_pos.x == hero.x)
    {
      pick_up(obj->o_type);
      return true;
    }

  msg(terse
      ? "nothing here"
      : "there is nothing here to pick up");
  return false;
}

bool
print_help()
{
  const struct h_list helpstr[] = {
    {'?',	"	prints help",				true},
    {'/',	"	identify object",			true},
    {'h',	"	left",					true},
    {'j',	"	down",					true},
    {'k',	"	up",					true},
    {'l',	"	right",					true},
    {'y',	"	up & left",				true},
    {'u',	"	up & right",				true},
    {'b',	"	down & left",				true},
    {'n',	"	down & right",				true},
    {'H',	"	run left",				false},
    {'J',	"	run down",				false},
    {'K',	"	run up",				false},
    {'L',	"	run right",				false},
    {'Y',	"	run up & left",				false},
    {'U',	"	run up & right",			false},
    {'B',	"	run down & left",			false},
    {'N',	"	run down & right",			false},
    {CTRL('H'),	"	run left until adjacent",		false},
    {CTRL('J'),	"	run down until adjacent",		false},
    {CTRL('K'),	"	run up until adjacent",			false},
    {CTRL('L'),	"	run right until adjacent",		false},
    {CTRL('Y'),	"	run up & left until adjacent",		false},
    {CTRL('U'),	"	run up & right until adjacent",		false},
    {CTRL('B'),	"	run down & left until adjacent",	false},
    {CTRL('N'),	"	run down & right until adjacent",	false},
    {'\0',	"	<SHIFT><dir>: run that way",		true},
    {'\0',	"	<CTRL><dir>: run till adjacent",	true},
    {'f',	"<dir>	fight till death or near death",	true},
    {'t',	"<dir>	throw something",			true},
    {'m',	"<dir>	move onto without picking up",		true},
    {'z',	"<dir>	zap a wand in a direction",		true},
    {'^',	"<dir>	identify trap type",			true},
    {'s',	"	search for trap/secret door",		true},
    {'>',	"	go down a staircase",			true},
    {'<',	"	go up a staircase",			true},
    {'.',	"	rest for a turn",			true},
    {',',	"	pick something up",			true},
    {'i',	"	inventory",				true},
    {'I',	"	inventory single item",			true},
    {'q',	"	quaff potion",				true},
    {'r',	"	read scroll",				true},
    {'e',	"	eat food",				true},
    {'w',	"	wield a weapon",			true},
    {'W',	"	wear armor",				true},
    {'T',	"	take armor off",			true},
    {'P',	"	put on ring",				true},
    {'R',	"	remove ring",				true},
    {'d',	"	drop object",				true},
    {'c',	"	call object",				true},
    {'a',	"	repeat last command",			true},
    {')',	"	print current weapon",			true},
    {']',	"	print current armor",			true},
    {'=',	"	print current rings",			true},
    {'@',	"	print current stats",			true},
    {'D',	"	recall what's been discovered",		true},
    {'o',	"	examine/set options",			true},
    {CTRL('R'),	"	redraw screen",				true},
    {CTRL('P'),	"	repeat last message",			true},
    {KEY_ESCAPE,"	cancel command",			true},
    {'S',	"	save game",				true},
    {'Q',	"	quit",					true},
    {'!',	"	shell escape",				true},
    {'F',	"<dir>	fight till either of you dies",		true},
    { 0 ,		NULL,					false}
  };
  const struct h_list *strp;
  char helpch;
  int numprint;
  int cnt;

  msg("character you want help for (* for all): ");
  helpch = readchar();
  mpos = 0;

  /* If its not a *, print the right help string
   * or an error if he typed a funny character. */
  if (helpch != '*')
  {
    move(0, 0);
    for (strp = helpstr; strp->h_desc != NULL; strp++)
      if (strp->h_ch == helpch)
      {
        msg("%s)%s", unctrl(strp->h_ch), strp->h_desc);
        return false;
      }
    msg("unknown character '%s'", unctrl(helpch));
    return false;
  }

  /* Here we print help for everything.
   * Then wait before we return to command mode */

  numprint = 0;
  for (strp = helpstr; strp->h_desc != NULL; strp++)
    if (strp->h_print)
      numprint++;
  if (numprint & 01)		/* round odd numbers up */
    numprint++;
  numprint /= 2;
  if (numprint > LINES - 1)
    numprint = LINES - 1;

  wclear(hw);
  cnt = 0;
  for (strp = helpstr; strp->h_desc != NULL; strp++)
    if (strp->h_print)
    {
      wmove(hw, cnt % numprint, cnt >= numprint ? COLS / 2 : 0);
      if (strp->h_ch)
        waddstr(hw, unctrl(strp->h_ch));
      waddstr(hw, strp->h_desc);
      if (++cnt >= numprint * 2)
        break;
    }
  wmove(hw, LINES - 1, 0);
  waddstr(hw, "--Press space to continue--");
  wrefresh(hw);
  wait_for(KEY_SPACE);
  clearok(stdscr, true);

  msg("");
  touchwin(stdscr);
  wrefresh(stdscr);
  return false;
}


