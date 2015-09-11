#ifndef ROGUE14_STATE_H
#define ROGUE14_STATE_H

#include <stdint.h>
#include <stdio.h>

#include "things.h"

bool state_save_string(char const* s);
bool state_load_string(char** s);

bool state_save_index(char const** start, int max, char const* str);
bool state_load_index(char const** start, int max, char const** str);

bool state_save_obj_info(struct obj_info const* i, int count);
bool state_load_obj_info(struct obj_info* i,       int count);

bool state_save_file(FILE* savef);
bool state_load_file(FILE* inf);

bool state_save_int8(int8_t data);
bool state_load_int8(int8_t* data);

bool state_save_int32(int32_t data);
bool state_load_int32(int32_t* data);
bool state_assert_int32(int32_t data);

bool state_load_coord(coord* c);
bool state_save_coord(coord const* c);

bool state_save_struct_damage(struct damage const* dmg);
bool state_load_struct_damage(struct damage* dmg);

bool state_save_structs_damage(struct damage const dmg[MAXATTACKS]);
bool state_load_structs_damage(struct damage dmg[MAXATTACKS]);

bool state_save_bools(bool const* c, int32_t count);
bool state_load_bools(bool* c, int32_t count);

bool state_save_list(THING const* list);
bool state_load_list(THING** list);

bool state_save_thing(THING const* thing);
bool state_load_thing(THING* thing);

static int32_t const RSID_PACK         = 0x1011E001;
static int32_t const RSID_POTIONS      = 0x1011E002;
static int32_t const RSID_RINGS        = 0x1011E003;
static int32_t const RSID_SCROLLS      = 0x1011E004;
static int32_t const RSID_WANDS        = 0x1011E005;
static int32_t const RSID_START        = 0x1011E006;
static int32_t const RSID_END          = 0x1011E007;
static int32_t const RSID_PLAYER       = 0x1011E008;
static int32_t const RSID_LEVEL        = 0x1011E009;
static int32_t const RSID_FOOD         = 0x1011E00A;

static int32_t const RSID_STATS        = 0x1011EA01;
static int32_t const RSID_THING        = 0x1011EA02;
static int32_t const RSID_OBJECT       = 0x1011EA03;
static int32_t const RSID_MAGICITEMS   = 0x1011EA04;
static int32_t const RSID_OBJECTLIST   = 0x1011EA05;
static int32_t const RSID_MONSTERLIST  = 0x1011EA09;
static int32_t const RSID_WINDOW       = 0x1011EA0D;
static int32_t const RSID_DAEMONS      = 0x1011EA0E;
static int32_t const RSID_ARMOR        = 0x1011EA10;
static int32_t const RSID_RHAND        = 0x1011EA11;
static int32_t const RSID_RRING        = 0x1011EA13;
static int32_t const RSID_LRING        = 0x1011E012;
static int32_t const RSID_NULL         = 0x1011E000;

#endif /* ROGUE14_STATE_H */
