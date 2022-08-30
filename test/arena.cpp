#include "gtest/gtest.h"
#include "demogobbler_arena.h"

TEST(Arena, Int32Works) {
  arena a = demogobbler_arena_create(4);
  ASSERT_EQ(a.block_count, 0);
  uint32_t* ptr = (uint32_t*)demogobbler_arena_allocate(&a, 4, 4);
  ASSERT_NE(ptr, nullptr);
  *ptr = 1;
  ASSERT_EQ(a.block_count, 1);
  ASSERT_EQ(a.blocks[0].bytes_allocated, 4);
  demogobbler_arena_free(&a);
}

TEST(Arena, AutoExpandWorks) {
  arena a = demogobbler_arena_create(4096);
  const int COUNT = 100000;
  uint32_t* pointers[COUNT];

  for(int i=0; i < COUNT; ++i) {
    pointers[i] = (uint32_t*)demogobbler_arena_allocate(&a, 4, 4);
    *(pointers[i]) = i; 
  }

  for(int i=0; i < COUNT; ++i) {
    EXPECT_EQ(*(pointers[i]), i); 
  }

  demogobbler_arena_free(&a);
}
