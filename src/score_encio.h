#pragma once

#include <cstdio>

size_t encwrite(char const* start, size_t size, FILE* outf);
size_t encread(char* start, size_t size, FILE* inf);
