#include "demogobbler/freddie.hpp"
#include <stdio.h>

// TODO: This is still completely broken at doesnt work at all
using namespace freddie;

int main(int argc, char **argv) {
  if (argc < 3) {
    printf("Usage: demosplicer <output filepath> <input 1> <input 2> ...");
    return 0;
  }

  dg_parse_result result = splice_demos(argv[1], (const char **)argv + 2, argc - 2);

  if (result.error) {
    printf("Error splicing demos: %s\n", result.error_message);
  }

  return 0;
}