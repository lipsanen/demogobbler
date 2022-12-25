#include "parser_netmessages.h"
#include "demogobbler/allocator.h"
#include "demogobbler/bitstream.h"
#include "demogobbler/bitwriter.h"
#include "parser_packetentities.h"
#include "utils.h"
#include "vector_array.h"
#include "version_utils.h"
#include <string.h>

#define TOSS_STRING() dg_bitstream_read_cstring(stream, scrap, 260);
#define COPY_STRING(variable)                                                                      \
  {                                                                                                \
    char *temp_string = scrap->address;                                                            \
    size_t bytes = dg_bitstream_read_cstring(stream, scrap->address, scrap->size);                    \
    scrap->address = (uint8_t *)scrap->address + bytes;                                            \
    scrap->size -= bytes;                                                                          \
    variable = temp_string;                                                                        \
  }

#ifdef DEBUG
#define SEND_MESSAGE() message->offset = stream->bitoffset;
#else
#define SEND_MESSAGE()
#endif

typedef struct {
  void *address;
  size_t size;
} blk;

static void handle_net_nop(dg_parser *thisptr, dg_bitstream *stream, packet_net_message *message,
                           blk *scrap) {
  SEND_MESSAGE();
}

static void write_net_nop(dg_bitwriter *writer, dg_demver_data *version,
                          packet_net_message *message) {}

static void handle_net_disconnect(dg_parser *thisptr, dg_bitstream *stream,
                                  packet_net_message *message, blk *scrap) {
  struct dg_net_disconnect *ptr = &message->message_net_disconnect;
  COPY_STRING(ptr->text);
  SEND_MESSAGE();
}

static void write_net_disconnect(dg_bitwriter *writer, dg_demver_data *version,
                                 packet_net_message *message) {
  struct dg_net_disconnect *ptr = &message->message_net_disconnect;
  dg_bitwriter_write_cstring(writer, ptr->text);
}

static void handle_net_file(dg_parser *thisptr, dg_bitstream *stream, packet_net_message *message,
                            blk *scrap) {
  struct dg_net_file *ptr = &message->message_net_file;

  ptr->transfer_id = dg_bitstream_read_uint32(stream);
  COPY_STRING(ptr->filename);
  ptr->file_requested = dg_bitstream_read_uint(stream, thisptr->demo_version.net_file_bits);
  SEND_MESSAGE();
}

static void write_net_file(dg_bitwriter *writer, dg_demver_data *version,
                           packet_net_message *message) {
  struct dg_net_file *ptr = &message->message_net_file;
  dg_bitwriter_write_uint32(writer, ptr->transfer_id);
  dg_bitwriter_write_cstring(writer, ptr->filename);
  dg_bitwriter_write_uint(writer, ptr->file_requested, version->net_file_bits);
}

static void handle_net_tick(dg_parser *thisptr, dg_bitstream *stream, packet_net_message *message,
                            blk *scrap) {
  struct dg_net_tick *ptr = &message->message_net_tick;
  ptr->tick = dg_bitstream_read_uint32(stream);
  if (thisptr->demo_version.has_nettick_times) {
    ptr->host_frame_time = dg_bitstream_read_uint(stream, 16);
    ptr->host_frame_time_std_dev = dg_bitstream_read_uint(stream, 16);
  }
  SEND_MESSAGE();
}

static void write_net_tick(dg_bitwriter *writer, dg_demver_data *version,
                           packet_net_message *message) {
  struct dg_net_tick *ptr = &message->message_net_tick;
  dg_bitwriter_write_uint32(writer, ptr->tick);

  if (version->has_nettick_times) {
    dg_bitwriter_write_uint(writer, ptr->host_frame_time, 16);
    dg_bitwriter_write_uint(writer, ptr->host_frame_time_std_dev, 16);
  }
}

static void handle_net_stringcmd(dg_parser *thisptr, dg_bitstream *stream, packet_net_message *message,
                                 blk *scrap) {
  struct dg_net_stringcmd *ptr = &message->message_net_stringcmd;
  COPY_STRING(ptr->command);
  SEND_MESSAGE();
}

static void write_net_stringcmd(dg_bitwriter *writer, dg_demver_data *version,
                                packet_net_message *message) {
  struct dg_net_stringcmd *ptr = &message->message_net_stringcmd;
  dg_bitwriter_write_cstring(writer, ptr->command);
}

static void handle_net_setconvar(dg_parser *thisptr, dg_bitstream *stream, packet_net_message *message,
                                 blk *scrap) {
  // Reserve space on stack for maximum amount of convars
  dg_alloc_state* arena = dg_parser_temp_allocator(thisptr);
  struct dg_net_setconvar *ptr = &message->message_net_setconvar;
  ptr->count = dg_bitstream_read_uint(stream, 8);
  ptr->convars =
      dg_alloc_allocate(arena, sizeof(struct dg_net_setconvar_convar) * ptr->count,
                        alignof(struct dg_net_setconvar_convar));
  for (size_t i = 0; i < message->message_net_setconvar.count; ++i) {
    COPY_STRING(ptr->convars[i].name);
    COPY_STRING(ptr->convars[i].value);
  }

  SEND_MESSAGE();
}

static void write_net_setconvar(dg_bitwriter *writer, dg_demver_data *version,
                                packet_net_message *message) {
  struct dg_net_setconvar *ptr = &message->message_net_setconvar;
  dg_bitwriter_write_uint(writer, ptr->count, 8);

  for (size_t i = 0; i < ptr->count; ++i) {
    dg_bitwriter_write_cstring(writer, ptr->convars[i].name);
    dg_bitwriter_write_cstring(writer, ptr->convars[i].value);
  }
}

static void handle_net_signonstate(dg_parser *thisptr, dg_bitstream *stream,
                                   packet_net_message *message, blk *scrap) {
  struct dg_net_signonstate *ptr = &message->message_net_signonstate;
  ptr->signon_state = dg_bitstream_read_uint(stream, 8);
  ptr->spawn_count = dg_bitstream_read_sint32(stream);

  if (thisptr->demo_version.demo_protocol >= 4) {
    ptr->NE_num_server_players = dg_bitstream_read_uint32(stream);
    unsigned int length = dg_bitstream_read_uint32(stream) * 8;
    ptr->NE_player_network_ids = dg_bitstream_fork_and_advance(stream, length);
    ptr->NE_map_name_length = dg_bitstream_read_uint32(stream);
    ptr->NE_map_name = scrap->address;

    if (scrap->size < ptr->NE_map_name_length) {
      thisptr->error = true;
      thisptr->error_message = "Map name in net_signonstate has bad length";
    } else {
      dg_bitstream_read_fixed_string(stream, scrap->address, ptr->NE_map_name_length);
    }
  }
  // Uncrafted: GameState.ClientSoundSequence = 1; reset sound sequence number after receiving
  // SignOn sounds
  SEND_MESSAGE();
}

static void write_net_signonstate(dg_bitwriter *writer, dg_demver_data *version,
                                  packet_net_message *message) {
  struct dg_net_signonstate *ptr = &message->message_net_signonstate;
  dg_bitwriter_write_uint(writer, ptr->signon_state, 8);
  dg_bitwriter_write_uint32(writer, ptr->spawn_count);

  if (version->demo_protocol >= 4) {
    dg_bitwriter_write_uint32(writer, ptr->NE_num_server_players);
    uint32_t length =
        (ptr->NE_player_network_ids.bitsize - ptr->NE_player_network_ids.bitoffset) / 8;
    dg_bitwriter_write_uint32(writer, length);
    dg_bitwriter_write_bitstream(writer, &ptr->NE_player_network_ids);
    dg_bitwriter_write_uint32(writer, ptr->NE_map_name_length);
    dg_bitwriter_write_bits(writer, ptr->NE_map_name, ptr->NE_map_name_length * 8);
  }
}

static void handle_svc_print(dg_parser *thisptr, dg_bitstream *stream, packet_net_message *message,
                             blk *scrap) {
  struct dg_svc_print *ptr = &message->message_svc_print;

  COPY_STRING(ptr->message);

  if (thisptr->demo_version.game == l4d2 && thisptr->demo_version.l4d2_version == 2042) {
    int l4d2_build = 0;
    get_l4d2_build(ptr->message, &l4d2_build);

    if (l4d2_build == 4710) {
      dg_parser_update_l4d2_version(thisptr, 2091);
    } else if (l4d2_build == 6403) {
      dg_parser_update_l4d2_version(thisptr, 2147);
    }
  }

  SEND_MESSAGE();
}

static void write_svc_print(dg_bitwriter *writer, dg_demver_data *version,
                            packet_net_message *message) {
  struct dg_svc_print *ptr = &message->message_svc_print;
  dg_bitwriter_write_cstring(writer, ptr->message);
}

static void handle_svc_serverinfo(dg_parser *thisptr, dg_bitstream *stream,
                                  packet_net_message *message, blk *scrap) {
  dg_alloc_state* arena = dg_parser_temp_allocator(thisptr);
  message->message_svc_serverinfo = dg_alloc_allocate(
      arena, sizeof(struct dg_svc_serverinfo), alignof(struct dg_svc_serverinfo));
  struct dg_svc_serverinfo *ptr = message->message_svc_serverinfo;
  ptr->network_protocol = dg_bitstream_read_uint(stream, 16);
  ptr->server_count = dg_bitstream_read_uint32(stream);
  ptr->is_hltv = dg_bitstream_read_bit(stream);
  ptr->is_dedicated = dg_bitstream_read_bit(stream);

  if (thisptr->demo_version.game == l4d2 && thisptr->demo_version.l4d2_version >= 2147)
    ptr->unk_l4d_bit = dg_bitstream_read_bit(stream);
  else
    ptr->unk_l4d_bit = 0;

  ptr->client_crc = dg_bitstream_read_sint32(stream);

  if (thisptr->demo_version.demo_protocol >= 4)
    ptr->stringtable_crc = dg_bitstream_read_uint32(stream);
  else
    ptr->stringtable_crc = 0;

  ptr->max_classes = dg_bitstream_read_uint(stream, 16);

  if (thisptr->demo_version.game == steampipe) {
    dg_bitstream_read_fixed_string(stream, ptr->map_md5, 16);
    ptr->map_crc = 0;
  } else {
    memset(ptr->map_md5, 0, sizeof(ptr->map_md5));
    ptr->map_crc = dg_bitstream_read_uint32(stream);
  }

  ptr->player_count = dg_bitstream_read_uint(stream, 8);
  ptr->max_clients = dg_bitstream_read_uint(stream, 8);
  ptr->tick_interval = dg_bitstream_read_float(stream);
  ptr->platform = dg_bitstream_read_uint(stream, 8);
  COPY_STRING(ptr->game_dir);
  COPY_STRING(ptr->map_name);
  COPY_STRING(ptr->sky_name);
  COPY_STRING(ptr->host_name);

  if (thisptr->demo_version.game == l4d2 && thisptr->demo_version.l4d2_version >= 2147) {
    COPY_STRING(ptr->mission_name);
    COPY_STRING(ptr->mutation_name);
  }

  if (thisptr->demo_version.game == steampipe)
    ptr->has_replay = dg_bitstream_read_bit(stream);

  SEND_MESSAGE();
}

static void write_svc_serverinfo(dg_bitwriter *writer, dg_demver_data *version,
                                 packet_net_message *message) {
  struct dg_svc_serverinfo *ptr = message->message_svc_serverinfo;

  dg_bitwriter_write_uint(writer, ptr->network_protocol, 16);
  dg_bitwriter_write_uint32(writer, ptr->server_count);
  dg_bitwriter_write_bit(writer, ptr->is_hltv);
  dg_bitwriter_write_bit(writer, ptr->is_dedicated);

  if (version->game == l4d2 && version->l4d2_version >= 2147)
    dg_bitwriter_write_bit(writer, ptr->unk_l4d_bit);

  dg_bitwriter_write_sint32(writer, ptr->client_crc);

  if (version->demo_protocol >= 4)
    dg_bitwriter_write_uint32(writer, ptr->stringtable_crc);

  dg_bitwriter_write_uint(writer, ptr->max_classes, 16);

  if (version->game == steampipe) {
    dg_bitwriter_write_bits(writer, ptr->map_md5, 16 * 8);
  } else {
    dg_bitwriter_write_uint32(writer, ptr->map_crc);
  }

  dg_bitwriter_write_uint(writer, ptr->player_count, 8);
  dg_bitwriter_write_uint(writer, ptr->max_clients, 8);
  dg_bitwriter_write_float(writer, ptr->tick_interval);
  dg_bitwriter_write_uint(writer, ptr->platform, 8);

  dg_bitwriter_write_cstring(writer, ptr->game_dir);
  dg_bitwriter_write_cstring(writer, ptr->map_name);
  dg_bitwriter_write_cstring(writer, ptr->sky_name);
  dg_bitwriter_write_cstring(writer, ptr->host_name);

  if (version->game == l4d2 && version->l4d2_version >= 2147) {
    dg_bitwriter_write_cstring(writer, ptr->mission_name);
    dg_bitwriter_write_cstring(writer, ptr->mutation_name);
  }

  if (version->game == steampipe)
    dg_bitwriter_write_bit(writer, ptr->has_replay);
}

static void handle_svc_sendtable(dg_parser *thisptr, dg_bitstream *stream, packet_net_message *message,
                                 blk *scrap) {
  struct dg_svc_sendtable *ptr = &message->message_svc_sendtable;
  ptr->needs_decoder = dg_bitstream_read_bit(stream);
  ptr->length = dg_bitstream_read_uint(stream, 16);
  ptr->data = dg_bitstream_fork_and_advance(stream, ptr->length);
  SEND_MESSAGE();
}

static void write_svc_sendtable(dg_bitwriter *writer, dg_demver_data *version,
                                packet_net_message *message) {
  writer->error = true;
  writer->error_message = "Writing not implemented for svc_sendtable";
}

static void handle_svc_classinfo(dg_parser *thisptr, dg_bitstream *stream, packet_net_message *message,
                                 blk *scrap) {
  struct dg_svc_classinfo *ptr = &message->message_svc_classinfo;
  ptr->length = dg_bitstream_read_uint(stream, 16);
  ptr->create_on_client = dg_bitstream_read_bit(stream);
  unsigned int bits = highest_bit_index(ptr->length) + 1;
  dg_alloc_state* arena = dg_parser_temp_allocator(thisptr);

  // Could in theory have 32768 classes so use the allocator for this
  if (!ptr->create_on_client) {
    ptr->server_classes = dg_alloc_allocate(
        arena, ptr->length * sizeof(struct dg_svc_classinfo_serverclass), 1);
    for (unsigned int i = 0; i < ptr->length && !stream->overflow; ++i) {
      ptr->server_classes[i].class_id = dg_bitstream_read_uint(stream, bits);
      COPY_STRING(ptr->server_classes[i].class_name);
      COPY_STRING(ptr->server_classes[i].datatable_name);
    }
  } else {
    ptr->server_classes = NULL;
  }
  SEND_MESSAGE();
}

static void write_svc_classinfo(dg_bitwriter *writer, dg_demver_data *version,
                                packet_net_message *message) {
  struct dg_svc_classinfo *ptr = &message->message_svc_classinfo;
  dg_bitwriter_write_uint(writer, ptr->length, 16);
  dg_bitwriter_write_bit(writer, ptr->create_on_client);
  unsigned int bits = highest_bit_index(ptr->length) + 1;

  if (!ptr->create_on_client) {
    for (unsigned int i = 0; i < ptr->length; ++i) {
      dg_bitwriter_write_uint(writer, ptr->server_classes[i].class_id, bits);
      dg_bitwriter_write_cstring(writer, ptr->server_classes[i].class_name);
      dg_bitwriter_write_cstring(writer, ptr->server_classes[i].datatable_name);
    }
  }
}

static void handle_svc_setpause(dg_parser *thisptr, dg_bitstream *stream, packet_net_message *message,
                                blk *scrap) {
  message->message_svc_setpause.paused = dg_bitstream_read_bit(stream);
  SEND_MESSAGE();
}

static void write_svc_setpause(dg_bitwriter *writer, dg_demver_data *version,
                               packet_net_message *message) {
  struct dg_svc_setpause *ptr = &message->message_svc_setpause;
  dg_bitwriter_write_bit(writer, ptr->paused);
}

static void handle_svc_create_stringtable(dg_parser *thisptr, dg_bitstream *stream,
                                          packet_net_message *message, blk *scrap) {
  struct dg_svc_create_stringtable *ptr = &message->message_svc_create_stringtable;
  COPY_STRING(ptr->name);
  ptr->max_entries = dg_bitstream_read_uint(stream, 16);
  unsigned int num_entries_bits = highest_bit_index(ptr->max_entries) + 1;
  ptr->num_entries = dg_bitstream_read_uint(stream, num_entries_bits);
  uint32_t data_length;

  if (thisptr->demo_version.game == steampipe) {
    data_length = dg_bitstream_read_varuint32(stream);
  } else {
    data_length =
        dg_bitstream_read_uint(stream, thisptr->demo_version.stringtable_userdata_size_bits);
  }

  ptr->user_data_fixed_size = dg_bitstream_read_bit(stream);

  if (ptr->user_data_fixed_size) {
    ptr->user_data_size = dg_bitstream_read_uint(stream, 12);
    ptr->user_data_size_bits = dg_bitstream_read_uint(stream, 4);
  } else {
    ptr->user_data_size = 0;
    ptr->user_data_size_bits = 0;
  }

  if (thisptr->demo_version.network_protocol >= 15) {
    ptr->flags = dg_bitstream_read_uint(stream, thisptr->demo_version.stringtable_flags_bits);
  } else {
    ptr->flags = 0;
  }

  ptr->data = dg_bitstream_fork_and_advance(stream, data_length);

  dg_parse_result result;
  if((ptr->flags & 1) == 0 && thisptr->demo_version.demo_protocol <= 3) {
    dg_sentry_parse_args args;
    args.allocator = dg_parser_temp_allocator(thisptr);
    args.flags = ptr->flags;
    args.demver_data = &thisptr->demo_version;
    args.max_entries = ptr->max_entries;
    args.num_updated_entries = ptr->num_entries;
    args.stream = ptr->data;
    args.user_data_fixed_size = ptr->user_data_size;
    args.user_data_size_bits = ptr->user_data_size_bits;
    result = dg_parse_stringtable_entry(&args, &ptr->stringtable);

    if(!result.error) {
      result = dg_parser_add_stringtable(thisptr, &ptr->stringtable);
    }

  } else {
    ptr->stringtable.dictionary_enabled = false;
    ptr->stringtable.flags = ptr->flags;
    ptr->stringtable.max_entries = ptr->max_entries;
    ptr->stringtable.user_data_fixed_size = ptr->user_data_fixed_size;
    ptr->stringtable.user_data_size_bits = ptr->user_data_size_bits;
    result = dg_parser_add_stringtable(thisptr, &ptr->stringtable);
  }

  thisptr->error = result.error;
  thisptr->error_message = result.error_message;

  SEND_MESSAGE();
}

static void write_svc_create_stringtable(dg_bitwriter *writer, dg_demver_data *version,
                                         packet_net_message *message) {
  struct dg_svc_create_stringtable *ptr = &message->message_svc_create_stringtable;
  dg_bitwriter_write_cstring(writer, ptr->name);
  dg_bitwriter_write_uint(writer, ptr->max_entries, 16);
  unsigned int num_entries_bits = highest_bit_index(ptr->max_entries) + 1;
  dg_bitwriter_write_uint(writer, ptr->num_entries, num_entries_bits);
  uint32_t bits_left = dg_bitstream_bits_left(&ptr->data);

  if (version->game == steampipe) {
    dg_bitwriter_write_varuint32(writer, bits_left);
  } else {
    dg_bitwriter_write_uint(writer, bits_left, version->stringtable_userdata_size_bits);
  }

  dg_bitwriter_write_bit(writer, ptr->user_data_fixed_size);

  if (ptr->user_data_fixed_size) {
    dg_bitwriter_write_uint(writer, ptr->user_data_size, 12);
    dg_bitwriter_write_uint(writer, ptr->user_data_size_bits, 4);
  }

  if (version->network_protocol >= 15) {
    dg_bitwriter_write_uint(writer, ptr->flags, version->stringtable_flags_bits);
  }

  dg_bitwriter_write_bitstream(writer, &ptr->data);
}

static void handle_svc_update_stringtable(dg_parser *thisptr, dg_bitstream *stream,
                                          packet_net_message *message, blk *scrap) {
  struct dg_svc_update_stringtable *ptr = &message->message_svc_update_stringtable;
  ptr->table_id =
      dg_bitstream_read_uint(stream, thisptr->demo_version.svc_update_stringtable_table_id_bits);
  ptr->exists = dg_bitstream_read_bit(stream);
  if (ptr->exists) {
    ptr->changed_entries = dg_bitstream_read_uint(stream, 16);
  } else {
    ptr->changed_entries = 1;
  }

  uint32_t data_length;

  if (thisptr->demo_version.network_protocol <= 7) {
    data_length = dg_bitstream_read_uint(stream, 16);
  } else {
    data_length = dg_bitstream_read_uint(stream, 20);
  }
  ptr->data = dg_bitstream_fork_and_advance(stream, data_length);

  if(ptr->table_id < thisptr->state.stringtables_count && thisptr->demo_version.demo_protocol <= 3) {
    dg_stringtable_data *data = thisptr->state.stringtables + ptr->table_id;
    dg_sentry_parse_args args;
    args.allocator = dg_parser_temp_allocator(thisptr);
    args.demver_data = &thisptr->demo_version;
    args.flags = 0;
    args.max_entries = data->max_entries;
    args.num_updated_entries = ptr->changed_entries;
    args.stream = ptr->data;
    args.user_data_fixed_size = data->user_data_fixed_size;
    args.user_data_size_bits = data->user_data_size_bits;
    
    dg_parse_result result = dg_parse_stringtable_entry(&args, &ptr->parsed_sentry);
    thisptr->error = result.error;
    thisptr->error_message = result.error_message;
  }

  SEND_MESSAGE();
}

static void write_svc_update_stringtable(dg_bitwriter *writer, dg_demver_data *version,
                                         packet_net_message *message) {
  struct dg_svc_update_stringtable *ptr = &message->message_svc_update_stringtable;
  dg_bitwriter_write_uint(writer, ptr->table_id, version->svc_update_stringtable_table_id_bits);
  dg_bitwriter_write_bit(writer, ptr->exists);

  if (ptr->exists) {
    dg_bitwriter_write_uint(writer, ptr->changed_entries, 16);
  }

  uint32_t data_length = dg_bitstream_bits_left(&ptr->data);

  if (version->network_protocol <= 7) {
    dg_bitwriter_write_uint(writer, data_length, 16);
  } else {
    dg_bitwriter_write_uint(writer, data_length, 20);
  }

  dg_bitwriter_write_bitstream(writer, &ptr->data);
}

static void handle_svc_voice_init(dg_parser *thisptr, dg_bitstream *stream,
                                  packet_net_message *message, blk *scrap) {
  struct dg_svc_voice_init *ptr = &message->message_svc_voice_init;

  COPY_STRING(ptr->codec);
  ptr->quality = dg_bitstream_read_uint(stream, 8);
  if (ptr->quality == 255) {
    // Steampipe version uses shorts
    if (thisptr->demo_version.game == steampipe) {
      ptr->unk = dg_bitstream_read_uint(stream, 16);
    }
    // Protocol 4 version uses floats?
    else if (thisptr->demo_version.demo_protocol == 4) {
      ptr->unk = dg_bitstream_read_float(stream);
    }
    // Not available on other versions
  }
  SEND_MESSAGE();
}

static void write_svc_voice_init(dg_bitwriter *writer, dg_demver_data *version,
                                 packet_net_message *message) {
  struct dg_svc_voice_init *ptr = &message->message_svc_voice_init;
  dg_bitwriter_write_cstring(writer, ptr->codec);
  dg_bitwriter_write_uint(writer, ptr->quality, 8);

  if (ptr->quality == 255) {
    if (version->game == steampipe) {
      dg_bitwriter_write_uint(writer, ptr->unk, 16);
    } else if (version->demo_protocol == 4) {
      dg_bitwriter_write_float(writer, ptr->unk);
    }
  }
}

static void handle_svc_voice_data(dg_parser *thisptr, dg_bitstream *stream,
                                  packet_net_message *message, blk *scrap) {
  struct dg_svc_voice_data *ptr = &message->message_svc_voice_data;
  ptr->client = dg_bitstream_read_uint(stream, 8);
  ptr->proximity = dg_bitstream_read_uint(stream, 8);
  ptr->length = dg_bitstream_read_uint(stream, 16);
  ptr->data = dg_bitstream_fork_and_advance(stream, ptr->length);
  SEND_MESSAGE();
}

static void write_svc_voice_data(dg_bitwriter *writer, dg_demver_data *version,
                                 packet_net_message *message) {
  writer->error = true;
  writer->error_message = "Writing not implemented for svc_voice_data";
}

static void handle_svc_sounds(dg_parser *thisptr, dg_bitstream *stream, packet_net_message *message,
                              blk *scrap) {
  struct dg_svc_sounds *ptr = &message->message_svc_sounds;
  ptr->reliable_sound = dg_bitstream_read_bit(stream);
  if (ptr->reliable_sound) {
    ptr->sounds = 1;
    ptr->length = dg_bitstream_read_uint(stream, 8);
  } else {
    ptr->sounds = dg_bitstream_read_uint(stream, 8);
    ptr->length = dg_bitstream_read_uint(stream, 16);
  }
  ptr->data = dg_bitstream_fork_and_advance(stream, ptr->length);
  SEND_MESSAGE();
}

static void write_svc_sounds(dg_bitwriter *writer, dg_demver_data *version,
                             packet_net_message *message) {
  struct dg_svc_sounds *ptr = &message->message_svc_sounds;
  dg_bitwriter_write_bit(writer, ptr->reliable_sound);
  if (ptr->reliable_sound) {
    dg_bitwriter_write_uint(writer, ptr->length, 8);
  } else {
    dg_bitwriter_write_uint(writer, ptr->sounds, 8);
    dg_bitwriter_write_uint(writer, ptr->length, 16);
  }

  dg_bitwriter_write_bitstream(writer, &ptr->data);
}

static void handle_svc_setview(dg_parser *thisptr, dg_bitstream *stream, packet_net_message *message,
                               blk *scrap) {
  struct dg_svc_setview *ptr = &message->message_svc_setview;
  ptr->entity_index = dg_bitstream_read_uint(stream, 11);
  SEND_MESSAGE();
}

static void write_svc_setview(dg_bitwriter *writer, dg_demver_data *version,
                              packet_net_message *message) {
  struct dg_svc_setview *ptr = &message->message_svc_setview;
  dg_bitwriter_write_uint(writer, ptr->entity_index, 11);
}

static void handle_svc_fixangle(dg_parser *thisptr, dg_bitstream *stream, packet_net_message *message,
                                blk *scrap) {
  struct dg_svc_fixangle *ptr = &message->message_svc_fixangle;
  ptr->relative = dg_bitstream_read_bit(stream);
  ptr->angle = dg_bitstream_read_bitvector(stream, 16);
  SEND_MESSAGE();
}

static void write_svc_fixangle(dg_bitwriter *writer, dg_demver_data *version,
                               packet_net_message *message) {
  struct dg_svc_fixangle *ptr = &message->message_svc_fixangle;
  dg_bitwriter_write_bit(writer, ptr->relative);
  dg_bitwriter_write_bitvector(writer, ptr->angle);
}

static void handle_svc_crosshair_angle(dg_parser *thisptr, dg_bitstream *stream,
                                       packet_net_message *message, blk *scrap) {
  struct dg_svc_crosshair_angle *ptr = &message->message_svc_crosshair_angle;
  ptr->angle = dg_bitstream_read_bitvector(stream, 16);
  SEND_MESSAGE();
}

static void write_svc_crosshair_angle(dg_bitwriter *writer, dg_demver_data *version,
                                      packet_net_message *message) {
  struct dg_svc_crosshair_angle *ptr = &message->message_svc_crosshair_angle;
  dg_bitwriter_write_bitvector(writer, ptr->angle);
}

static void handle_svc_bsp_decal(dg_parser *thisptr, dg_bitstream *stream, packet_net_message *message,
                                 blk *scrap) {
  struct dg_svc_bsp_decal *ptr = &message->message_svc_bsp_decal;
  ptr->pos = dg_bitstream_read_coordvector(stream);
  ptr->decal_texture_index = dg_bitstream_read_uint(stream, 9);
  ptr->index_bool = dg_bitstream_read_bit(stream);

  if (ptr->index_bool) {
    ptr->entity_index = dg_bitstream_read_uint(stream, 11);
    ptr->model_index = dg_bitstream_read_uint(stream, thisptr->demo_version.model_index_bits);
  }
  ptr->lowpriority = dg_bitstream_read_bit(stream);
  SEND_MESSAGE();
}

static void write_svc_bsp_decal(dg_bitwriter *writer, dg_demver_data *version,
                                packet_net_message *message) {
  struct dg_svc_bsp_decal *ptr = &message->message_svc_bsp_decal;
  dg_bitwriter_write_coordvector(writer, ptr->pos);
  dg_bitwriter_write_uint(writer, ptr->decal_texture_index, 9);
  dg_bitwriter_write_bit(writer, ptr->index_bool);

  if (ptr->index_bool) {
    dg_bitwriter_write_uint(writer, ptr->entity_index, 11);
    dg_bitwriter_write_uint(writer, ptr->model_index, version->model_index_bits);
  }

  dg_bitwriter_write_bit(writer, ptr->lowpriority);
}

static void handle_svc_user_message(dg_parser *thisptr, dg_bitstream *stream,
                                    packet_net_message *message, blk *scrap) {
  struct dg_svc_user_message *ptr = &message->message_svc_user_message;
  ptr->msg_type = dg_bitstream_read_uint(stream, 8);
  ptr->length = dg_bitstream_read_uint(stream, thisptr->demo_version.svc_user_message_bits);
  ptr->data = dg_bitstream_fork_and_advance(stream, ptr->length);
  SEND_MESSAGE();
}

static void write_svc_user_message(dg_bitwriter *writer, dg_demver_data *version,
                                   packet_net_message *message) {
  struct dg_svc_user_message *ptr = &message->message_svc_user_message;
  dg_bitwriter_write_uint(writer, ptr->msg_type, 8);
  dg_bitwriter_write_uint(writer, ptr->length, version->svc_user_message_bits);
  dg_bitwriter_write_bitstream(writer, &ptr->data);
}

static void handle_svc_entity_message(dg_parser *thisptr, dg_bitstream *stream,
                                      packet_net_message *message, blk *scrap) {
  struct dg_svc_entity_message *ptr = &message->message_svc_entity_message;
  ptr->entity_index = dg_bitstream_read_uint(stream, 11);
  ptr->class_id = dg_bitstream_read_uint(stream, 9);
  ptr->length = dg_bitstream_read_uint(stream, 11);
  ptr->data = dg_bitstream_fork_and_advance(stream, ptr->length);
  SEND_MESSAGE();
}

static void write_svc_entity_message(dg_bitwriter *writer, dg_demver_data *version,
                                     packet_net_message *message) {
  struct dg_svc_entity_message *ptr = &message->message_svc_entity_message;
  dg_bitwriter_write_uint(writer, ptr->entity_index, 11);
  dg_bitwriter_write_uint(writer, ptr->class_id, 9);
  dg_bitwriter_write_uint(writer, ptr->length, 11);
  dg_bitwriter_write_bitstream(writer, &ptr->data);
}

static void handle_svc_game_event(dg_parser *thisptr, dg_bitstream *stream,
                                  packet_net_message *message, blk *scrap) {
  struct dg_svc_game_event *ptr = &message->message_svc_game_event;
  ptr->length = dg_bitstream_read_uint(stream, 11);
  ptr->data = dg_bitstream_fork_and_advance(stream, ptr->length);

  SEND_MESSAGE();
}

static void write_svc_game_event(dg_bitwriter *writer, dg_demver_data *version,
                                 packet_net_message *message) {
  struct dg_svc_game_event *ptr = &message->message_svc_game_event;
  dg_bitwriter_write_uint(writer, ptr->length, 11);
  dg_bitwriter_write_bitstream(writer, &ptr->data);
}

static void handle_svc_packet_entities(dg_parser *thisptr, dg_bitstream *stream,
                                       packet_net_message *message, blk *scrap) {
  struct dg_svc_packet_entities *ptr = &message->message_svc_packet_entities;
  ptr->max_entries = dg_bitstream_read_uint(stream, 11);
  ptr->is_delta = dg_bitstream_read_bit(stream);

  if (ptr->is_delta) {
    ptr->delta_from = dg_bitstream_read_sint32(stream);
  } else {
    ptr->delta_from = -1;
  }

  ptr->base_line = dg_bitstream_read_bit(stream);
  ptr->updated_entries = dg_bitstream_read_uint(stream, 11);
  uint32_t data_length = dg_bitstream_read_uint(stream, 20);
  ptr->update_baseline = dg_bitstream_read_bit(stream);
  ptr->data = dg_bitstream_fork_and_advance(stream, data_length);

  if (thisptr->m_settings.parse_packetentities) {
    dg_parser_handle_packetentities(thisptr, ptr);
  }
  SEND_MESSAGE();
}

static void write_svc_packet_entities(dg_bitwriter *writer, dg_demver_data *version,
                                      packet_net_message *message) {
  struct dg_svc_packet_entities *ptr = &message->message_svc_packet_entities;
  dg_bitwriter_write_uint(writer, ptr->max_entries, 11);
  dg_bitwriter_write_bit(writer, ptr->is_delta);
  if (ptr->is_delta) {
    dg_bitwriter_write_sint32(writer, ptr->delta_from);
  }

  dg_bitwriter_write_bit(writer, ptr->base_line);
  dg_bitwriter_write_uint(writer, ptr->updated_entries, 11);
  uint32_t bits = dg_bitstream_bits_left(&ptr->data);
  dg_bitwriter_write_uint(writer, bits, 20);
  dg_bitwriter_write_bit(writer, ptr->update_baseline);
  dg_bitwriter_write_bitstream(writer, &ptr->data);
}

static void handle_svc_temp_entities(dg_parser *thisptr, dg_bitstream *stream,
                                     packet_net_message *message, blk *scrap) {
  struct dg_svc_temp_entities *ptr = &message->message_svc_temp_entities;
  ptr->num_entries = dg_bitstream_read_uint(stream, 8);
  uint32_t data_length;

  if (thisptr->demo_version.game == steampipe) {
    data_length = dg_bitstream_read_varuint32(stream);
  } else if (thisptr->demo_version.game == l4d2) {
    data_length = dg_bitstream_read_uint(stream, 18);
  } else {
    data_length = dg_bitstream_read_uint(stream, 17);
  }
  ptr->data = dg_bitstream_fork_and_advance(stream, data_length);

  SEND_MESSAGE();
}

static void write_svc_temp_entities(dg_bitwriter *writer, dg_demver_data *version,
                                    packet_net_message *message) {
  struct dg_svc_temp_entities *ptr = &message->message_svc_temp_entities;
  dg_bitwriter_write_uint(writer, ptr->num_entries, 8);
  uint32_t data_length = dg_bitstream_bits_left(&ptr->data);

  if (version->game == steampipe) {
    dg_bitwriter_write_varuint32(writer, data_length);
  } else if (version->game == l4d2) {
    dg_bitwriter_write_uint(writer, data_length, 18);
  } else {
    dg_bitwriter_write_uint(writer, data_length, 17);
  }
  dg_bitwriter_write_bitstream(writer, &ptr->data);
}

static void handle_svc_prefetch(dg_parser *thisptr, dg_bitstream *stream, packet_net_message *message,
                                blk *scrap) {
  struct dg_svc_prefetch *ptr = &message->message_svc_prefetch;
  ptr->sound_index = dg_bitstream_read_uint(stream, thisptr->demo_version.svc_prefetch_bits);
  SEND_MESSAGE();
}

static void write_svc_prefetch(dg_bitwriter *writer, dg_demver_data *version,
                               packet_net_message *message) {
  struct dg_svc_prefetch *ptr = &message->message_svc_prefetch;
  dg_bitwriter_write_uint(writer, ptr->sound_index, version->svc_prefetch_bits);
}

static void handle_svc_menu(dg_parser *thisptr, dg_bitstream *stream, packet_net_message *message,
                            blk *scrap) {
  struct dg_svc_menu *ptr = &message->message_svc_menu;
  ptr->menu_type = dg_bitstream_read_uint(stream, 16);
  uint32_t data_length = dg_bitstream_read_uint32(stream);
  ptr->data = dg_bitstream_fork_and_advance(stream, data_length);
  SEND_MESSAGE();
}

static void write_svc_menu(dg_bitwriter *writer, dg_demver_data *version,
                           packet_net_message *message) {
  writer->error = true;
  writer->error_message = "Writing not implemented for svc_menu";
}

static void handle_svc_game_event_list(dg_parser *thisptr, dg_bitstream *stream,
                                       packet_net_message *message, blk *scrap) {
  struct dg_svc_game_event_list *ptr = &message->message_svc_game_event_list;
  ptr->events = dg_bitstream_read_uint(stream, 9);
  ptr->length = dg_bitstream_read_uint(stream, 20);
  ptr->data = dg_bitstream_fork_and_advance(stream, ptr->length);
  SEND_MESSAGE();
}

static void write_svc_game_event_list(dg_bitwriter *writer, dg_demver_data *version,
                                      packet_net_message *message) {
  struct dg_svc_game_event_list *ptr = &message->message_svc_game_event_list;
  dg_bitwriter_write_uint(writer, ptr->events, 9);
  dg_bitwriter_write_uint(writer, ptr->length, 20);
  dg_bitwriter_write_bitstream(writer, &ptr->data);
}

static void handle_svc_get_cvar_value(dg_parser *thisptr, dg_bitstream *stream,
                                      packet_net_message *message, blk *scrap) {
  struct dg_svc_get_cvar_value *ptr = &message->message_svc_get_cvar_value;
  ptr->cookie = dg_bitstream_read_sint32(stream);
  COPY_STRING(ptr->cvar_name);
  SEND_MESSAGE();
}

static void write_svc_get_cvar_value(dg_bitwriter *writer, dg_demver_data *version,
                                     packet_net_message *message) {
  struct dg_svc_get_cvar_value *ptr = &message->message_svc_get_cvar_value;
  dg_bitwriter_write_sint32(writer, ptr->cookie);
  dg_bitwriter_write_cstring(writer, ptr->cvar_name);
}

static void handle_net_splitscreen_user(dg_parser *thisptr, dg_bitstream *stream,
                                        packet_net_message *message, blk *scrap) {
  message->message_net_splitscreen_user.unk = dg_bitstream_read_bit(stream);
  SEND_MESSAGE();
}

static void write_net_splitscreen_user(dg_bitwriter *writer, dg_demver_data *version,
                                       packet_net_message *message) {
  writer->error = true;
  writer->error_message = "Writing not implemented for net_splitscreen_user";
}

static void handle_svc_splitscreen(dg_parser *thisptr, dg_bitstream *stream,
                                   packet_net_message *message, blk *scrap) {
  struct dg_svc_splitscreen *ptr = &message->message_svc_splitscreen;
  ptr->remove_user = dg_bitstream_read_bit(stream);
  uint32_t data_length = dg_bitstream_read_uint(stream, 11);
  ptr->data = dg_bitstream_fork_and_advance(stream, data_length);
  SEND_MESSAGE();
}

static void write_svc_splitscreen(dg_bitwriter *writer, dg_demver_data *version,
                                  packet_net_message *message) {
  writer->error = true;
  writer->error_message = "Writing not implemented for svc_splitscreen";
}

static void handle_svc_paintmap_data(dg_parser *thisptr, dg_bitstream *stream,
                                     packet_net_message *message, blk *scrap) {
  struct dg_svc_paintmap_data *ptr = &message->message_svc_paintmap_data;
  uint32_t data_length = dg_bitstream_read_uint32(stream);
  ptr->data = dg_bitstream_fork_and_advance(stream, data_length);
  SEND_MESSAGE();
}

static void write_svc_paintmap_data(dg_bitwriter *writer, dg_demver_data *version,
                                    packet_net_message *message) {
  struct dg_svc_paintmap_data *ptr = &message->message_svc_paintmap_data;
  uint32_t data_length = dg_bitstream_bits_left(&ptr->data);
  dg_bitwriter_write_uint32(writer, data_length);
  dg_bitwriter_write_bitstream(writer, &ptr->data);
}

static void handle_svc_cmd_key_values(dg_parser *thisptr, dg_bitstream *stream,
                                      packet_net_message *message, blk *scrap) {
  struct dg_svc_cmd_key_values *ptr = &message->message_svc_cmd_key_values;
  uint32_t data_length = dg_bitstream_read_uint32(stream);
  ptr->data = dg_bitstream_fork_and_advance(stream, data_length * 8);
  SEND_MESSAGE();
}

static void write_svc_cmd_key_values(dg_bitwriter *writer, dg_demver_data *version,
                                     packet_net_message *message) {
  struct dg_svc_cmd_key_values *ptr = &message->message_svc_cmd_key_values;
  uint32_t data_length = dg_bitstream_bits_left(&ptr->data) / 8;
  dg_bitwriter_write_uint32(writer, data_length);
  dg_bitwriter_write_bitstream(writer, &ptr->data);
}

static net_message_type version_get_message_type(dg_parser *thisptr, unsigned int value) {
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

void dg_bitwriter_write_netmessage(dg_bitwriter *writer, dg_demver_data *version,
                                   packet_net_message *message) {
  int type_out = -1;
  // Don't use the type index directly, we want to support writing to different protocols than the
  // demo was read
  for (int i = 0; i < version->netmessage_count; ++i) {
    if (version->netmessage_array[i] == message->mtype) {
      type_out = i;
      break;
    }
  }

#define DECLARE_SWITCH_STATEMENT(message_type)                                                     \
  case message_type:                                                                               \
    write_##message_type(writer, version, message);                                                \
    break;

  // Only write if exists on this protocol
  if (type_out != -1) {
    dg_bitwriter_write_uint(writer, type_out, version->netmessage_type_bits);

    switch (message->mtype) {
      DEMOGOBBLER_MACRO_ALL_MESSAGES(DECLARE_SWITCH_STATEMENT);
    default:
      writer->error = true;
      writer->error_message = "No handler for this type of message.";
      break;
    }
  }
#undef DECLARE_SWITCH_STATEMENT
}

void parse_netmessages(dg_parser *thisptr, dg_packet *packet) {
#ifdef DEBUG
#define MAX_HISTORY 256
  net_message_type type_history[MAX_HISTORY];
  size_t history_index = 0;
#endif

  void *data = packet->data;
  size_t size = packet->size_bytes;
  dg_bitstream stream = dg_bitstream_create(data, size * 8);
  // fprintf(stderr, "packet start:\n");

  // We allocate a single scrap buffer for the duration of parsing the packet that is as large as
  // the whole packet. Should never run out of space as long as we don't make things larger as we
  // read it out
  dg_alloc_state* arena = dg_parser_temp_allocator(thisptr);
  blk scrap_blk;
  scrap_blk.address = dg_alloc_allocate(arena, size, 1);
  scrap_blk.size = size;
  unsigned int bits = thisptr->demo_version.netmessage_type_bits;

  packet_net_message initial_array[64];
  dg_vector_array packet_arr = dg_va_create(initial_array, packet_net_message);

  while (dg_bitstream_bits_left(&stream) > bits && !thisptr->error && !stream.overflow) {
    if (scrap_blk.address == NULL) {
      thisptr->error = true;
      thisptr->error_message = "Unable to allocate scrap block";
      break;
    }

    unsigned int type_index = dg_bitstream_read_uint(&stream, bits);
    net_message_type type = version_get_message_type(thisptr, type_index);

#ifdef DEBUG
    if (history_index < MAX_HISTORY) {
      type_history[history_index] = type;
      ++history_index;
    }
#endif

    // fprintf(stderr, "%d : %d\n", type_index, type);
    packet_net_message *message = dg_va_push_back_empty(&packet_arr);
    memset(message, 0, sizeof(packet_net_message));
    message->mtype = type;

#define DECLARE_SWITCH_STATEMENT(message_type)                                                     \
  case message_type:                                                                               \
    handle_##message_type(thisptr, &stream, message, &scrap_blk);                                  \
    break;

    switch (type) {
      DEMOGOBBLER_MACRO_ALL_MESSAGES(DECLARE_SWITCH_STATEMENT);
    default:
      thisptr->error = true;
      thisptr->error_message = "No handler for this type of message.";
      break;
    }
  }

#undef DECLARE_SWITCH_STATEMENT

  if (stream.overflow && !thisptr->error) {
    thisptr->error = true;
    thisptr->error_message = "Bitstream overflowed during packet parsing";
  }

  if (!thisptr->error) {
    packet_parsed parsed;
    memset(&parsed, 0, sizeof(parsed));
    parsed.messages = dg_alloc_allocate(arena, packet_arr.count_elements * sizeof(packet_net_message), alignof(packet_net_message));
    memcpy(parsed.messages, packet_arr.ptr, packet_arr.count_elements * sizeof(packet_net_message));

    parsed.message_count = packet_arr.count_elements;
    parsed.orig = *packet;
    parsed.leftover_bits = stream;

    if (thisptr->m_settings.packet_parsed_handler) {
      thisptr->m_settings.packet_parsed_handler(&thisptr->state, &parsed);
    }
  }

  // Ignore errors on negative tick packets, these are known to be bad
  if (packet->preamble.tick < 0 && packet->preamble.converted_type == dg_type_packet &&
      thisptr->error) {
    // printf("%s\n", thisptr->error_message);
    thisptr->error = false;
  }

  // fprintf(stderr, "packet end:\n");
  dg_va_free(&packet_arr);

#ifdef DEBUG
  if (thisptr->error) {
    fprintf(stderr, "netmessages before error: ");
    for (size_t i = 0; i < history_index; ++i) {
      fprintf(stderr, "%d ", type_history[i]);
    }
    fprintf(stderr, "\n");
  }
#endif
}
