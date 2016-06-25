#pragma once

// Game Options - These are set in main.c and options.c

extern bool fight_flush;  // Flush typeahead during battle
extern bool jump;         // Show running as a series of jumps
extern bool passgo;       // Follow the turnings in passageways
extern bool use_colors;   // Use ncurses colors

// Does the play want to automatically pick up items of given type?
bool option_autopickup(int type);

// Menu for changing options
bool option();
