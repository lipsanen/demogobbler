#include "demogobbler/bitstream.h"
#include "demogobbler/bitwriter.h"
#include "gtest/gtest.h"
#include <cmath>
#include <cstdint>

extern "C" {
#include "utils.h"
}

template <typename T> bool test_num(T num, T &got, unsigned int bits, unsigned int bitOffset) {
  T orig = num;
  num <<= bitOffset;
  dg_bitstream stream = dg_bitstream_create(&num, sizeof(T) * 8);
  if (bitOffset > 0) {
    T offset_number = dg_bitstream_read_uint(&stream, bitOffset);

    if (offset_number != 0) {
      got = -1;
      return false;
    }
  }

  got = dg_bitstream_read_uint(&stream, bits);

  if (got != orig) {
    return false;
  }

  return true;
}

template <typename T> bool test_signum(T num, T &got, unsigned int bits, unsigned int bitOffset) {
  T orig = num;
  num <<= bitOffset;
  dg_bitstream stream = dg_bitstream_create(&num, 16 * 8);
  if (bitOffset > 0) {
    T offset_number = dg_bitstream_read_sint(&stream, bitOffset);

    if (offset_number != 0) {
      got = -1;
      return false;
    }
  }

  got = dg_bitstream_read_sint(&stream, bits);

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
  dg_bitstream stream =
      dg_bitstream_create(const_cast<void *>((const void *)string), (strlen(string) + 1) * 8);

  dg_bitstream_read_cstring(&stream, buffer, 80);

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

  dg_bitstream stream = bitstream_create(writer.ptr, BITS);
  srand(0);

  for (int i = 0; i < BITS; ++i) {
    bool value = rand() % 2;
    bool read = bitstream_read_bit(&stream);
    EXPECT_EQ(value, read) << i;
  }
  bitwriter_free(&writer);
}

TEST(BitstreamPlusWriter, Bits) {
  bitwriter writer;
  const int BITS = 3276800;
  bitwriter_init(&writer, BITS);
  srand(0);
  int bits = 9;

  for (int i = 0;;) {
    bits = rand() % 8 + 1;
    i += bits;

    if (i > BITS)
      break;

    uint64_t value = rand() % (1 << bits);
    bitwriter_write_bits(&writer, &value, bits);
  }

  dg_bitstream stream = bitstream_create(writer.ptr, BITS);
  srand(0);

  for (int i = 0;;) {
    bits = rand() % 8 + 1;
    i += bits;

    if (i > BITS)
      break;
    uint64_t value = rand() % (1 << bits);
    uint64_t read = bitstream_read_uint(&stream, bits);
    EXPECT_EQ(value, read) << i;
  }

  bitwriter_free(&writer);
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

  dg_bitstream stream = bitstream_create(writer.ptr, BITS);
  srand(0);

  for (int i = 0; i < NUMBERS; ++i) {
    uint32_t value = rand() % (1 << 30);
    uint32_t read = bitstream_read_varuint32(&stream);
    EXPECT_EQ(read, value) << i;
  }
  bitwriter_free(&writer);
}

static void test_bitangle_vectors(dg_bitangle_vector v1, dg_bitangle_vector v2) {
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
    bits = rand() % 31 + 1;
    i += bits * 3;

    if (i > BITS)
      break;

    dg_bitangle_vector vec;
    vec.bits = bits;
    vec.x = get_random_int(bits);
    vec.y = get_random_int(bits);
    vec.z = get_random_int(bits);
    bitwriter_write_bitvector(&writer, vec);
  }

  dg_bitstream stream = bitstream_create(writer.ptr, writer.bitsize);
  srand(0);

  for (int i = 0;;) {
    bits = rand() % 31 + 1;
    i += bits * 3;

    if (i > BITS)
      break;

    dg_bitangle_vector vec;
    vec.bits = bits;
    vec.x = get_random_int(bits);
    vec.y = get_random_int(bits);
    vec.z = get_random_int(bits);
    dg_bitangle_vector read = bitstream_read_bitvector(&stream, bits);
    test_bitangle_vectors(vec, read);
  }

  bitwriter_free(&writer);
}

static void test_coord(dg_bitcoord coord1, dg_bitcoord coord2) {
  EXPECT_EQ(coord1.exists, coord2.exists);
  EXPECT_EQ(coord1.has_frac, coord2.has_frac);
  EXPECT_EQ(coord1.frac_value, coord2.frac_value);
  EXPECT_EQ(coord1.has_int, coord2.has_int);
  EXPECT_EQ(coord1.int_value, coord2.int_value);
  EXPECT_EQ(coord1.sign, coord2.sign);
}

static void test_coord_vectors(dg_bitcoord_vector v1, dg_bitcoord_vector v2) {
  test_coord(v1.x, v2.x);
  test_coord(v1.y, v2.y);
  test_coord(v1.z, v2.z);
}

static dg_bitcoord get_rng_coord() {
  dg_bitcoord out;
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

static dg_bitcoord_vector get_rng_bitcoord_vector() {
  dg_bitcoord_vector out;
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

    dg_bitcoord_vector vec = get_rng_bitcoord_vector();
    bitwriter_write_coordvector(&writer, vec);
  }

  dg_bitstream stream = bitstream_create(writer.ptr, writer.bitsize);
  srand(0);

  for (int i = 0;;) {
    i += MAX_BITS_PER_ITERATION;

    if (i > BITS)
      break;

    dg_bitcoord_vector vec = get_rng_bitcoord_vector();
    dg_bitcoord_vector read = bitstream_read_coordvector(&stream);
    test_coord_vectors(vec, read);
  }
  bitwriter_free(&writer);
}

TEST(BitstreamPlusWriter, UInt) {
  bitwriter writer;
  uint64_t value = 0b10101;
  unsigned int bits = 5;
  bitwriter_init(&writer, 1);
  bitwriter_write_uint(&writer, value, bits);
  dg_bitstream stream = bitstream_create(writer.ptr, writer.bitsize);

  EXPECT_EQ(value, bitstream_read_uint(&stream, bits));
  bitwriter_free(&writer);
}

TEST(BitstreamPlusWriter, Int) {
  bitwriter writer;
  int64_t value = -1;
  unsigned int bits = 5;
  bitwriter_init(&writer, 1);
  bitwriter_write_sint(&writer, value, bits);
  dg_bitstream stream = bitstream_create(writer.ptr, writer.bitsize);

  EXPECT_EQ(value, bitstream_read_sint(&stream, bits));
  bitwriter_free(&writer);
}

TEST(BitstreamPlusWriter, Int32) {
  bitwriter writer;
  int64_t value = -1;
  bitwriter_init(&writer, 1);
  bitwriter_write_sint32(&writer, value);
  dg_bitstream stream = bitstream_create(writer.ptr, writer.bitsize);

  EXPECT_EQ(value, bitstream_read_sint32(&stream));
  bitwriter_free(&writer);
}

TEST(BitstreamPlusWriter, CString) {
  const char *text = "The quick brown fox something something something.";
  bitwriter writer;
  bitwriter_init(&writer, 1);
  bitwriter_write_cstring(&writer, text);
  dg_bitstream stream = bitstream_create(writer.ptr, writer.bitsize);

  char BUFFER[80];
  bitstream_read_cstring(&stream, BUFFER, 80);

  EXPECT_EQ(strcmp(text, BUFFER), 0);
  bitwriter_free(&writer);
}

TEST(BitstreamPlusWriter, UInt32) {
  bitwriter writer;
  uint64_t value = 0b101010;
  bitwriter_init(&writer, 1);
  bitwriter_write_uint32(&writer, value);
  dg_bitstream stream = bitstream_create(writer.ptr, writer.bitsize);

  EXPECT_EQ(value, bitstream_read_uint32(&stream));
  bitwriter_free(&writer);
}

TEST(BitstreamPlusWriter, Bitstream) {
  const int SIZE = 1024;
  uint8_t *data = (uint8_t *)malloc(SIZE);

  for (int i = 0; i < SIZE; ++i) {
    data[i] = i % 256;
  }

  dg_bitstream stream = bitstream_create(data, SIZE * 8 - 31);
  bitwriter writer;
  bitwriter_init(&writer, 1);
  bitwriter_write_bitstream(&writer, &stream);
  dg_bitstream stream2;
  stream2.bitoffset = 0;
  stream2.bitsize = writer.bitoffset;
  stream2.data = writer.ptr;
  stream2.overflow = false;

  stream.bitoffset = 0;
  stream.overflow = false;

  for (int i = 0; dg_bitstream_bits_left(&stream) > 0; ++i) {
    bool set1 = bitstream_read_bit(&stream);
    bool set2 = bitstream_read_bit(&stream2);
    ASSERT_EQ(set1, set2) << "wrong bit at index " << i << " / " << stream.bitsize;
  }
  bitwriter_free(&writer);
  free(data);
}

TEST(BitstreamPlusWriter, IndexDiff) {
  for (uint32_t old_index = -1; old_index < 0xFFE; ++old_index) {
    for (uint32_t new_index = old_index + 1; new_index < 0xFFF; ++new_index) {
      bitwriter writer;
      bitwriter_init(&writer, 1);
      dg_bitwriter_write_field_index(&writer, new_index, old_index, true);

      dg_bitstream stream = bitstream_create(writer.ptr, writer.bitsize);
      EXPECT_EQ(new_index, dg_bitstream_read_field_index(&stream, old_index, true));
      EXPECT_EQ(stream.bitoffset, writer.bitoffset);
      bitwriter_free(&writer);
    }
  }

  for (uint32_t old_index = -1; old_index < 0xFFE; ++old_index) {
    for (uint32_t new_index = old_index + 1; new_index < 0xFFF; ++new_index) {
      bitwriter writer;
      bitwriter_init(&writer, 1);
      dg_bitwriter_write_field_index(&writer, new_index, old_index, false);

      dg_bitstream stream = bitstream_create(writer.ptr, writer.bitsize);
      EXPECT_EQ(new_index, dg_bitstream_read_field_index(&stream, old_index, false));
      EXPECT_EQ(stream.bitoffset, writer.bitoffset);
      bitwriter_free(&writer);
    }
  }

  bitwriter writer;
  bitwriter_init(&writer, 1);
  dg_bitwriter_write_field_index(&writer, -1, 1, false);
  dg_bitwriter_write_field_index(&writer, -1, 1, true);
  dg_bitstream stream = bitstream_create(writer.ptr, writer.bitsize);
  EXPECT_EQ(-1, dg_bitstream_read_field_index(&stream, 1, false));
  EXPECT_EQ(-1, dg_bitstream_read_field_index(&stream, 1, true));
  EXPECT_EQ(stream.bitoffset, writer.bitoffset);
  bitwriter_free(&writer);
}

TEST(BitstreamPlusWriter, BitNormal) {
  const size_t max_frac = (1 << 11) - 1;

  for (size_t sign = 0; sign <= 1; ++sign) {
    for (size_t frac = 0; frac <= max_frac; ++frac) {
      dg_bitnormal normal;
      memset(&normal, 0, sizeof(normal));
      normal.sign = sign;
      normal.frac = frac;
      bitwriter writer;
      bitwriter_init(&writer, 1);
      dg_bitwriter_write_bitnormal(&writer, normal);

      dg_bitstream stream = bitstream_create(writer.ptr, writer.bitsize);
      dg_bitnormal output = dg_bitstream_read_bitnormal(&stream);
      ASSERT_EQ(normal.frac, output.frac);
      ASSERT_EQ(normal.sign, output.sign);
      ASSERT_EQ(stream.bitoffset, writer.bitoffset);
      bitwriter_free(&writer);
    }
  }
}

TEST(BitstreamPlusWriter, UBitInt) {
  const size_t max = 10000;
  srand(0);
  bitwriter writer;
  bitwriter_init(&writer, 1);
  for (size_t i = 0; i < max; ++i) {
    dg_bitwriter_write_ubitint(&writer, rand());
  }

  dg_bitstream stream = bitstream_create(writer.ptr, writer.bitsize);
  srand(0);

  for (size_t i = 0; i < max; ++i) {
    EXPECT_EQ(rand(), dg_bitstream_read_ubitint(&stream));
  }

  bitwriter_free(&writer);
}

TEST(BitstreamPlusWriter, UBitVar) {
  const size_t max = 10000;
  srand(0);
  bitwriter writer;
  bitwriter_init(&writer, 1);
  for (size_t i = 0; i < max; ++i) {
    dg_bitwriter_write_ubitvar(&writer, rand());
  }

  dg_bitstream stream = bitstream_create(writer.ptr, writer.bitsize);
  srand(0);

  for (size_t i = 0; i < max; ++i) {
    EXPECT_EQ(rand(), dg_bitstream_read_ubitvar(&stream));
  }

  bitwriter_free(&writer);
}

struct rng_bitcellcoord {
  bool is_int;
  bool lp;
  unsigned bits;
  dg_bitcellcoord value;
};

rng_bitcellcoord get_random_bitcellcoord() {
  rng_bitcellcoord output;
  memset(&output, 0, sizeof(output));
  output.is_int = rand() % 2;
  output.lp = rand() % 2;
  output.bits = rand() % 32 + 1;
  output.value.int_val = rand() % (1 << output.bits);

  if (!output.is_int) {
    if (output.lp) {
      output.value.fract_val = rand() % (1 << FRAC_BITS_LP);
    } else {
      output.value.fract_val = rand() % (1 << FRAC_BITS);
    }
  }

  return output;
}

struct rng_bitcoordmp {
  bool is_int;
  bool lp;
  dg_bitcoordmp value;
};

rng_bitcoordmp get_random_bitcoordmp() {
  rng_bitcoordmp output;
  memset(&output, 0, sizeof(output));
  output.is_int = rand() % 2;
  output.lp = rand() % 2;
  output.value.inbounds = rand() % 2;

  if (output.is_int) {
    output.value.int_has_val = rand() % 2;

    if (output.value.int_has_val) {
      output.value.sign = rand() % 2;
      if (output.value.inbounds) {
        output.value.int_val = rand() % (1 << COORD_INT_BITS_MP);
      } else {
        output.value.int_val = rand() % (1 << 14);
      }
    }
  } else {
    output.value.int_has_val = rand() % 2;
    output.value.sign = rand() % 2;

    if (output.value.int_has_val) {
      if (output.value.inbounds) {
        output.value.int_val = rand() % (1 << COORD_INT_BITS_MP);
      } else {
        output.value.int_val = rand() % (1 << 14);
      }
    }

    if (output.lp) {
      output.value.frac_val = rand() % (1 << FRAC_BITS_LP);
    } else {
      output.value.frac_val = rand() % (1 << FRAC_BITS);
    }
  }

  return output;
}

TEST(BitstreamPlusWriter, BitCellCoord) {
  const size_t max = 10000;
  srand(0);
  bitwriter writer;
  bitwriter_init(&writer, 1);
  for (size_t i = 0; i < max; ++i) {
    rng_bitcellcoord value = get_random_bitcellcoord();
    dg_bitwriter_write_bitcellcoord(&writer, value.value, value.is_int, value.lp, value.bits);
  }

  dg_bitstream stream = bitstream_create(writer.ptr, writer.bitsize);
  srand(0);

  for (size_t i = 0; i < max; ++i) {
    rng_bitcellcoord value = get_random_bitcellcoord();
    dg_bitcellcoord value2 =
        dg_bitstream_read_bitcellcoord(&stream, value.is_int, value.lp, value.bits);
    EXPECT_EQ(memcmp(&value.value, &value2, sizeof(dg_bitcellcoord)), 0);
  }

  EXPECT_EQ(stream.bitoffset, writer.bitoffset);
  bitwriter_free(&writer);
}

bool dg_bitcoordmp_eq(dg_bitcoordmp lhs, dg_bitcoordmp rhs) {
  return lhs.frac_val == rhs.frac_val && lhs.inbounds == rhs.inbounds &&
         lhs.int_has_val == rhs.int_has_val && lhs.int_val == rhs.int_val && lhs.sign == rhs.sign;
}

TEST(BitstreamPlusWriter, BitCoordMp) {
  const size_t max = 1;
  srand(0);
  bitwriter writer;
  bitwriter_init(&writer, 1);
  for (size_t i = 0; i < max; ++i) {
    rng_bitcoordmp value = get_random_bitcoordmp();
    dg_bitwriter_write_bitcoordmp(&writer, value.value, value.is_int, value.lp);
  }

  dg_bitstream stream = bitstream_create(writer.ptr, writer.bitsize);
  srand(0);

  for (size_t i = 0; i < max; ++i) {
    rng_bitcoordmp value = get_random_bitcoordmp();
    dg_bitcoordmp value1 = value.value;
    dg_bitcoordmp value2 = dg_bitstream_read_bitcoordmp(&stream, value.is_int, value.lp);

    bool eq = dg_bitcoordmp_eq(value1, value2);
    EXPECT_EQ(eq, true) << "Error on iteration " << i;
    if (!eq)
      break;
  }

  EXPECT_EQ(stream.bitoffset, writer.bitoffset);
  bitwriter_free(&writer);
}
