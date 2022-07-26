#include "demogobbler.h"
#include "demogobbler_bitstream.h"
#include "dynamic_array.h"
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
    }
  }

  void verify_bulk(uint64_t start_offset, uint64_t bitsize) {
    if (this->error)
      return;

    // Original data
    bitstream stream1;
    memset(&stream1, 0, sizeof(stream1));
    stream1.data = current_data;
    stream1.bitoffset = start_offset;
    stream1.bitsize = bitsize;

    // Written data
    bitstream stream2;
    memset(&stream2, 0, sizeof(stream2));
    stream2.data = writer.ptr;
    stream2.bitoffset = start_offset;
    stream2.bitsize = bitsize;

    while (demogobbler_bitstream_bits_left(&stream1) > 0) {
      uint32_t value1 = bitstream_read_bit(&stream1);
      uint32_t value2 = bitstream_read_bit(&stream2);

      if (value1 != value2) {
        int64_t offset = stream1.bitoffset - prev_offset;
        EXPECT_EQ(value1, value2) << " error writing message of type " << last_message << " at offset " << offset  << " / " << stream1.bitsize - prev_offset;
        this->error = true;
        break;
      }
    }
  }

  void verify_last_iteration() {
    verify_bulk(this->prev_offset, this->writer.bitoffset);
    this->prev_offset = this->writer.bitoffset;
  }
};

static void version_handler(parser_state* state, demo_version_data data) {
  packet_copy_tester *tester = (packet_copy_tester *)state->client_state;
  tester->version = data;
}

static void packet_handler(parser_state* state, demogobbler_packet *packet) {
  packet_copy_tester *tester = (packet_copy_tester *)state->client_state;

  if(packet->size_bytes > tester->current_data_size) {
    free(tester->current_data);
    tester->current_data = malloc(packet->size_bytes);
    tester->current_data_size = packet->size_bytes;
  }
  memcpy(tester->current_data, packet->data, packet->size_bytes);
  tester->prev_offset = 0;
  tester->writer.bitoffset = 0;
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
    tester->verify_last_iteration();
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

  auto out = demogobbler_parser_parse_file(&settings, filepath);
  EXPECT_EQ(out.error, false) << out.error_message;
  tester.verify_last_packet_size();

}

TEST(E2E, packet_copy) {
  for (auto &demo : get_test_demos()) {
    std::cout << "[----------] " << demo << std::endl;
    packet_write_test(demo.c_str());
  }
}
