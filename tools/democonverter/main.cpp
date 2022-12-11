#include "demogobbler.h"
#include "demogobbler/freddie.hpp"
#include <cstdio>

int main(int argc, char **argv) {
  if (argc <= 3) {
    printf("Usage: democonverter <example file> <input file> <output file>\n");
    return 0;
  }

  freddie::demo_t example;
  freddie::demo_t input;
  auto result = freddie::demo_t::parse_demo(&example, argv[1]);

  if(result.error)
  {
    std::printf("error parsing example demo: %s\n", result.error_message);
    return 1;
  }

  result = freddie::demo_t::parse_demo(&input, argv[2]);

  if(result.error)
  {
    std::printf("error parsing input demo: %s\n", result.error_message);
    return 1;
  }

  result = freddie::convert_demo(&example, &input);

  if(result.error)
  {
    std::printf("error converting demo: %s\n", result.error_message);
    return 1;
  }

  result = input.write_demo(argv[3]);

  if(result.error)
  {
    std::printf("error writing output demo: %s\n", result.error_message);
    return 1;
  }

  return 0;
}