#pragma once

#include <cstdlib>
#include "demogobbler/freddie.hpp"

struct wrapped_memory_stream {
  freddie::memory_stream underlying;
  wrapped_memory_stream();
  void error(const char* err);
};
