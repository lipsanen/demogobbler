#include <cstdint>
#include "demogobbler_bitstream.h"
#include <cmath>
#include "gtest/gtest.h"

template<typename T>
bool test_num(T num, T& got, unsigned int bits, unsigned int bitOffset) {
  T orig = num;
  num <<= bitOffset;
  bitstream stream = demogobbler_bitstream_create(&num, sizeof(T) * 8);
  if(bitOffset > 0)
  {
    T offset_number = demogobbler_bitstream_read_uint(&stream, bitOffset);

    if(offset_number != 0) {
      got = -1;
      return false;
    }
  }

  got = demogobbler_bitstream_read_uint(&stream, bits);

  if(got != orig) {
      return false;
  }

  return true;
}

template<typename T>
bool test_signum(T num, T& got, unsigned int bits, unsigned int bitOffset) {
  T orig = num;
  num <<= bitOffset;
  bitstream stream = demogobbler_bitstream_create(&num, 16 * 8);
  if(bitOffset > 0)
  {
    T offset_number = demogobbler_bitstream_read_sint(&stream, bitOffset);

    if(offset_number != 0) {
      got = -1;
      return false;
    }
  }

  got = demogobbler_bitstream_read_sint(&stream, bits);

  if(got != orig) {
      return false;
  }

  return true;
}

TEST(Bitstream, Signed64Works) {
  int64_t got;
  EXPECT_EQ(test_signum<int64_t>(-2147483648, got, 64, 0), true);
  EXPECT_EQ(test_signum<int64_t>(0, got, 64, 0), true);
  EXPECT_EQ(test_signum<int64_t>(2147483647, got, 64, 0), true);
}

TEST(Bitstream, Unsigned64Works) {
  uint64_t got;
  EXPECT_EQ(test_num<uint64_t>(1, got, 64, 0), true);
  EXPECT_EQ(test_num<uint64_t>(1, got, 32, 0), true);
  EXPECT_EQ(test_num<uint64_t>(1, got, 32, 1), true);
  EXPECT_EQ(test_num<uint64_t>(128, got, 32, 1), true);
  EXPECT_EQ(test_num<uint64_t>(1, got, 5, 0), true);

 /* for(int offset=0; offset < 64; ++offset) {
    for(int i=0; i < 64 - offset; ++i) {
      uint64_t value = 1ULL << i;
      EXPECT_EQ(test_num(value, got, i + 1, offset), true) << " offset " << offset << " : " << got  << " != " << value << " failed.";
    }
  } */
}

TEST(Bitstream, AlignedCString) {
  const char* string = "this is a wonderful string.";
  char buffer[80];
  bitstream stream = demogobbler_bitstream_create(const_cast<void*>((const void*)string), strlen(string) + 1);

  demogobbler_bitstream_read_cstring(&stream, buffer, 80);

  EXPECT_EQ(strcmp(string, buffer), 0);
}
