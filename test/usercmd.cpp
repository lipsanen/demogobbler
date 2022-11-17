extern "C" {
    #include "demogobbler/bitwriter.h"
    #include "parser_usercmd.h"
}

#include "gtest/gtest.h"

#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

uint8_t msg1[] = { 0xd3, 0x8, 0x0, 0x0, 0xfa, 0x5, 0x0, 0x0, 0x64, 0x71, 0x24, 0x15, 0x7e, 0x6c, 0xf4, 0xf0, 0x43, 0x0, 0x80, 0xf0, 0x61, 0x9, 0x4, 0x0, 0x0, 0x0 };
uint8_t msg2[] = { 0xb5, 0x8, 0x0, 0x0, 0xbe, 0x5, 0x0, 0x0, 0x34, 0x3e, 0x2d, 0x15, 0x3e, 0x89, 0x4e, 0xf1, 0x3, 0x5, 0x0, 0x0, 0x0, 0x0 };
uint8_t msg3[] = { 0x87, 0x3c, 0x0, 0x0, 0x86, 0x79, 0x0, 0x0, 0xf4, 0xd4, 0x58, 0x9, 0x5a, 0x89, 0x39, 0x30, 0x4, 0x20 };

uint8_t *msgs[] = {
    msg1,
    msg2,
    msg3
};

size_t sizes[] = {
    sizeof(msg1),
    sizeof(msg2),
    sizeof(msg3)
};

TEST(usercmd, works) 
{ 
    dg_demver_data data;
    dg_usercmd input;
    dg_usercmd_parsed output;

    memset(&data, 0, sizeof(data));
    memset(&input, 0, sizeof(input));

    dg_bitwriter writer;
    dg_bitwriter_init(&writer, 1024);

    for(size_t i=0; i < COUNT_OF(msgs); ++i) {
        writer.bitoffset = 0;
        input.data = msgs[i];
        input.size_bytes = sizes[i];

        auto result = dg_parser_parse_usercmd(&data, &input, &output);
        EXPECT_EQ(result.error, false);
        EXPECT_LT(dg_bitstream_bits_left(&output.left_overbits), 8);
    
#ifdef GROUND_TRUTH_CHECK
        writer.truth_data = input.data;
        writer.truth_data_offset = 0;
        writer.truth_size_bits = input.size_bytes * 8;
#endif
        dg_bitwriter_write_usercmd(&writer, &output);

        int mem_result = memcmp(input.data, writer.ptr, input.size_bytes);
        EXPECT_EQ(mem_result, 0);
    }
    dg_bitwriter_free(&writer);
}
