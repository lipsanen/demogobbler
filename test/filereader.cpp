#include "gtest/gtest.h"
#include <filesystem>
extern "C"
{
#include "filereader.h"
}

struct FileReaderTest: ::testing::Test {
    static void SetUpTestSuite() {
        std::filesystem::create_directory("./tmp");
    }

    static void TearDownTestSuite() {
        std::filesystem::remove_all("./tmp");
    }
};

void write_data_fstream(void* ptr, std::size_t size, const char* filepath)
{
    FILE* output = fopen(filepath, "wb");
    ASSERT_NE(output, nullptr);
   
    fwrite(ptr, 1, 1024, output);
    fclose(output);
}

void read_data_filereader(void* ptr, std::size_t size, const char* filepath)
{
    FILE* input = fopen(filepath, "rb");
    filereader reader;
    filereader_init(&reader, input);
    filereader_readdata(&reader, ptr, size);
    filereader_free(&reader);
}

TEST_F(FileReaderTest, readdata_works)
{
    char BUFFER[1024];
    strcpy(BUFFER, "This is a test.");
    write_data_fstream(BUFFER, 1024, "./tmp/demogobbler_test.bin");

    char INPUT_BUFFER[1024];
    read_data_filereader(INPUT_BUFFER, 1024, "./tmp/demogobbler_test.bin");
    EXPECT_EQ(strcmp(INPUT_BUFFER, BUFFER), 0);
}