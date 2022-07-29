#include "freddie.h"
#include <stdio.h>

// TODO: This is still completely broken at doesnt work at all

int main(int argc, char **argv) {
  if (argc < 3) {
    printf("Usage: demosplicer <output filepath> <input 1> <input 2> ...");
    return 0;
  }

  demogobbler_parse_result result = freddie_splice_demos(argv[1], (const char**)argv + 2, argc - 2);

  if(result.error) {
    printf("Error splicing demos: %s\n", result.error_message);
  }

  return 0;
}