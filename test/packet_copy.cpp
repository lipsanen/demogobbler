#include "demogobbler.h"
#include "demogobbler/bitstream.h"
#include "demogobbler/bitwriter.h"
#include "utils/test_demos.hpp"
#include "demogobbler/utils.h"
#include "gtest/gtest.h"

struct packet_copy_tester {
  dg_demver_data version;
  dg_bitwriter writer;
  int64_t prev_offset;
  net_message_type last_message;
  void *current_data;
  size_t current_data_size;
  int64_t packet_bits;
  bool error;

  ~packet_copy_tester() {
    dg_bitwriter_free(&writer);
    free(current_data);
  }

  packet_copy_tester() {
    memset(this, 0, sizeof(*this));
    this->last_message = svc_invalid;
    dg_bitwriter_init(&writer, 32768);
    current_data_size = 32768;
    current_data = malloc(current_data_size);
  }
};

static void version_handler(parser_state *state, dg_demver_data data) {
  packet_copy_tester *tester = (packet_copy_tester *)state->client_state;
  tester->version = data;
}

static void packet_handler(parser_state *state, packet_parsed *packet_parsed) {
  packet_copy_tester *tester = (packet_copy_tester *)state->client_state;
  dg_packet *packet = &packet_parsed->orig;

  if (packet->size_bytes > (int32_t)tester->current_data_size) {
    free(tester->current_data);
    tester->current_data = malloc(packet->size_bytes);
    tester->current_data_size = packet->size_bytes;
  }
  memcpy(tester->current_data, packet->data, packet->size_bytes);
  tester->prev_offset = 0;
  tester->writer.bitoffset = 0;
#if GROUND_TRUTH_CHECK
  tester->writer.truth_data = packet->data;
  tester->writer.truth_size_bits = packet->size_bytes * 8;
#endif
  tester->packet_bits = packet->size_bytes * 8;

  for (size_t i = 0; i < packet_parsed->message_count; ++i) {
    packet_net_message *message = &packet_parsed->messages[i];
    dg_bitwriter_write_netmessage(&tester->writer, &tester->version, message);

#ifdef DEBUG
    EXPECT_EQ(tester->writer.bitoffset, message->offset)
        << "Error writing message of type " << message->mtype;
#endif

    if (tester->writer.error) {
      EXPECT_EQ(tester->writer.error, false) << tester->writer.error_message;
      tester->error = true;
      state->error = true;
      state->error_message = "Copy failed";
    }
  }

  dg_bitwriter_write_bitstream(&tester->writer, &packet_parsed->leftover_bits);

  if (!tester->error) {
    for (int32_t i = 0; i < packet->size_bytes; ++i) {
      uint8_t dest = *((uint8_t *)tester->writer.ptr + i);
      uint8_t src = *((uint8_t *)packet->data + i);
      EXPECT_EQ(dest, src) << "manual mem check failed, bug with bitstreams " << dest
                           << " != " << src;
    }
  }
}

static void packet_write_test(const char *filepath) {
  packet_copy_tester tester;
  dg_settings settings;
  dg_settings_init(&settings);

  settings.packet_parsed_handler = packet_handler;
  settings.demo_version_handler = version_handler;
  settings.client_state = &tester;

  auto out = dg_parse_file(&settings, filepath);
  EXPECT_EQ(out.error, false) << out.error_message;
}

TEST(E2E, packet_copy) {
  for (auto &demo : get_test_demos()) {
    std::cout << "[----------] " << demo << std::endl;
    packet_write_test(demo.c_str());
  }
}
