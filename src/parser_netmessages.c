#include "parser_netmessages.h"
#include "utils.h"
#include "version_utils.h"
#include <string.h>

#define TOSS_STRING() bitstream_read_cstring(stream, scrap, 260);
#define COPY_STRING(variable)                                                                      \
  prev_string = scrap;                                                                             \
  scrap += bitstream_read_cstring(stream, scrap, 260);                                            \
  variable = prev_string;
#define SEND_MESSAGE()                                                                             \
  if (!stream->overflow && thisptr->m_settings.packet_net_message_handler)                         \
    thisptr->m_settings.packet_net_message_handler(thisptr->parent->client_state, message);

static void handle_net_nop(parser *thisptr, bitstream *stream, packet_net_message *message,
                           char *scrap) {
  SEND_MESSAGE();
}

static void handle_net_disconnect(parser *thisptr, bitstream *stream, packet_net_message *message,
                                  char *scrap) {
  char *prev_string = NULL;
  struct demogobbler_net_disconnect *ptr = &message->message_net_disconnect;
  COPY_STRING(ptr->text);
  SEND_MESSAGE();
}

static void handle_net_file(parser *thisptr, bitstream *stream, packet_net_message *message,
                            char *scrap) {
  char *prev_string = NULL;
  struct demogobbler_net_file *ptr = &message->message_net_file;

  ptr->transfer_id = bitstream_read_uint32(stream);
  COPY_STRING(ptr->filename);
  ptr->file_requested = bitstream_read_uint(stream, thisptr->demo_version.net_file_bits);
  SEND_MESSAGE();
}

static void handle_net_tick(parser *thisptr, bitstream *stream, packet_net_message *message,
                            char *scrap) {
  const float net_tick_scale_up = 100000.0f;
  message->message_net_tick.tick = bitstream_read_uint32(stream);
  if (thisptr->demo_version.has_nettick_times) {
    message->message_net_tick.host_frame_time = bitstream_read_uint(stream, 16) / net_tick_scale_up;
    message->message_net_tick.host_frame_time_std_dev =
        bitstream_read_uint(stream, 16) / net_tick_scale_up;
  }
  SEND_MESSAGE();
}

static void handle_net_stringcmd(parser *thisptr, bitstream *stream, packet_net_message *message,
                                 char *scrap) {
  char *prev_string = NULL;
  struct demogobbler_net_stringcmd *ptr = &message->message_net_stringcmd;
  COPY_STRING(ptr->command);
  SEND_MESSAGE();
}

static void handle_net_setconvar(parser *thisptr, bitstream *stream, packet_net_message *message,
                                 char *scrap) {
  char *prev_string = NULL;
  // Reserve space on stack for maximum amount of convars
  struct demogobbler_net_setconvar_convar convars[256];
  struct demogobbler_net_setconvar *ptr = &message->message_net_setconvar;
  ptr->convars = convars;
  ptr->count = bitstream_read_uint(stream, 8);
  for (size_t i = 0; i < message->message_net_setconvar.count; ++i) {
    COPY_STRING(convars[i].name);
    COPY_STRING(convars[i].value);
  }
  SEND_MESSAGE();
}

static void handle_net_signonstate(parser *thisptr, bitstream *stream, packet_net_message *message,
                                   char *scrap) {
  struct demogobbler_net_signonstate *ptr = &message->message_net_signonstate;
  ptr->signon_state = bitstream_read_uint(stream, 8);
  ptr->spawn_count = bitstream_read_sint32(stream);

  if (thisptr->demo_version.demo_protocol >= 4) {
    ptr->NE_num_server_players = bitstream_read_uint32(stream);
    unsigned int length = bitstream_read_uint32(stream) * 8;
    ptr->NE_player_network_ids = bitstream_fork_and_advance(stream, length);
    ptr->NE_map_name_length = bitstream_read_uint32(stream);
    ptr->NE_map_name = scrap;
    bitstream_read_bits(stream, scrap, ptr->NE_map_name_length * 8);
  }
  // Uncrafted: GameState.ClientSoundSequence = 1; reset sound sequence number after receiving
  // SignOn sounds
  SEND_MESSAGE();
}

static void handle_svc_print(parser *thisptr, bitstream *stream, packet_net_message *message,
                             char *scrap) {
  char *prev_string = NULL;
  struct demogobbler_svc_print *ptr = &message->message_svc_print;

  COPY_STRING(ptr->message);

  if(thisptr->demo_version.game == l4d2 && thisptr->demo_version.l4d2_version == 2042) {
    int l4d2_build = 0;
    get_l4d2_build(ptr->message, &l4d2_build);

    if(l4d2_build == 4710) {
      parser_update_l4d2_version(thisptr, 2091);
    }
    else if(l4d2_build == 6403) {
      parser_update_l4d2_version(thisptr, 2147);
    }
  }

  SEND_MESSAGE();
}

static void handle_svc_serverinfo(parser *thisptr, bitstream *stream, packet_net_message *message,
                                  char *scrap) {
  struct demogobbler_svc_serverinfo *ptr = &message->message_svc_serverinfo;
  char *prev_string = NULL;
  ptr->network_protocol = bitstream_read_uint(stream, 16);
  ptr->server_count = bitstream_read_uint32(stream);
  ptr->is_hltv = bitstream_read_uint(stream, 1);
  ptr->is_dedicated = bitstream_read_uint(stream, 1);

  if (thisptr->demo_version.game == l4d2 && thisptr->demo_version.l4d2_version >= 2147)
    ptr->unk_l4d_bit = bitstream_read_uint(stream, 1);
  else
    ptr->unk_l4d_bit = 0;

  ptr->client_crc = bitstream_read_sint32(stream);

  if (thisptr->demo_version.demo_protocol >= 4)
    ptr->stringtable_crc = bitstream_read_uint32(stream);
  else
    ptr->stringtable_crc = 0;

  ptr->max_classes = bitstream_read_uint(stream, 16);

  if (thisptr->demo_version.game == steampipe) {
    bitstream_read_bits(stream, ptr->map_md5, 16 * 8);
    ptr->map_crc = 0;
  } else {
    memset(ptr->map_md5, 0, sizeof(ptr->map_md5));
    ptr->map_crc = bitstream_read_uint32(stream);
  }

  ptr->player_count = bitstream_read_uint(stream, 8);
  ptr->max_clients = bitstream_read_uint(stream, 8);
  ptr->tick_interval = bitstream_read_float(stream);
  ptr->platform = bitstream_read_uint(stream, 8);
  COPY_STRING(ptr->game_dir);
  COPY_STRING(ptr->map_name);
  COPY_STRING(ptr->sky_name);
  COPY_STRING(ptr->host_name);

  if(thisptr->demo_version.game == l4d2 && thisptr->demo_version.l4d2_version >= 2147) {
    COPY_STRING(ptr->mission_name);
    COPY_STRING(ptr->mutation_name);
  }

  if (thisptr->demo_version.game == steampipe)
    ptr->has_replay = bitstream_read_uint(stream, 1);

  SEND_MESSAGE();
}

static void handle_svc_sendtable(parser *thisptr, bitstream *stream, packet_net_message *message,
                                 char *scrap) {
  struct demogobbler_svc_sendtable *ptr = &message->message_svc_sendtable;
  ptr->needs_decoder = bitstream_read_uint(stream, 1);
  ptr->length = bitstream_read_uint(stream, 16);
  ptr->data = bitstream_fork_and_advance(stream, ptr->length);
  SEND_MESSAGE();
}

static void handle_svc_classinfo(parser *thisptr, bitstream *stream, packet_net_message *message,
                                 char *scrap) {
  char *prev_string = NULL;
  struct demogobbler_svc_classinfo *ptr = &message->message_svc_classinfo;
  ptr->length = bitstream_read_uint(stream, 16);
  ptr->create_on_client = bitstream_read_uint(stream, 1);
  unsigned int bits = highest_bit_index(ptr->length) + 1;

  // Could in theory have 32768 classes so use the allocator for this
  if (!ptr->create_on_client) {
    blk allocated = allocator_alloc(
      &thisptr->allocator, ptr->length * sizeof(struct demogobbler_svc_classinfo_serverclass));
    ptr->server_classes = allocated.address;
    for (unsigned int i = 0; i < ptr->length && !stream->overflow; ++i) {
      ptr->server_classes[i].class_id = bitstream_read_uint(stream, bits);
      COPY_STRING(ptr->server_classes[i].class_name);
      COPY_STRING(ptr->server_classes[i].datatable_name);
    }
    allocator_dealloc(&thisptr->allocator, allocated);
  }
  else {
    ptr->server_classes = NULL;
  }
  SEND_MESSAGE();
}

static void handle_svc_setpause(parser *thisptr, bitstream *stream, packet_net_message *message,
                                char *scrap) {
  message->message_svc_setpause.paused = bitstream_read_uint(stream, 1);
  SEND_MESSAGE();
}

static void handle_svc_create_stringtable(parser *thisptr, bitstream *stream,
                                          packet_net_message *message, char *scrap) {
  struct demogobbler_svc_create_stringtable *ptr = &message->message_svc_create_stringtable;
  char *prev_string = NULL;
  COPY_STRING(ptr->name);
  unsigned int max_entries = bitstream_read_uint(stream, 16);
  unsigned int num_entries_bits = highest_bit_index(max_entries) + 1;
  ptr->max_entries = max_entries;
  ptr->num_entries = bitstream_read_uint(stream, num_entries_bits);

  if (thisptr->demo_version.game == steampipe) {
    ptr->data_length = bitstream_read_varuint32(stream);
  } else {
    ptr->data_length = bitstream_read_uint(stream, thisptr->demo_version.stringtable_userdata_size_bits);
  }
  
  bool fixed_size_data = bitstream_read_uint(stream, 1);

  if (fixed_size_data) {
    ptr->user_data_fixed_size = bitstream_read_uint(stream, 12);
    ptr->user_data_size_bits = bitstream_read_uint(stream, 4);
  } else {
    ptr->user_data_fixed_size = 0;
    ptr->user_data_size_bits = 0;
  }

  if (thisptr->demo_version.network_protocol >= 15) {
    ptr->flags = bitstream_read_uint(stream, thisptr->demo_version.stringtable_flags_bits);
  }

  ptr->data = bitstream_fork_and_advance(stream, ptr->data_length);

  SEND_MESSAGE();
}

static void handle_svc_update_stringtable(parser *thisptr, bitstream *stream,
                                          packet_net_message *message, char *scrap) {
  struct demogobbler_svc_update_stringtable *ptr = &message->message_svc_update_stringtable;
  ptr->table_id = bitstream_read_uint(stream, 5);
  ptr->exists = bitstream_read_uint(stream, 1);
  if (ptr->exists) {
    ptr->changed_entries = bitstream_read_uint(stream, 16);
  } else {
    ptr->changed_entries = 1;
  }

  if(thisptr->demo_version.network_protocol <= 7) {
    ptr->data_length = bitstream_read_uint(stream, 16);
  }
  else {
    ptr->data_length = bitstream_read_uint(stream, 20);
  }
  ptr->data = bitstream_fork_and_advance(stream, ptr->data_length);
  SEND_MESSAGE();
}

static void handle_svc_voice_init(parser *thisptr, bitstream *stream, packet_net_message *message,
                                  char *scrap) {
  struct demogobbler_svc_voice_init *ptr = &message->message_svc_voice_init;
  char *prev_string = NULL;

  COPY_STRING(ptr->codec);
  ptr->quality = bitstream_read_uint(stream, 8);
  if (ptr->quality == 255) {
    // Steampipe version uses shorts
    if(thisptr->demo_version.game == steampipe) {
      ptr->unk = bitstream_read_uint(stream, 16);
    }
    // Protocol 4 version uses floats?
    else if (thisptr->demo_version.demo_protocol == 4) {
      ptr->unk = bitstream_read_float(stream);
    }
    // Not available on other versions
  }
}

static void handle_svc_voice_data(parser *thisptr, bitstream *stream, packet_net_message *message,
                                  char *scrap) {
  struct demogobbler_svc_voice_data *ptr = &message->message_svc_voice_data;
  ptr->client = bitstream_read_uint(stream, 8);
  ptr->proximity = bitstream_read_uint(stream, 8);
  ptr->length = bitstream_read_uint(stream, 16);
  ptr->data = bitstream_fork_and_advance(stream, ptr->length);
  SEND_MESSAGE();
}

static void handle_svc_sounds(parser *thisptr, bitstream *stream, packet_net_message *message,
                              char *scrap) {
  struct demogobbler_svc_sounds *ptr = &message->message_svc_sounds;
  ptr->reliable_sound = bitstream_read_uint(stream, 1);
  if (ptr->reliable_sound) {
    ptr->sounds = 1;
    ptr->length = bitstream_read_uint(stream, 8);
  } else {
    ptr->sounds = bitstream_read_uint(stream, 8);
    ptr->length = bitstream_read_uint(stream, 16);
  }
  ptr->data = bitstream_fork_and_advance(stream, ptr->length);
  SEND_MESSAGE();
}

static void handle_svc_setview(parser *thisptr, bitstream *stream, packet_net_message *message,
                               char *scrap) {
  struct demogobbler_svc_setview *ptr = &message->message_svc_setview;
  ptr->entity_index = bitstream_read_uint(stream, 11);
  SEND_MESSAGE();
}

static void handle_svc_fixangle(parser *thisptr, bitstream *stream, packet_net_message *message,
                                char *scrap) {
  struct demogobbler_svc_fixangle *ptr = &message->message_svc_fixangle;
  ptr->relative = bitstream_read_uint(stream, 1);
  ptr->angle = bitstream_read_bitvector(stream, 16);
  SEND_MESSAGE();
}

static void handle_svc_crosshair_angle(parser *thisptr, bitstream *stream,
                                       packet_net_message *message, char *scrap) {
  struct demogobbler_svc_crosshair_angle *ptr = &message->message_svc_crosshair_angle;
  ptr->angle = bitstream_read_bitvector(stream, 16);
  SEND_MESSAGE();
}

static void handle_svc_bsp_decal(parser *thisptr, bitstream *stream, packet_net_message *message,
                                 char *scrap) {
  struct demogobbler_svc_bsp_decal *ptr = &message->message_svc_bsp_decal;
  ptr->pos = bitstream_read_coordvector(stream);
  ptr->decal_texture_index = bitstream_read_uint(stream, 9);
  ptr->index_bool = bitstream_read_uint(stream, 1);

  if(ptr->index_bool) {
    ptr->entity_index = bitstream_read_uint(stream, 11);
    ptr->model_index = bitstream_read_uint(stream, thisptr->demo_version.model_index_bits);
  }
  ptr->lowpriority = bitstream_read_uint(stream, 1);
  SEND_MESSAGE();
}

static void handle_svc_user_message(parser *thisptr, bitstream *stream, packet_net_message *message,
                                    char *scrap) {
  struct demogobbler_svc_user_message *ptr = &message->message_svc_user_message;
  ptr->msg_type = bitstream_read_uint(stream, 8);
  ptr->length = bitstream_read_uint(stream, thisptr->demo_version.svc_user_message_bits);
  ptr->data = bitstream_fork_and_advance(stream, ptr->length);
  SEND_MESSAGE();
}

static void handle_svc_entity_message(parser *thisptr, bitstream *stream,
                                      packet_net_message *message, char *scrap) {
  struct demogobbler_svc_entity_message *ptr = &message->message_svc_entity_message;
  ptr->entity_index = bitstream_read_uint(stream, 11);
  ptr->class_id = bitstream_read_uint(stream, 9);
  ptr->length = bitstream_read_uint(stream, 11);
  ptr->data = bitstream_fork_and_advance(stream, ptr->length);
  SEND_MESSAGE();
}

static void handle_svc_game_event(parser *thisptr, bitstream *stream, packet_net_message *message,
                                  char *scrap) {
  struct demogobbler_svc_game_event *ptr = &message->message_svc_game_event;
  ptr->length = bitstream_read_uint(stream, 11);
  ptr->data = bitstream_fork_and_advance(stream, ptr->length);

  SEND_MESSAGE();
}

static void handle_svc_packet_entities(parser *thisptr, bitstream *stream,
                                       packet_net_message *message, char *scrap) {
  struct demogobbler_svc_packet_entities *ptr = &message->message_svc_packet_entities;
  ptr->max_entries = bitstream_read_uint(stream, 11);
  ptr->is_delta = bitstream_read_uint(stream, 1);

  if (ptr->is_delta) {
    ptr->delta_from = bitstream_read_sint32(stream);
  } else {
    ptr->delta_from = -1;
  }

  ptr->base_line = bitstream_read_uint(stream, 1);
  ptr->updated_entries = bitstream_read_uint(stream, 11);
  ptr->data_length = bitstream_read_uint(stream, 20);
  ptr->update_baseline = bitstream_read_uint(stream, 1);
  ptr->data = bitstream_fork_and_advance(stream, ptr->data_length);
  SEND_MESSAGE();
}

static void handle_svc_temp_entities(parser *thisptr, bitstream *stream,
                                     packet_net_message *message, char *scrap) {
  struct demogobbler_svc_temp_entities *ptr = &message->message_svc_temp_entities;
  ptr->num_entries = bitstream_read_uint(stream, 8);

  if(thisptr->demo_version.game == steampipe) {
    ptr->data_length = bitstream_read_varuint32(stream);
  }
  else if(thisptr->demo_version.game == l4d2) {
    ptr->data_length = bitstream_read_uint(stream, 18);
  }
  else {
    ptr->data_length = bitstream_read_uint(stream, 17);
  }
  ptr->data = bitstream_fork_and_advance(stream, ptr->data_length);

  SEND_MESSAGE();
}

static void handle_svc_prefetch(parser *thisptr, bitstream *stream, packet_net_message *message,
                                char *scrap) {
  struct demogobbler_svc_prefetch *ptr = &message->message_svc_prefetch;
  ptr->sound_index = bitstream_read_uint(stream, thisptr->demo_version.svc_prefetch_bits);
  SEND_MESSAGE();
}

static void handle_svc_menu(parser *thisptr, bitstream *stream, packet_net_message *message,
                            char *scrap) {
  struct demogobbler_svc_menu *ptr = &message->message_svc_menu;
  ptr->menu_type = bitstream_read_uint(stream, 16);
  ptr->data_length = bitstream_read_uint32(stream);
  ptr->data = bitstream_fork_and_advance(stream, ptr->data_length);
  SEND_MESSAGE();
}

static void handle_svc_game_event_list(parser *thisptr, bitstream *stream,
                                       packet_net_message *message, char *scrap) {
  struct demogobbler_svc_game_event_list *ptr = &message->message_svc_game_event_list;
  ptr->events = bitstream_read_uint(stream, 9);
  ptr->length = bitstream_read_uint(stream, 20);
  ptr->data = bitstream_fork_and_advance(stream, ptr->length);
  SEND_MESSAGE();
}

static void handle_svc_get_cvar_value(parser *thisptr, bitstream *stream,
                                      packet_net_message *message, char *scrap) {
  char* prev_string = NULL;
  struct demogobbler_svc_get_cvar_value *ptr = &message->message_svc_get_cvar_value;
  ptr->cookie = bitstream_read_sint32(stream);
  COPY_STRING(ptr->cvar_name);
  SEND_MESSAGE();
}

static void handle_net_splitscreen_user(parser *thisptr, bitstream *stream,
                                        packet_net_message *message, char *scrap) {
  message->message_net_splitscreen_user.unk = bitstream_read_uint(stream, 1);
  SEND_MESSAGE();
}

static void handle_svc_splitscreen(parser *thisptr, bitstream *stream, packet_net_message *message,
                                   char *scrap) {
  struct demogobbler_svc_splitscreen *ptr = &message->message_svc_splitscreen;
  ptr->remove_user = bitstream_read_uint(stream, 1);
  ptr->data_length = bitstream_read_uint(stream, 11);
  ptr->data = bitstream_fork_and_advance(stream, ptr->data_length);
  SEND_MESSAGE();
}

static void handle_svc_paintmap_data(parser *thisptr, bitstream *stream,
                                     packet_net_message *message, char *scrap) {
  thisptr->error = true;
  thisptr->error_message = "svc_paintmap_data parsing is not implemented";
  SEND_MESSAGE();
}

static void handle_svc_cmd_key_values(parser *thisptr, bitstream *stream,
                                      packet_net_message *message, char *scrap) {
  struct demogobbler_svc_cmd_key_values *ptr = &message->message_svc_cmd_key_values;
  ptr->data_length = bitstream_read_uint32(stream);
  ptr->data = bitstream_fork_and_advance(stream, ptr->data_length * 8);
  SEND_MESSAGE();
}

static net_message_type parser_get_message_type(parser *thisptr, unsigned int value) {
  net_message_type *list = thisptr->demo_version.netmessage_array;
  size_t count = thisptr->demo_version.netmessage_count;

  if (value >= count || list[value] == svc_invalid) {
    thisptr->error = true;
    thisptr->error_message = "Encountered bad net message type.\n";
    return svc_invalid;
  } else {
    return list[value];
  }
}

void parse_netmessages(parser *thisptr, void *data, size_t size) {
  bitstream stream = demogobbler_bitstream_create(data, size * 8);
  //fprintf(stderr, "packet start:\n");

  // We allocate a single scrap buffer for the duration of parsing the packet that is as large as
  // the whole packet Should never run out of space as long as we don't make things larger as we
  // read it out
  blk scrap_blk = allocator_alloc(&thisptr->allocator, size);
  char *scrap = scrap_blk.address;
  unsigned int bits = thisptr->demo_version.netmessage_type_bits;

  while (demogobbler_bitstream_bits_left(&stream) >= bits && !thisptr->error && !stream.overflow) {
    unsigned int type_index = bitstream_read_uint(&stream, bits);
    net_message_type type = parser_get_message_type(thisptr, type_index);
    //fprintf(stderr, "%d : %d\n", type_index, type);
    packet_net_message message;
    message._mtype = type_index;
    message.mtype = type;

#define DECLARE_SWITCH_STATEMENT(message_type)                                                     \
  case message_type:                                                                               \
    handle_##message_type(thisptr, &stream, &message, scrap);                                      \
    break;

    if (type != svc_invalid) {
      switch (type) {
        DEMOGOBBLER_MACRO_ALL_MESSAGES(DECLARE_SWITCH_STATEMENT);
      default:
        thisptr->error = true;
        thisptr->error_message = "No handler for this type of message.";
        break;
      }
    }
  }

  if (stream.overflow && !thisptr->error) {
    thisptr->error = true;
    thisptr->error_message = "Bitstream overflowed during packet parsing";
  }

  /*if (thisptr->error) {
    printf("%s\n", thisptr->error_message);
    thisptr->error = false;
  }*/

  allocator_dealloc(&thisptr->allocator, scrap_blk);
  //fprintf(stderr, "packet end:\n");
}
