#include "gtest/gtest.h"
#include <filesystem>
extern "C" {
#include "demogobbler/filereader.h"
#include "streams.h"
}

struct FileReaderTest : ::testing::Test {
  static void SetUpTestSuite() { std::filesystem::create_directory("./tmp"); }

  static void TearDownTestSuite() { std::filesystem::remove_all("./tmp"); }
};

void write_data_fstream(void *ptr, std::size_t size, const char *filepath) {
  FILE *output = fopen(filepath, "wb");
  ASSERT_NE(output, nullptr);

  fwrite(ptr, 1, 1024, output);
  fclose(output);
}

void write_data_ints(int count, const char *filepath) {
  FILE *output = fopen(filepath, "wb");
  ASSERT_NE(output, nullptr);

  for (int i = 0; i < count; ++i) {
    fwrite(&i, sizeof(int), 1, output);
  }

  fclose(output);
}

void read_data_filereader(void *ptr, std::size_t size, const char *filepath) {
  FILE *input = fopen(filepath, "rb");
  dg_input_interface iface;
  iface.read = fstream_read;
  iface.seek = fstream_seek;
  dg_filereader reader;
  char buffer[256];
  dg_filereader_init(&reader, buffer, sizeof(buffer), input, iface);
  dg_filereader_readdata(&reader, ptr, size);
  fclose(input);
}

TEST_F(FileReaderTest, readdata_works) {
  char BUFFER[1024];
  strcpy(BUFFER, "This is a test");
  write_data_fstream(BUFFER, 1024, "./tmp/dg_test.bin");

  char INPUT_BUFFER[1024];
  read_data_filereader(INPUT_BUFFER, 1024, "./tmp/dg_test.bin");
  EXPECT_EQ(strcmp(INPUT_BUFFER, BUFFER), 0);
}

TEST_F(FileReaderTest, readint32_works) {
  auto filepath = "./tmp/dg_test.bin";
  const int count = 1000;
  write_data_ints(count, filepath);

  FILE *input = fopen(filepath, "rb");
  dg_filereader reader;
  dg_input_interface iface;
  iface.read = fstream_read;
  iface.seek = fstream_seek;
  char buffer[256];
  dg_filereader_init(&reader, buffer, sizeof(buffer), input, iface);

  for (int i = 0; i < count; ++i) {
    int value = dg_filereader_readint32(&reader);
    EXPECT_EQ(value, i);
  }

  fclose(input);
}

TEST_F(FileReaderTest, skipbytes_works) {
  auto filepath = "./tmp/dg_test.bin";
  const int count = 1000;
  write_data_ints(count, filepath);

  FILE *input = fopen(filepath, "rb");
  dg_filereader reader;
  dg_input_interface iface;
  iface.read = fstream_read;
  iface.seek = fstream_seek;
  char buffer[256];
  dg_filereader_init(&reader, buffer, sizeof(buffer), input, iface);
  dg_filereader_skipbytes(&reader, sizeof(int) * 256);

  for (int i = 256; i < count; ++i) {
    int value = dg_filereader_readint32(&reader);
    EXPECT_EQ(value, i);
  }

  fclose(input);
}

TEST_F(FileReaderTest, skipto_works) {
  auto filepath = "./tmp/dg_test.bin";
  const int count = 10000;
  write_data_ints(count, filepath);

  FILE *input = fopen(filepath, "rb");
  dg_filereader reader;
  dg_input_interface iface;
  iface.read = fstream_read;
  iface.seek = fstream_seek;
  char buffer[256];
  dg_filereader_init(&reader, buffer, sizeof(buffer), input, iface);

  for (int i = 0; i < 256; ++i) {
    int value = dg_filereader_readint32(&reader);
    EXPECT_EQ(value, i);
  }

  dg_filereader_skipto(&reader, sizeof(int) * 9999);
  int value = dg_filereader_readint32(&reader);
  EXPECT_EQ(value, 9999);
  fclose(input);
}