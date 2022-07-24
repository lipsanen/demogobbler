#include "demogobbler_bitstream.h"
#include "demogobbler_bitwriter.h"
#include "gtest/gtest.h"
#include <cmath>
#include <cstdint>

template <typename T> bool test_num(T num, T &got, unsigned int bits, unsigned int bitOffset) {
  T orig = num;
  num <<= bitOffset;
  bitstream stream = demogobbler_bitstream_create(&num, sizeof(T) * 8);
  if (bitOffset > 0) {
    T offset_number = demogobbler_bitstream_read_uint(&stream, bitOffset);

    if (offset_number != 0) {
      got = -1;
      return false;
    }
  }

  got = demogobbler_bitstream_read_uint(&stream, bits);

  if (got != orig) {
    return false;
  }

  return true;
}

template <typename T> bool test_signum(T num, T &got, unsigned int bits, unsigned int bitOffset) {
  T orig = num;
  num <<= bitOffset;
  bitstream stream = demogobbler_bitstream_create(&num, 16 * 8);
  if (bitOffset > 0) {
    T offset_number = demogobbler_bitstream_read_sint(&stream, bitOffset);

    if (offset_number != 0) {
      got = -1;
      return false;
    }
  }

  got = demogobbler_bitstream_read_sint(&stream, bits);

  if (got != orig) {
    return false;
  }

  return true;
}

uint64_t get_random_int(unsigned int bits) { return rand() % (1 << bits); }

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
       EXPECT_EQ(test_num(value, got, i + 1, offset), true) << " offset " << offset << " : " << got
   << " != " << value << " failed.";
     }
   } */
}

TEST(Bitstream, AlignedCString) {
  const char *string = "this is a wonderful string.";
  char buffer[80];
  bitstream stream =
      demogobbler_bitstream_create(const_cast<void *>((const void *)string), strlen(string) + 1);

  demogobbler_bitstream_read_cstring(&stream, buffer, 80);

  EXPECT_EQ(strcmp(string, buffer), 0);
}

TEST(BitstreamPlusWriter, Bit) {
  bitwriter writer;
  const int BITS = 8192;
  bitwriter_init(&writer, BITS);
  srand(0);

  for (int i = 0; i < BITS; ++i) {
    bool value = rand() % 2;
    bitwriter_write_bit(&writer, value);
  }

  bitstream stream = bitstream_create(writer.ptr, BITS);
  srand(0);

  for (int i = 0; i < BITS; ++i) {
    bool value = rand() % 2;
    bool read = bitstream_read_bit(&stream);
    EXPECT_EQ(value, read) << i;
  }
}

TEST(BitstreamPlusWriter, Bits) {
  bitwriter writer;
  const int BITS = 32768;
  bitwriter_init(&writer, BITS);
  srand(0);
  int bits = 9;

  for (int i = 0;;) {
    bits = rand() % 64 + 1;
    i += bits;

    if (i > BITS)
      break;

    uint64_t value = rand() % (1 << bits);
    bitwriter_write_bits(&writer, &value, bits);
  }

  bitstream stream = bitstream_create(writer.ptr, BITS);
  srand(0);

  for (int i = 0;;) {
    bits = rand() % 64 + 1;
    i += bits;

    if (i > BITS)
      break;
    uint64_t value = rand() % (1 << bits);
    uint64_t read = bitstream_read_uint(&stream, bits);
    EXPECT_EQ(value, read) << i;
  }
}

TEST(BitstreamPlusWriter, VarUint) {
  bitwriter writer;
  const int NUMBERS = 1000;
  const int BITS = NUMBERS * 40;
  bitwriter_init(&writer, BITS);
  srand(0);
  for (int i = 0; i < NUMBERS; ++i) {
    uint32_t value = rand() % (1 << 30);
    bitwriter_write_varuint32(&writer, value);
  }

  bitstream stream = bitstream_create(writer.ptr, BITS);
  srand(0);

  for (int i = 0; i < NUMBERS; ++i) {
    uint32_t value = rand() % (1 << 30);
    uint32_t read = bitstream_read_varuint32(&stream);
    EXPECT_EQ(read, value) << i;
  }
}

static void test_bitangle_vectors(bitangle_vector v1, bitangle_vector v2) {
  EXPECT_EQ(v1.x, v2.x);
  EXPECT_EQ(v1.y, v2.y);
  EXPECT_EQ(v1.z, v2.z);
  EXPECT_EQ(v1.bits, v2.bits);
}

TEST(BitstreamPlusWriter, BitVector) {
  bitwriter writer;
  const int BITS = 100000;
  bitwriter_init(&writer, BITS);
  unsigned int bits;

  srand(0);

  for (int i = 0;;) {
    bits = rand() % 32 + 1;
    i += bits * 3;

    if (i > BITS)
      break;

    bitangle_vector vec;
    vec.bits = bits;
    vec.x = get_random_int(bits);
    vec.y = get_random_int(bits);
    vec.z = get_random_int(bits);
    bitwriter_write_bitvector(&writer, vec);
  }

  bitstream stream = bitstream_create(writer.ptr, writer.bitsize);
  srand(0);

  for (int i = 0;;) {
    bits = rand() % 32 + 1;
    i += bits * 3;

    if (i > BITS)
      break;

    bitangle_vector vec;
    vec.bits = bits;
    vec.x = get_random_int(bits);
    vec.y = get_random_int(bits);
    vec.z = get_random_int(bits);
    bitangle_vector read = bitstream_read_bitvector(&stream, bits);
    test_bitangle_vectors(vec, read);
  }
}

static void test_coord(bitcoord coord1, bitcoord coord2) {
  EXPECT_EQ(coord1.exists, coord2.exists);
  EXPECT_EQ(coord1.has_frac, coord2.has_frac);
  EXPECT_EQ(coord1.frac_value, coord2.frac_value);
  EXPECT_EQ(coord1.has_int, coord2.has_int);
  EXPECT_EQ(coord1.int_value, coord2.int_value);
  EXPECT_EQ(coord1.sign, coord2.sign);
}

static void test_coord_vectors(bitcoord_vector v1, bitcoord_vector v2) {
  test_coord(v1.x, v2.x);
  test_coord(v1.y, v2.y);
  test_coord(v1.z, v2.z);
}

static bitcoord get_rng_coord() {
  bitcoord out;
  memset(&out, 0, sizeof(out));
  out.exists = rand() % 3;
  if (out.exists) {
    out.has_frac = rand() % 3;
    if (out.has_frac)
      out.frac_value = get_random_int(5);
    out.has_int = rand() % 3;
    if (out.has_int)
      out.int_value = get_random_int(14);
  }

  return out;
}

static bitcoord_vector get_rng_bitcoord_vector() {
  bitcoord_vector out;
  memset(&out, 0, sizeof(out));
  out.x = get_rng_coord();
  out.y = get_rng_coord();
  out.z = get_rng_coord();
  return out;
}

TEST(BitstreamPlusWriter, CoordVector) {
  bitwriter writer;
  const int BITS = 10000;
  const int MAX_BITS_PER_ITERATION =
      100;                    // idc to calculate the exact value but should be less than this
  bitwriter_init(&writer, 1); // Test the dynamic resizing thing just for fun here

  srand(0);

  for (int i = 0;;) {
    i += MAX_BITS_PER_ITERATION;

    if (i > BITS)
      break;

    bitcoord_vector vec = get_rng_bitcoord_vector();
    bitwriter_write_coordvector(&writer, vec);
  }

  bitstream stream = bitstream_create(writer.ptr, writer.bitsize);
  srand(0);

  for (int i = 0;;) {
    i += MAX_BITS_PER_ITERATION;

    if (i > BITS)
      break;

    bitcoord_vector vec = get_rng_bitcoord_vector();
    bitcoord_vector read = bitstream_read_coordvector(&stream);
    test_coord_vectors(vec, read);
  }
}

TEST(BitstreamPlusWriter, UInt) {
  bitwriter writer;
  uint64_t value = 0b10101;
  unsigned int bits = 5;
  bitwriter_init(&writer, 1);
  bitwriter_write_uint(&writer, value, bits);
  bitstream stream = bitstream_create(writer.ptr, writer.bitsize);

  EXPECT_EQ(value, bitstream_read_uint(&stream, bits));
}

TEST(BitstreamPlusWriter, Int) {
  bitwriter writer;
  int64_t value = -1;
  unsigned int bits = 5;
  bitwriter_init(&writer, 1);
  bitwriter_write_sint(&writer, value, bits);
  bitstream stream = bitstream_create(writer.ptr, writer.bitsize);

  EXPECT_EQ(value, bitstream_read_sint(&stream, bits));
}

TEST(BitstreamPlusWriter, Int32) {
  bitwriter writer;
  int64_t value = -1;
  bitwriter_init(&writer, 1);
  bitwriter_write_sint32(&writer, value);
  bitstream stream = bitstream_create(writer.ptr, writer.bitsize);

  EXPECT_EQ(value, bitstream_read_sint32(&stream));
}

TEST(BitstreamPlusWriter, CString) {
  const char *text = "The quick brown fox something something something.";
  bitwriter writer;
  bitwriter_init(&writer, 1);
  bitwriter_write_cstring(&writer, text);
  bitstream stream = bitstream_create(writer.ptr, writer.bitsize);

  char BUFFER[80];
  bitstream_read_cstring(&stream, BUFFER, 80);

  EXPECT_EQ(strcmp(text, BUFFER), 0);
}

TEST(BitstreamPlusWriter, UInt32) {
  bitwriter writer;
  uint64_t value = 0b101010;
  bitwriter_init(&writer, 1);
  bitwriter_write_uint32(&writer, value);
  bitstream stream = bitstream_create(writer.ptr, writer.bitsize);

  EXPECT_EQ(value, bitstream_read_uint32(&stream));
}
