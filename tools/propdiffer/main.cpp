#include "demogobbler.h"
#include "demogobbler/freddie.hpp"
#include <cstdio>
#include <stdarg.h>

const char* firstdemo = nullptr;
const char* seconddemo = nullptr;

static void print_msg(bool first, const char* format, ...)
{
  va_list args;
  va_start(args, format);
  printf("%s: ", first ? firstdemo : seconddemo);
  vprintf(format, args);
  va_end(args);
}

int main(int argc, char **argv) {
  if (argc <= 2) {
    printf("Usage: ddiff <example file> <input file>\n");
    return 0;
  }

  freddie::demo_t lhs;
  freddie::demo_t rhs;
  firstdemo = argv[1];
  seconddemo = argv[2];
  auto result = freddie::demo_t::parse_demo(&lhs, argv[1]);

  if(result.error)
  {
    std::printf("error parsing 1st demo: %s\n", result.error_message);
    return 1;
  }

  result = freddie::demo_t::parse_demo(&rhs, argv[2]);

  if(result.error)
  {
    std::printf("error parsing 2nd demo: %s\n", result.error_message);
    return 1;
  }

  freddie::compare_props_args args;
  args.first = &lhs;
  args.second = &rhs;
  args.func = print_msg;
  freddie::compare_props(&args);

  return 0;
}