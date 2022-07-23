#include "freddie.h"
#include "copy.hpp"
#include "gtest/gtest.h"
#include "test_demos.hpp"

TEST(freddie, works) {
  for(auto& demo : get_test_demos())
  {
    std::cout << "[----------] " << demo << std::endl;
    auto output = demogobbler_freddie_file(demo.c_str());
    demogobbler_freddie_free(&output);
  }
}
