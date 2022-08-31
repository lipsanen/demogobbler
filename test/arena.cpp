#include "gtest/gtest.h"
extern "C" {
#include "arena.h"
}

TEST(Arena, Int32Works) {
  arena a = demogobbler_arena_create(4);
  ASSERT_EQ(a.block_count, 0);
  uint32_t* ptr = (uint32_t*)demogobbler_arena_allocate(&a, 4, 4);
  ASSERT_NE(ptr, nullptr);
  *ptr = 1;
  ASSERT_EQ(a.block_count, 1);
  ASSERT_EQ(a.blocks[0].bytes_used, 4);
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

TEST(Arena, ClearWorks) {
  arena a = demogobbler_arena_create(4096);
  const int COUNT = 100000;

  for(int i=0; i < COUNT; ++i) {
    demogobbler_arena_allocate(&a, 4, 4);
  }

  size_t bytes_in_array = sizeof(demogobbler_arena_block) * a.block_count;
  demogobbler_arena_block* expected_blocks = (demogobbler_arena_block*)malloc(bytes_in_array);
  size_t expected_block_count = a.block_count;
  memcpy(expected_blocks, a.blocks, bytes_in_array);
  demogobbler_arena_clear(&a);

  for(int i=0; i < COUNT; ++i) {
    demogobbler_arena_allocate(&a, 4, 4);
  }

  EXPECT_EQ(memcmp(expected_blocks, a.blocks, bytes_in_array), 0);
  EXPECT_EQ(a.block_count, expected_block_count);

  demogobbler_arena_free(&a);
  free(expected_blocks);
}
