#include "memory_stream.hpp"
#include "gtest/gtest.h"

wrapped_memory_stream::wrapped_memory_stream()
{
  underlying.errfunc = std::bind(&wrapped_memory_stream::error, this, std::placeholders::_1);
}

void wrapped_memory_stream::error(const char* msg)
{
  EXPECT_TRUE(false) << msg;
}