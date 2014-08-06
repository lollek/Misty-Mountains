#include "ctype.h"
#include "string.h"
#include "stdlib.h"

#include "rogue.h"
#include "status_effects.h"
#include "potions.h"
#include "scrolls.h"
#include "traps.h"
#include "options.h"
#include "io.h"
#include "chase.h"
#include "pack.h"

#include "command_private.h"

bool
change_dungeon_level(char up_or_down)
{
  if (is_levitating(&player))
    msg("You can't. You're floating off the ground!");

  else if (chat(hero.y, hero.x) != STAIRS)
    msg("You're not standing on any stairs");

  else if (up_or_down == '>') /* DOWN */
  {
    level++;
    new_level();
  }

  else if (up_or_down == '<') /* UP */
  {
    if (player_has_amulet())
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
fight_monster(bool fight_to_death)
{
  THING *mp;

  kamikaze = fight_to_death;
  if (!get_dir())
    return false;
  delta.y += hero.y;
  delta.x += hero.x;

  mp = moat(delta.y, delta.x);
  if (mp == NULL || (!see_monst(mp) && !on(player, SEEMONST)))
  {
    msg(terse
        ? "no monster there"
        : "I see no monster there");
    return false;
  }
  else if (diag_ok(&hero, &delta))
  {
    to_death = true;
    max_hit = 0;
    mp->t_flags |= ISTARGET;
    runch = dir_ch;
    return do_command(dir_ch);
  }
  else
    return true;
}

bool
give_item_nickname(void)
{
  THING *obj = get_item("call", CALLABLE);
  char **guess;
  char *elsewise = NULL;
  bool already_known = false;
  char tmpbuf[MAXSTR] = { '\0' };

  if (obj == NULL)
    return false;

  switch (obj->o_type)
  {
    struct obj_info *op = NULL;

    case FOOD: msg("Don't play with your food!"); return false;

    case RING:
      op = &ring_info[obj->o_which];
      already_known = op->oi_know;
      guess = &op->oi_guess;
      elsewise = *guess ? *guess : r_stones[obj->o_which];

    when POTION:
      op = &pot_info[obj->o_which];
      already_known = op->oi_know;
      guess = &op->oi_guess;
      elsewise = *guess ? *guess : p_colors[obj->o_which];

    when SCROLL:
      op = &scr_info[obj->o_which];
      already_known = op->oi_know;
      guess = &op->oi_guess;
      elsewise = *guess ? *guess : s_names[obj->o_which];

    when STICK:
      op = &ws_info[obj->o_which];
      already_known = op->oi_know;
      guess = &op->oi_guess;
      elsewise = *guess ? *guess : ws_made[obj->o_which];

    otherwise:
      guess = &obj->o_label;
      elsewise = obj->o_label;
  }

  if (already_known)
  {
    msg("that has already been identified");
    return false;
  }

  if (elsewise != NULL && elsewise == *guess)
    msg("Was called \"%s\"", elsewise);

  msg(terse
      ? "call it? "
      : "What do you want to call it? ");

  if (readstr(tmpbuf) == 0)
  {
    if (*guess != NULL) {
      free(*guess);
      *guess = NULL;
    }
    if (strlen(tmpbuf) > 0)
    {
      *guess = malloc(strlen(tmpbuf) + 1);
      strcpy(*guess, tmpbuf);
    }
  }

  msg("");
  return false;
}

bool
identify_a_character(void)
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
identify_trap(void)
{
  if (get_dir())
  {
    char *fp;
    delta.y += hero.y;
    delta.x += hero.x;
    fp = &flat(delta.y, delta.x);
    if (!terse)
      addmsg("You have found ");
    if (chat(delta.y, delta.x) != TRAP)
      msg("no trap there");
    else if (is_hallucinating(&player))
      msg(trap_names[rnd(NTRAPS)]);
    else
    {
      msg(trap_names[*fp & F_TMASK]);
      *fp |= F_SEEN;
    }
  }
  return false;
}

bool
maybe_quit(void)
{
  quit(0);
  return false;
}

bool
pick_up_item_from_ground(void)
{
  const THING *obj = NULL;

  if (is_levitating(&player))
    msg("You can't. You're floating off the ground!");

  for (obj = lvl_obj; obj != NULL; obj = obj->l_next)
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
print_currently_wearing(char thing)
{
  bool item_found = false;

  if (!terse)
    addmsg("You are %s ", thing == WEAPON ? "wielding" : "wearing");

  if (thing == RING)
  {
    unsigned i;
    for (i = 0; i < CONCURRENT_RINGS; ++i)
      if (cur_ring[i]) {
        addmsg("%c) %s ", cur_ring[i]->o_packch, inv_name(cur_ring[i], true, 0));
        item_found = true;
      }
    if (item_found)
      endmsg();

  }
  else
  {
    THING *current_thing = thing == WEAPON ? cur_weapon : cur_armor;
    if (current_thing) {
      msg("%c) %s", current_thing->o_packch, inv_name(current_thing, true, 0));
      item_found = true;
    }
  }

  if (!item_found)
    msg("no %s", thing == WEAPON ? "weapon": thing == RING ? "rings":"armor");

  return false;
}

bool
print_help(void)
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

bool
repeat_last_command(void)
{
  if (last_comm != '\0')
  {
    again = true;
    return do_command(last_comm);
  }
  return false;
}

bool
search(void)
{
  int y, x;
  int probinc = (is_hallucinating(&player) ? 3:0) + is_blind(&player) ? 2:0;
  bool found = false;

  for (y = hero.y - 1; y <= hero.y + 1; y++)
    for (x = hero.x - 1; x <= hero.x + 1; x++)
    {
      char *fp = &flat(y, x);
      char chatyx = chat(y, x);

      /* Real wall/floor/shadow */
      if (*fp & F_REAL)
        continue;

      /* Wall */
      if ((chatyx == VWALL || chatyx == HWALL) &&
          rnd(5 + probinc) == 0)
      {
        chat(y, x) = DOOR;
        msg("a secret door");
        found = true;
        *fp |= F_REAL;
      }

      /* Floor */
      if (chatyx == FLOOR && rnd(2 + probinc) == 0)
      {
        chat(y, x) = TRAP;

        if (!terse)
          addmsg("you found ");

        if (is_hallucinating(&player))
          msg(trap_names[rnd(NTRAPS)]);
        else {
          msg(trap_names[*fp & F_TMASK]);
          *fp |= F_SEEN;
        }

        found = true;
        *fp |= F_REAL;
      }

      /* Shadow */
      if (chatyx == SHADOW && rnd(3 + probinc) == 0)
      {
        chat(y, x) = PASSAGE;
        found = true;
        *fp |= F_REAL;
      }
    }
  if (found)
  {
    look(false);
    count = false;
    running = false;
  }
  return true;
}

bool
show_players_inventory(void)
{
  print_inventory(0);
  msg("--Press any key to continue--");
  getch();
  clear_inventory();
  msg("");
  return false;
}

bool
toggle_wizard_mode(void)
{
  /* TODO: Add a query here, so you always can become a wiz */
  if (potential_wizard)
  {
    wizard = !wizard;
    turn_see(!wizard);
    if (wizard)
      msg("You are one with the force (seed: #%d)", seed);
    else
      msg("not wizard any more");
  }
  return false;
}


