#include "demogobbler.h"
#include "copy.hpp"
#include "gtest/gtest.h"
#include "test_demos.hpp"

TEST(E2E, copy_demos) {
  for(auto& demo : get_test_demos())
  {
    std::cout << "[----------] " << demo << std::endl;
    copy_demo_test(demo.c_str());
  }
}
