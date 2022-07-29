#include "gtest/gtest.h"
#include "freddie.h"
#include "test_demos.hpp"

TEST(Freddie, Works) {
  for(auto& demo : get_test_demos())
  {
    std::cout << "[----------] " << demo << std::endl;
    freddie_demo d;
    auto result = freddie_parse_file(demo.c_str(), &d);
    EXPECT_EQ(result.error, false);

    for(size_t i=0; i < d.message_count; ++i) {
      freddie_demo_message msg = d.messages[i];
      int brk = 1;
    }
    
    freddie_free_demo(&d);
  }
}
