#include "gtest/gtest.h"
extern "C" {
#include "demogobbler/allocator.h"
}

TEST(Arena, Int32Works) {
  dg_arena a = dg_arena_create(4);
  ASSERT_EQ(a.block_count, 0);
  uint32_t *ptr = (uint32_t *)dg_arena_allocate(&a, 4, 4);
  ASSERT_NE(ptr, nullptr);
  *ptr = 1;
  ASSERT_EQ(a.block_count, 1);
  ASSERT_EQ(a.blocks[0].bytes_used, 4);
  dg_arena_free(&a);
}

TEST(Arena, AutoExpandWorks) {
  dg_arena a = dg_arena_create(4096);
  const int COUNT = 100000;
  uint32_t *pointers[COUNT];

  for (int i = 0; i < COUNT; ++i) {
    pointers[i] = (uint32_t *)dg_arena_allocate(&a, 4, 4);
    *(pointers[i]) = i;
  }

  for (int i = 0; i < COUNT; ++i) {
    EXPECT_EQ(*(pointers[i]), i);
  }

  dg_arena_free(&a);
}

TEST(Arena, ReallocWorks) {
  dg_arena a = dg_arena_create(8);
  uint8_t *ptr = (uint8_t *)dg_arena_allocate(&a, 8, 1);

  for (size_t i = 0; i < 8; ++i) {
    ptr[i] = i;
  }

  uint8_t *newptr = (uint8_t *)dg_arena_reallocate(&a, ptr, 8, 16, 1);
  EXPECT_NE(newptr, ptr);

  for (size_t i = 0; i < 8; ++i) {
    EXPECT_EQ(newptr[i], i);
  }

  for (size_t i = 8; i < 16; ++i) {
    newptr[i] = i;
  }

  dg_arena_free(&a);
}

TEST(Arena, ReallocWorksInPlace) {
  dg_arena a = dg_arena_create(16);
  uint8_t *ptr = (uint8_t *)dg_arena_allocate(&a, 8, 1);

  for (size_t i = 0; i < 8; ++i) {
    ptr[i] = i;
  }

  uint8_t *newptr = (uint8_t *)dg_arena_reallocate(&a, ptr, 8, 16, 1);
  EXPECT_EQ(newptr, ptr);

  for (size_t i = 0; i < 8; ++i) {
    EXPECT_EQ(ptr[i], i);
  }

  for (size_t i = 8; i < 16; ++i) {
    ptr[i] = i;
  }

  dg_arena_free(&a);
}

TEST(Arena, ClearWorks) {
  dg_arena a = dg_arena_create(4096);
  const int COUNT = 100000;

  for (int i = 0; i < COUNT; ++i) {
    dg_arena_allocate(&a, 4, 4);
  }

  size_t bytes_in_array = sizeof(dg_arena_block) * a.block_count;
  dg_arena_block *expected_blocks = (dg_arena_block *)malloc(bytes_in_array);
  size_t expected_block_count = a.block_count;
  memcpy(expected_blocks, a.blocks, bytes_in_array);
  dg_arena_clear(&a);

  for (int i = 0; i < COUNT; ++i) {
    dg_arena_allocate(&a, 4, 4);
  }

  EXPECT_EQ(memcmp(expected_blocks, a.blocks, bytes_in_array), 0);
  EXPECT_EQ(a.block_count, expected_block_count);

  dg_arena_free(&a);
  free(expected_blocks);
}
