#include "demogobbler.h"
#include "memory_stream.hpp"
#include "gtest/gtest.h"
#include <algorithm>
#include <cstring>

writer w;

#define DECLARE_WRITE_FUNC(type) static void type ##_handler(demogobbler_ ## type *message) {\
  demogobbler_write_ ## type (&w, message); \
}

DECLARE_WRITE_FUNC(consolecmd);
DECLARE_WRITE_FUNC(customdata);
DECLARE_WRITE_FUNC(datatables);
DECLARE_WRITE_FUNC(header);
DECLARE_WRITE_FUNC(packet);
DECLARE_WRITE_FUNC(stringtables);
DECLARE_WRITE_FUNC(stop);
DECLARE_WRITE_FUNC(synctick);
DECLARE_WRITE_FUNC(usercmd);

void handle_version(demo_version version)
{
  w.version = version;
}

void copy_demo_test(const char* filepath)
{
  memory_stream output;
  memory_stream input;
  output.ground_truth = &input;
  input.fill_with_file(filepath);

  demogobbler_writer_init(&w);
  demogobbler_writer_open(&w, &output, {memory_stream_write});
  if(!w.error_set)
  {
    demogobbler_parser parser;
    demogobbler_settings settings;
    demogobbler_settings_init(&settings);

    settings.consolecmd_handler = consolecmd_handler;
    settings.customdata_handler = customdata_handler;
    settings.datatables_handler = datatables_handler;
    settings.header_handler = header_handler;
    settings.packet_handler = packet_handler;
    settings.stop_handler = stop_handler;
    settings.stringtables_handler = stringtables_handler;
    settings.synctick_handler = synctick_handler;
    settings.usercmd_handler = usercmd_handler;
    settings.demo_version_handler = handle_version;

    input_interface input_funcs = {memory_stream_read, memory_stream_seek};

    demogobbler_parser_init(&parser, &settings);
    demogobbler_parser_parse(&parser, &input, input_funcs );
    EXPECT_EQ(parser.error, false);
    demogobbler_parser_free(&parser);

    demogobbler_writer_close(&w);
  }
  demogobbler_writer_free(&w);

  EXPECT_EQ(output.file_size, input.file_size) << "Output and Input file sizes did not match " << output.file_size << " vs. " << input.file_size;
  std::size_t size = std::min(output.file_size, input.file_size);
  uint8_t* ptr1 = (uint8_t*)output.buffer;
  uint8_t* ptr2 = (uint8_t*)input.buffer;

  for(std::size_t i=0; i < size; ++i)
  {
    if(ptr1[i] != ptr2[i])
    {
      EXPECT_EQ(ptr1[i], ptr2[i]) << " expected output/input bytes to match at index " << i;
      break;
    }
  }
}