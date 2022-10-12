#include "gtest/gtest.h"

extern "C" {
#include "vector_array.h"
}

TEST(dg_vector_array, create) {
  uint32_t array[500];
  dg_vector_array vec = dg_va_create(array, uint32_t);
  EXPECT_EQ(vec.allocated_by_malloc, false);
  EXPECT_EQ(vec.bytes_per_element, sizeof(uint32_t));
  EXPECT_EQ(vec.count_elements, 0);
  EXPECT_EQ(vec.ptr_bytes, 500 * sizeof(uint32_t));
  EXPECT_EQ(vec.ptr, array);
  dg_va_free(&vec);
}

TEST(dg_vector_array, push_back) {
  uint32_t array[500];
  dg_vector_array vec = dg_va_create(array, uint32_t);
  for (uint32_t i = 0; i < 501; ++i)
    dg_va_push_back(&vec, &i);

  uint32_t *ptr = (uint32_t *)dg_va_indexptr(&vec, 0);
  for (uint32_t i = 0; i < 501; ++i) {
    EXPECT_EQ(ptr[i], i);
  }

  EXPECT_NE(ptr, array);

  dg_va_free(&vec);
}

TEST(dg_vector_array, clear) {
  uint32_t array[500];
  dg_vector_array vec = dg_va_create(array, uint32_t);
  for (uint32_t i = 0; i < 500; ++i)
    dg_va_push_back(&vec, &i);

  dg_va_clear(&vec);
  EXPECT_EQ(vec.count_elements, 0);
}

TEST(dg_vector_array, nullptr_works) {
  dg_vector_array vec = dg_va_create_(nullptr, 0, 4, 4);
  int value = 4;
  dg_va_push_back(&vec, &value);
  ASSERT_NE(vec.ptr, nullptr);
  EXPECT_EQ(value, ((int *)vec.ptr)[0]);
  dg_va_free(&vec);
}
