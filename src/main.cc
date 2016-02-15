#include <getopt.h>

#include <iostream>
#include <fstream>

#include "error_handling.h"
#include "game.h"
#include "command.h"
#include "io.h"
#include "score.h"
#include "misc.h"
#include "level_rooms.h"
#include "player.h"
#include "options.h"
#include "os.h"
#include "move.h"
#include "rogue.h"
#include "wizard.h"

using namespace std;


// Parse command-line arguments
static void
parse_args(int argc, char* const* argv, bool& restore, string& save_path, string& whoami)
{
  string const game_version = "Misty Mountains v2.0-alpha2-dev - Based on Rogue5.4.4";
  int option_index = 0;
  struct option const long_options[] = {
    {"no-colors", no_argument,       0, 'c'},
    {"escdelay",  optional_argument, 0, 'E'},
    {"flush",     no_argument,       0, 'f'},
    {"hide-floor",no_argument,       0, 'F'},
    {"no-jump",   no_argument,       0, 'j'},
    {"name",      required_argument, 0, 'n'},
    {"passgo",    no_argument,       0, 'p'},
    {"restore",   optional_argument, 0, 'r'},
    {"score",     no_argument,       0, 's'},
    {"seed",      required_argument, 0, 'S'},
    {"wizard",    no_argument,       0, 'W'},
    {"help",      no_argument,       0, '0'},
    {"dicerolls", no_argument,       0,  1 },
    {"version",   no_argument,       0, '1'},
    {0,           0,                 0,  0 }
  };

  // Global default options
  ESCDELAY = 0;

  // Set seed and dungeon number
  os_rand_seed = static_cast<unsigned>(time(nullptr) + getpid());

  for (;;)
  {
    int c = getopt_long(argc, argv, "cE::fjn:pr::sS:W",
                        long_options, &option_index);
    if (c == -1)
      break;

    switch (c)
    {
      case 'c': use_colors = false; break;
      case 'E': ESCDELAY = optarg == nullptr ? 64 : atoi(optarg); break;
      case 'f': fight_flush = true; break;
      case 'j': jump = false; break;
      case 'n': if (optarg != nullptr) {
                  whoami = optarg;
                } break;
      case 'p': passgo = true; break;
      case 'r': {
        restore = true;
        if (optarg != nullptr) {
          save_path = optarg;
        }
      } break;
      case 's': score_show_and_exit(0, -1, 0); // does not return
      case 'W': wizard = true; break;
      case 'S': if (wizard && optarg != nullptr) {
                  os_rand_seed = static_cast<unsigned>(stoul(optarg));
                } break;
      case   1: if (wizard) {
                  wizard_dicerolls = true;
                } break;
      case '0':
        cout << "Usage: " << argv[0] << " [OPTIONS] [FILE]\n"
             << "Run Rogue14 with selected options or a savefile\n\n"
             << "  -c, --no-colors      remove colors from the game\n"
             << "  -E, --escdelay=[NUM] set escdelay in ms. Not setting this\n"
             << "                       defaults to 0. If you do not give a NUM\n"
             << "                       argument, it's set to 64 (old standard)\n"
             << "  -f, --flush          flush typeahead during battle\n"
             << "  -j, --no-jump        draw each player step separately\n"
             << "  -n, --name=NAME      set highscore name\n"
             << "  -p, --passgo         follow the turnings in passageways\n"
             << "  -r, --restore        restore game instead of creating a new\n"
             << "  -s, --score          display the highscore and exit\n"
             << "  -W, --wizard         run the game in debug-mode\n"
             << "      --dicerolls      (wizard) show all dice rolls\n"
             << "  -S, --seed=NUMBER    (wizard) set map seed to NUMBER\n"
             << "      --help           display this help and exit\n"
             << "      --version        display game version and exit\n\n"
             << game_version
             << endl;
        exit(0);
      case '1':
        cout << game_version << endl;
        exit(0);
      default:
        cerr << "Try '" << argv[0] << " --help' for more information\n";
        exit(1);
    }
  }

  if (optind < argc)
  {
    cerr << "Try '" << argv[0] << " --help' for more information\n";
    exit(1);
  }
}

/** main:
 * The main program, of course */
int
main(int argc, char** argv)
{
  /* Open scoreboard, so we can modify the score later */
  score_open();

  bool   restore = false;
  string save_path;
  string whoami;

  /* Parse args and then init new (or old) game */
  parse_args(argc, argv, restore, save_path, whoami);

  if (whoami.empty()) {
    whoami = os_whoami();
  }

  if (save_path.empty()) {
    save_path = os_homedir() + ".misty_mountain.save";
  }


  Game* game = nullptr;
  if (restore) {
    ifstream savefile(save_path);
    if (!savefile) {
      cerr << save_path + ": " + strerror(errno) + "\n";
      return 1;
    }

    game = new Game(savefile);
    savefile.close();
    remove(save_path.c_str());
  } else {
    game = new Game(whoami, save_path);
  }


  if (game != nullptr) {
    return game->run();
  }

  return 0;
}

