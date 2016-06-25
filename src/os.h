#pragma once

#include <string>

extern unsigned os_rand_seed;

int os_rand(void);                 // Return a pseudorandom number
int os_rand_range(int max);        // Return a number [0,max[
size_t os_rand_range(size_t max);  // Return a number [0,max[

int os_process_id();  // Get process PID

int os_usleep(unsigned int usec);  // Sleep for nanoseconds
std::string os_whoami();           // Return name for player
std::string os_homedir();          // Return user's home directory
