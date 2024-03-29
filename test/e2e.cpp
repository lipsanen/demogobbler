#include "utils/copy.hpp"
#include "demogobbler.h"
#include "utils/test_demos.hpp"
#include "gtest/gtest.h"

TEST(E2E, copy_demos) {
  for (auto &demo : get_test_demos()) {
    std::cout << "[----------] " << demo << std::endl;
    copy_demo_test(demo.c_str());
  }
}

TEST(E2E, freddie) {
  for (auto &demo : get_test_demos()) {
    std::cout << "[----------] " << demo << std::endl;
    copy_demo_freddie(demo.c_str());
  }
}

static void packet_handler(parser_state *, dg_packet *) {}

TEST(E2E, buffer_stream_test) {

  auto demos = get_test_demos();

  if (demos.empty()) {
    GTEST_SKIP();
  }

  auto demo = demos[0];
  std::cout << "[----------] " << demo << std::endl;
  FILE *stream = fopen(demo.c_str(), "rb");
  fseek(stream, 0, SEEK_END);
  std::size_t length = ftell(stream);

  void *buffer = malloc(length);

  fseek(stream, 0, SEEK_SET);
  fread(buffer, 1, length, stream);
  fclose(stream);

  dg_settings settings;
  dg_settings_init(&settings);

  settings.packet_handler = packet_handler;

  auto out = dg_parse_buffer(&settings, buffer, length);

  EXPECT_EQ(out.error, false) << out.error_message;

  free(buffer);
}
