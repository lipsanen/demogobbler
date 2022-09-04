#include "demogobbler.h"
#include "demogobbler_bitstream.h"
#include "demogobbler_bitwriter.h"
#include "test_demos.hpp"
#include "utils.h"
#include "gtest/gtest.h"

struct packet_copy_tester {
  demo_version_data version;
  bitwriter writer;
  int64_t prev_offset;
  net_message_type last_message;
  void *current_data;
  size_t current_data_size;
  int64_t packet_bits;
  bool error;

  ~packet_copy_tester() { 
    bitwriter_free(&writer);
    free(current_data); 
  }

  packet_copy_tester() {
    memset(this, 0, sizeof(*this));
    this->last_message = svc_invalid;
    bitwriter_init(&writer, 32768);
    current_data_size = 32768;
    current_data = malloc(current_data_size);
  }

  void verify_last_packet_size() {
    if(current_data && !this->error) {
      int64_t bit_diff = this->packet_bits - this->writer.bitoffset;

      // There might be a few padding bits missing from the end, but otherwise report size error
      if (bit_diff < 0 || bit_diff > 6) {
        EXPECT_EQ(1, 0) << "the packet had the wrong size " << this->packet_bits << " vs. "
                        << this->writer.bitoffset;
        error = true;
      }
      else {
        for(int i=0; i < this->packet_bits / 8 - 1; ++i) {
          uint8_t dest = *((uint8_t*)this->writer.ptr + i);
          uint8_t src = *((uint8_t*)this->current_data + i);
          EXPECT_EQ(dest, src) << "manual mem check failed, bug with bitstreams " << dest << " != " << src;
        }
      }
    }
  }
};

static void version_handler(parser_state* state, demo_version_data data) {
  packet_copy_tester *tester = (packet_copy_tester *)state->client_state;
  tester->version = data;
}

static void packet_handler(parser_state* state, demogobbler_packet *packet) {
  packet_copy_tester *tester = (packet_copy_tester *)state->client_state;

  if(packet->size_bytes > (int32_t)tester->current_data_size) {
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
}

static void netmessage_handler(parser_state* state, packet_net_message *message) {
  //printf("handler %d\n", message->mtype);
  packet_copy_tester *tester = (packet_copy_tester *)state->client_state;
  if(!tester->error) {
    demogobbler_bitwriter_write_netmessage(&tester->writer, &tester->version, message);

#ifdef DEBUG
    EXPECT_EQ(tester->writer.bitoffset, message->offset) << "Error writing message of type " << message->mtype;
#endif
    
    if(tester->writer.error) {
      EXPECT_EQ(tester->writer.error, false) << tester->writer.error_message;
      tester->error = true;
    }

    tester->last_message = message->mtype;

    if(message->last_message) {
      tester->verify_last_packet_size();
    }
  }
}

static void packet_write_test(const char *filepath) {
  packet_copy_tester tester;
  demogobbler_settings settings;
  demogobbler_settings_init(&settings);

  settings.packet_handler = packet_handler;
  settings.packet_net_message_handler = netmessage_handler;
  settings.demo_version_handler = version_handler;
  settings.client_state = &tester;

  auto out = demogobbler_parse_file(&settings, filepath);
  EXPECT_EQ(out.error, false) << out.error_message;

}

TEST(E2E, packet_copy) {
  for (auto &demo : get_test_demos()) {
    std::cout << "[----------] " << demo << std::endl;
    packet_write_test(demo.c_str());
  }
}
