#include "demogobbler/freddie.hpp"
#include <cstdio>
#include <cstring>
#include <map>
#include <vector>

#define DEMO_GHOST_INDEX 1024

using namespace freddie;

static int get_prop_index(demo_t *demo, int datatable_id, const char *prop_name) {
  estate state = {0};
  estate_init_args args;
  dg_alloc_state allocator = dg_arena_create_allocator(&demo->arena);
  args.allocator = &allocator;
  args.flatten_datatables = false;
  args.should_store_props = false;
  args.message = demo->get_datatables();
  auto result = dg_estate_init(&state, args);
  if (result.error) {
    printf("Failed to get prop_index: %s\n", result.error_message);
    return -1;
  }

  int prop_index = -1;
  dg_serverclass_data *class_data =
      dg_estate_serverclass_data(&state, &demo->demver_data, &allocator, datatable_id);
  for (size_t i = 0; i < class_data->prop_count; i++) {
    if (strcmp(prop_name, class_data->props[i].name) == 0) {
      prop_index = i;
      break;
    }
  }

  dg_estate_free(&state);
  return prop_index;
}

static void collect_player_updates(demo_t *demo, std::map<int32_t, dg_ent_update> &data) {
  for (auto &message : demo->packets) {
    auto *packet = &message->packet;
    packet_parsed *ptr = std::get_if<packet_parsed>(packet);

    if (!ptr)
      continue;

    int32_t tick = ptr->orig.preamble.tick;
    for (size_t msg_index = 0; msg_index < ptr->message_count; ++msg_index) {
      packet_net_message *msg = ptr->messages + msg_index;
      if (msg->mtype == svc_packet_entities) {
        dg_packetentities_data *packet_entities = &msg->message_svc_packet_entities.parsed->data;
        for (uint32_t j = 0; j < packet_entities->ent_updates_count; j++) {
          if (packet_entities->ent_updates[j].ent_index == 1) {
            data[tick] = packet_entities->ent_updates[j];
            break;
          }
        }
      }
    }
  }
}

static void add_player_updates(demo_t *demo, std::vector<std::map<int32_t, dg_ent_update>> &data) {
  int m_flSimulationTime_index = -1;
  // Changing m_flSimulationTime doesn't seem to fix Portal2 demo
  bool should_smooth_demo = (demo->demver_data.demo_protocol < 4);

  for (auto &message : demo->packets) {
    auto *packet = &message->packet;
    packet_parsed *ptr = std::get_if<packet_parsed>(packet);

    if (!ptr)
      continue;

    int32_t tick = ptr->orig.preamble.tick;
    for (size_t msg_index = 0; msg_index < ptr->message_count; ++msg_index) {
      packet_net_message *msg = ptr->messages + msg_index;
      if (msg->mtype == svc_packet_entities) {
        dg_packetentities_data *packet_entities = &msg->message_svc_packet_entities.parsed->data;

        if (should_smooth_demo && m_flSimulationTime_index == -1) {
          for (uint32_t i = 0; i < packet_entities->ent_updates_count; i++) {
            if (packet_entities->ent_updates[i].ent_index == 1) {
              m_flSimulationTime_index = get_prop_index(
                  demo, packet_entities->ent_updates[i].datatable_id, "m_flSimulationTime");
              break;
            }
          }
        }

        // Get all the updates
        std::vector<dg_ent_update> player_updates;
        uint32_t max_entries = msg->message_svc_packet_entities.max_entries;
        for (size_t i = 0; i < data.size(); i++) {
          if (data[i].find(tick) == data[i].end())
            continue;

          // Ghost index starts at DEMO_GHOST_INDEX
          dg_ent_update *update = &data[i][tick];
          update->ent_index = DEMO_GHOST_INDEX + i;
          max_entries = DEMO_GHOST_INDEX + i - 1;

          if (should_smooth_demo && m_flSimulationTime_index != -1) {
            // Hack to fix interpolation issue
            for (size_t j = 0; j < update->prop_value_array_size; j++) {
              if ((int)update->prop_value_array[j].prop_index == m_flSimulationTime_index &&
                  update->prop_value_array[j].value.signed_val < 33) {
                update->prop_value_array[j].value.signed_val += 100;
              }
            }
          }
          player_updates.push_back(data[i][tick]);
        }

        if (player_updates.empty())
          continue;

        // Edit the packet_entities->ent_pudates
        write_packetentities_args args = {0};
        args.is_delta = msg->message_svc_packet_entities.is_delta;
        args.version = &demo->demver_data;
        std::vector<dg_ent_update> new_updates(packet_entities->ent_updates,
                                               packet_entities->ent_updates +
                                                   packet_entities->ent_updates_count);
        new_updates.insert(new_updates.end(), player_updates.begin(), player_updates.end());

        packet_entities->ent_updates_count = new_updates.size();
        packet_entities->ent_updates = new_updates.data();
        msg->message_svc_packet_entities.max_entries = max_entries;
        msg->message_svc_packet_entities.updated_entries += player_updates.size();

        args.data = &msg->message_svc_packet_entities.parsed->data;
        uint32_t bits = dg_bitstream_bits_left(&msg->message_svc_packet_entities.data);
        dg_bitwriter bitwriter;
        dg_bitwriter_init(&bitwriter, bits);
        auto stream = get_start_state(&bitwriter);
        dg_bitwriter_write_packetentities(&bitwriter, args);
        freddie::finalize_stream(&stream, &bitwriter);
        message->memory.attach(bitwriter.ptr); // transfer the bitwriter memory over to the packet
        msg->message_svc_packet_entities.data = stream;
      }
    }
  }
}

static void demo_superimpose(const char *output_path, const char *main_demo_path,
                             const char **demo_paths, size_t demo_count) {
  dg_parse_result result = {0};
  demo_t main_demo;
  std::unique_ptr<demo_t[]> demos(new demo_t[demo_count]);
  std::vector<std::map<int32_t, dg_ent_update>> player_updates;

  // Collect player entity data from demos
  printf("Collecting data...\n");
  for (size_t i = 0; i < demo_count; i++) {
    result = demo_t::parse_demo(&demos[i], demo_paths[i]);
    if (result.error) {
      printf("Error parsing demo %s: %s\n", demo_paths[i], result.error_message);
      return;
    }

    std::map<int32_t, dg_ent_update> player_update;
    collect_player_updates(&demos[i], player_update);
    player_updates.push_back(player_update);
  }

  // Write collected data to one demo
  printf("Writing output...\n");
  result = demo_t::parse_demo(&main_demo, main_demo_path);
  if (result.error) {
    printf("Error parsing demo %s: %s\n", main_demo_path, result.error_message);
    return;
  }

  add_player_updates(&main_demo, player_updates);
  main_demo.write_demo(output_path);

  printf("Done!\n");
}

int main(int argc, char **argv) {
  if (argc < 4) {
    printf("Usage: demosuperimposer <output filepath> <main demo filepath> <input 1> ...\n");
    return 0;
  }

  demo_superimpose(argv[1], argv[2], (const char **)argv + 3, argc - 3);

  return 0;
}
