#include "copy.hpp"
#include "demogobbler.h"
#include "demogobbler/freddie.hpp"
#include "utils/memory_stream.hpp"
#include "gtest/gtest.h"
#include <filesystem>
#include <regex>

namespace {
  struct Demo {
    std::string path;
    std::string game;
    int build_num;
  };

  struct Conversion {
    std::string example;
    std::string input;
  };

  std::vector<Conversion> get_test_conversions() {
    std::vector<Demo> demos;
    std::regex demo_regex("(\\w+)_(\\d+).*\\.dem");
    std::smatch m;

    for (auto &file : std::filesystem::directory_iterator("./test_convert/")) {
      std::string path = file.path();
      if (file.is_regular_file() && std::regex_search(path, m, demo_regex)) {
        Demo demo;
        demo.path = path;
        demo.game = m[1];
        demo.build_num = std::stoi(m[2]);
        demos.push_back(demo);
      }
    }

    std::vector<Conversion> conversions;

    for(auto& input : demos) {
      for(auto& example : demos) {
        if(input.game == example.game && input.build_num <= example.build_num) {
          Conversion conv;
          conv.example = example.path;
          conv.input = input.path;
          conversions.push_back(conv);
        }
      }
    }

    return conversions;
  }

} // namespace

static bool verify_updates(freddie::demo_t* demo) {
  dg_parse_result result;
  memset(&result, 0, sizeof(result));
  estate state;
  memset(&state, 0, sizeof(state));
  estate_init_args args;
  dg_alloc_state alligator = dg_arena_create_allocator(&demo->arena);

  args.allocator = &alligator;
  args.flatten_datatables = false;
  args.message = demo->get_datatables();
  args.should_store_props = false;
  args.version_data = &demo->demver_data;
  dg_estate_init(&state, args);

  for(size_t i=0; i < demo->packets.size() && !result.error; ++i) {
    packet_parsed *ptr = std::get_if<packet_parsed>(&demo->packets[i]->packet);

    if(ptr == NULL)
      continue;

    for(size_t msg_index=0; msg_index <  ptr->message_count; ++msg_index) {
      packet_net_message* msg = ptr->messages + msg_index;
      if(msg->mtype != svc_packet_entities)
        continue;
      dg_svc_packet_entities* packet_entities = &msg->message_svc_packet_entities;
      write_packetentities_args args;
      args.data = &packet_entities->parsed->data;
      args.version = &demo->demver_data;
      args.is_delta = packet_entities->is_delta;
      bitwriter writer;
      bitwriter_init(&writer, 1024);
      dg_bitwriter_write_packetentities(&writer, args);

      dg_packetentities_data output;
      dg_packetentities_parse_args args_parse;
      args_parse.allocator = &alligator;
      args_parse.demver_data = &demo->demver_data;
      args_parse.entity_state = &state;
      args_parse.message = packet_entities;
      args_parse.output = &output;
      args_parse.permanent_allocator = &alligator;
      result = dg_parse_packetentities(&args_parse);

      if(!result.error && (output.ent_updates_count != packet_entities->parsed->data.ent_updates_count
      || output.explicit_deletes_count != packet_entities->parsed->data.explicit_deletes_count)) {
        result.error = true;
        result.error_message = "update counts did not match";
      }

      if(!result.error) {
        result = dg_estate_update(&state, &output);
      }

      if(result.error) {
        EXPECT_EQ(result.error, false) << result.error_message;
      }

      bitwriter_free(&writer);
    }
  }

  dg_estate_free(&state);

  return !result.error;
}

TEST(convert, test) {
  auto conversions = get_test_conversions();
  for(auto& conversion : conversions) {
    std::cout << "[----------] " << conversion.input << " => " << conversion.example << std::endl;

    freddie::demo_t example;
    freddie::demo_t input;
    auto result = freddie::demo_t::parse_demo(&example, conversion.example.c_str());

    if(result.error)
    {
      EXPECT_EQ(result.error, false) << "error parsing example demo: " << result.error_message;
      continue;
    }

    result = freddie::demo_t::parse_demo(&input, conversion.input.c_str());

    if(result.error)
    {
      EXPECT_EQ(result.error, false) << "error parsing example demo: " << result.error_message;
      continue;
    }

    result = freddie::convert_demo(&example, &input);

    if(result.error)
    {
      EXPECT_EQ(result.error, false) << "error parsing input demo: " << result.error_message;
      continue;
    }

    if(!verify_updates(&input)) {
      continue;
    }

    wrapped_memory_stream output_stream;
    result = input.write_demo(&output_stream.underlying, {freddie::memory_stream_write});

    if(result.error)
    {
      EXPECT_EQ(result.error, false) << "error writing demo: " << result.error_message;
      continue;
    }

    output_stream.underlying.offset = 0;
    freddie::demo_t output;
    result = freddie::demo_t::parse_demo(&output, &output_stream.underlying, {freddie::memory_stream_read, freddie::memory_stream_seek});

    if(result.error)
    {
      EXPECT_EQ(result.error, false) << "error parsing result demo: " << result.error_message;
      continue;
    }

    EXPECT_EQ(output.packets.size(), input.packets.size());
  }
}
