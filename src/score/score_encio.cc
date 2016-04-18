#include "score/encio.h"

static char const encstr[] {
    "\300k||`\251Y.'\305\321\201+\277~r\"]\240_\223=1\341)\222\212\241t;\t$"
    "\270\314/<#\201\254"};
static char const statlist[] {
    "\355kl{+\204\255\313idJ\361\214=4:\311\271\341wK<\312\321\213,,7\271/"
    "Rk%\b\312\f\246"};

size_t encwrite(char const* start, size_t size, FILE* outf) {
  char const* e1{encstr};
  char const* e2{statlist};
  char fb{0};
  size_t i;

  for (i = size; i > 0; --i) {
    if (putc(*start++ ^ *e1 ^ *e2 ^ fb, outf) == EOF) { break; }

    fb += *e1++ * *e2++;
    if (*e1 == '\0') { e1 = encstr; }
    if (*e2 == '\0') { e2 = statlist; }
  }

  return size - i;
}

size_t encread(char* start, size_t size, FILE* inf) {
  char const* e1{encstr};
  char const* e2{statlist};
  char fb{0};

  size_t read_size{fread(start, 1, size, inf)};
  if (read_size == 0) { return 0; }

  while (size--) {
    *start++ ^= *e1 ^ *e2 ^ fb;
    fb += *e1++ * *e2++;
    if (*e1 == '\0') { e1 = encstr; }
    if (*e2 == '\0') { e2 = statlist; }
  }

  return read_size;
}
