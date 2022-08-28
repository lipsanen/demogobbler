#include "gtest/gtest.h"

extern "C" {
  #include "vector_array.h"
}

TEST(vector_array, create) {
  uint32_t array[500];
  vector_array vec = demogobbler_va_create(array);
  EXPECT_EQ(vec.allocated_by_malloc, false);
  EXPECT_EQ(vec.bytes_per_element, sizeof(uint32_t));
  EXPECT_EQ(vec.count_elements, 0);
  EXPECT_EQ(vec.ptr_bytes, 500 * sizeof(uint32_t));
  EXPECT_EQ(vec.ptr, array);
  demogobbler_va_free(&vec);
}

TEST(vector_array, push_back) {
  uint32_t array[500];
  vector_array vec = demogobbler_va_create(array);
  for(uint32_t i=0; i < 501; ++i)
    demogobbler_va_push_back(&vec, &i);
  
  uint32_t* ptr = (uint32_t*)demogobbler_va_indexptr(&vec, 0);
  for(uint32_t i=0; i < 501; ++i) {
    EXPECT_EQ(ptr[i], i);
  }

  EXPECT_NE(ptr, array);

  demogobbler_va_free(&vec);
}


TEST(vector_array, clear) {
  uint32_t array[500];
  vector_array vec = demogobbler_va_create(array);
  for(uint32_t i=0; i < 500; ++i)
    demogobbler_va_push_back(&vec, &i);
  
  demogobbler_va_clear(&vec);
  EXPECT_EQ(vec.count_elements, 0);
}

TEST(vector_array, nullptr_works) {
  vector_array vec = demogobbler_va_create_(nullptr, 0, 4, 4);
  int value = 4;
  demogobbler_va_push_back(&vec, &value);
  ASSERT_NE(vec.ptr, nullptr);
  EXPECT_EQ(value, ((int*)vec.ptr)[0]);
  demogobbler_va_free(&vec);
}
