#include "parser_netmessages.h"
#include "arena.h"
#include "bitstream.h"
#include "demogobbler_bitwriter.h"
#include "parser_packetentities.h"
#include "utils.h"
#include "version_utils.h"
#include "vector_array.h"
#include <string.h>

#define TOSS_STRING() bitstream_read_cstring(stream, scrap, 260);
#define COPY_STRING(variable)                                                                      \
  {                                                                                                \
    char *temp_string = scrap->address;                                                            \
    size_t bytes = bitstream_read_cstring(stream, scrap->address, scrap->size);                    \
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
  void* address;
  size_t size;
} blk;

static void handle_net_nop(parser *thisptr, bitstream *stream, packet_net_message *message,
                           blk* scrap) {
  SEND_MESSAGE();
}

static void write_net_nop(bitwriter *writer, demo_version_data *version,
                          packet_net_message *message) {}

static void handle_net_disconnect(parser *thisptr, bitstream *stream, packet_net_message *message,
                                  blk* scrap) {
  struct demogobbler_net_disconnect *ptr = &message->message_net_disconnect;
  COPY_STRING(ptr->text);
  SEND_MESSAGE();
}

static void write_net_disconnect(bitwriter *writer, demo_version_data *version,
                                 packet_net_message *message) {
  struct demogobbler_net_disconnect *ptr = &message->message_net_disconnect;
  bitwriter_write_cstring(writer, ptr->text);
}

static void handle_net_file(parser *thisptr, bitstream *stream, packet_net_message *message,
                            blk* scrap) {
  struct demogobbler_net_file *ptr = &message->message_net_file;

  ptr->transfer_id = bitstream_read_uint32(stream);
  COPY_STRING(ptr->filename);
  ptr->file_requested = bitstream_read_uint(stream, thisptr->demo_version.net_file_bits);
  SEND_MESSAGE();
}

static void write_net_file(bitwriter *writer, demo_version_data *version,
                           packet_net_message *message) {
  struct demogobbler_net_file *ptr = &message->message_net_file;
  bitwriter_write_uint32(writer, ptr->transfer_id);
  bitwriter_write_cstring(writer, ptr->filename);
  bitwriter_write_uint(writer, ptr->file_requested, version->net_file_bits);
}

static void handle_net_tick(parser *thisptr, bitstream *stream, packet_net_message *message,
                            blk* scrap) {
  struct demogobbler_net_tick *ptr = &message->message_net_tick;
  ptr->tick = bitstream_read_uint32(stream);
  if (thisptr->demo_version.has_nettick_times) {
    ptr->host_frame_time = bitstream_read_uint(stream, 16);
    ptr->host_frame_time_std_dev = bitstream_read_uint(stream, 16);
  }
  SEND_MESSAGE();
}

static void write_net_tick(bitwriter *writer, demo_version_data *version,
                           packet_net_message *message) {
  struct demogobbler_net_tick *ptr = &message->message_net_tick;
  bitwriter_write_uint32(writer, ptr->tick);

  if (version->has_nettick_times) {
    bitwriter_write_uint(writer, ptr->host_frame_time, 16);
    bitwriter_write_uint(writer, ptr->host_frame_time_std_dev, 16);
  }
}

static void handle_net_stringcmd(parser *thisptr, bitstream *stream, packet_net_message *message,
                                 blk* scrap) {
  struct demogobbler_net_stringcmd *ptr = &message->message_net_stringcmd;
  COPY_STRING(ptr->command);
  SEND_MESSAGE();
}

static void write_net_stringcmd(bitwriter *writer, demo_version_data *version,
                                packet_net_message *message) {
  struct demogobbler_net_stringcmd *ptr = &message->message_net_stringcmd;
  bitwriter_write_cstring(writer, ptr->command);
}

static void handle_net_setconvar(parser *thisptr, bitstream *stream, packet_net_message *message,
                                 blk* scrap) {
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

static void write_net_setconvar(bitwriter *writer, demo_version_data *version,
                                packet_net_message *message) {
  struct demogobbler_net_setconvar *ptr = &message->message_net_setconvar;
  bitwriter_write_uint(writer, ptr->count, 8);

  for (size_t i = 0; i < ptr->count; ++i) {
    bitwriter_write_cstring(writer, ptr->convars[i].name);
    bitwriter_write_cstring(writer, ptr->convars[i].value);
  }
}

static void handle_net_signonstate(parser *thisptr, bitstream *stream, packet_net_message *message,
                                   blk* scrap) {
  struct demogobbler_net_signonstate *ptr = &message->message_net_signonstate;
  ptr->signon_state = bitstream_read_uint(stream, 8);
  ptr->spawn_count = bitstream_read_sint32(stream);

  if (thisptr->demo_version.demo_protocol >= 4) {
    ptr->NE_num_server_players = bitstream_read_uint32(stream);
    unsigned int length = bitstream_read_uint32(stream) * 8;
    ptr->NE_player_network_ids = bitstream_fork_and_advance(stream, length);
    ptr->NE_map_name_length = bitstream_read_uint32(stream);
    ptr->NE_map_name = scrap->address;

    if (scrap->size < ptr->NE_map_name_length) {
      thisptr->error = true;
      thisptr->error_message = "Map name in net_signonstate has bad length";
    }
    else {
      bitstream_read_fixed_string(stream, scrap->address, ptr->NE_map_name_length);
    }
  }
  // Uncrafted: GameState.ClientSoundSequence = 1; reset sound sequence number after receiving
  // SignOn sounds
  SEND_MESSAGE();
}

static void write_net_signonstate(bitwriter *writer, demo_version_data *version,
                                  packet_net_message *message) {
  struct demogobbler_net_signonstate *ptr = &message->message_net_signonstate;
  bitwriter_write_uint(writer, ptr->signon_state, 8);
  bitwriter_write_uint32(writer, ptr->spawn_count);

  if (version->demo_protocol >= 4) {
    bitwriter_write_uint32(writer, ptr->NE_num_server_players);
    uint32_t length =
        (ptr->NE_player_network_ids.bitsize - ptr->NE_player_network_ids.bitoffset) / 8;
    bitwriter_write_uint32(writer, length);
    bitwriter_write_bitstream(writer, &ptr->NE_player_network_ids);
    bitwriter_write_uint32(writer, ptr->NE_map_name_length);
    bitwriter_write_bits(writer, ptr->NE_map_name, ptr->NE_map_name_length * 8);
  }
}

static void handle_svc_print(parser *thisptr, bitstream *stream, packet_net_message *message,
                             blk* scrap) {
  struct demogobbler_svc_print *ptr = &message->message_svc_print;

  COPY_STRING(ptr->message);

  if (thisptr->demo_version.game == l4d2 && thisptr->demo_version.l4d2_version == 2042) {
    int l4d2_build = 0;
    get_l4d2_build(ptr->message, &l4d2_build);

    if (l4d2_build == 4710) {
      parser_update_l4d2_version(thisptr, 2091);
    } else if (l4d2_build == 6403) {
      parser_update_l4d2_version(thisptr, 2147);
    }
  }

  SEND_MESSAGE();
}

static void write_svc_print(bitwriter *writer, demo_version_data *version,
                            packet_net_message *message) {
  struct demogobbler_svc_print *ptr = &message->message_svc_print;
  bitwriter_write_cstring(writer, ptr->message);
}

static void handle_svc_serverinfo(parser *thisptr, bitstream *stream, packet_net_message *message,
                                  blk* scrap) {
  struct demogobbler_svc_serverinfo *ptr = &message->message_svc_serverinfo;
  ptr->network_protocol = bitstream_read_uint(stream, 16);
  ptr->server_count = bitstream_read_uint32(stream);
  ptr->is_hltv = bitstream_read_bit(stream);
  ptr->is_dedicated = bitstream_read_bit(stream);

  if (thisptr->demo_version.game == l4d2 && thisptr->demo_version.l4d2_version >= 2147)
    ptr->unk_l4d_bit = bitstream_read_bit(stream);
  else
    ptr->unk_l4d_bit = 0;

  ptr->client_crc = bitstream_read_sint32(stream);

  if (thisptr->demo_version.demo_protocol >= 4)
    ptr->stringtable_crc = bitstream_read_uint32(stream);
  else
    ptr->stringtable_crc = 0;

  ptr->max_classes = bitstream_read_uint(stream, 16);

  if (thisptr->demo_version.game == steampipe) {
    bitstream_read_fixed_string(stream, ptr->map_md5, 16);
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

  if (thisptr->demo_version.game == l4d2 && thisptr->demo_version.l4d2_version >= 2147) {
    COPY_STRING(ptr->mission_name);
    COPY_STRING(ptr->mutation_name);
  }

  if (thisptr->demo_version.game == steampipe)
    ptr->has_replay = bitstream_read_bit(stream);

  SEND_MESSAGE();
}

static void write_svc_serverinfo(bitwriter *writer, demo_version_data *version,
                                 packet_net_message *message) {
  struct demogobbler_svc_serverinfo *ptr = &message->message_svc_serverinfo;

  bitwriter_write_uint(writer, ptr->network_protocol, 16);
  bitwriter_write_uint32(writer, ptr->server_count);
  bitwriter_write_bit(writer, ptr->is_hltv);
  bitwriter_write_bit(writer, ptr->is_dedicated);

  if (version->game == l4d2 && version->l4d2_version >= 2147)
    bitwriter_write_bit(writer, ptr->unk_l4d_bit);

  bitwriter_write_sint32(writer, ptr->client_crc);

  if (version->demo_protocol >= 4)
    bitwriter_write_uint32(writer, ptr->stringtable_crc);

  bitwriter_write_uint(writer, ptr->max_classes, 16);

  if (version->game == steampipe) {
    bitwriter_write_bits(writer, ptr->map_md5, 16 * 8);
  } else {
    bitwriter_write_uint32(writer, ptr->map_crc);
  }

  bitwriter_write_uint(writer, ptr->player_count, 8);
  bitwriter_write_uint(writer, ptr->max_clients, 8);
  bitwriter_write_float(writer, ptr->tick_interval);
  bitwriter_write_uint(writer, ptr->platform, 8);

  bitwriter_write_cstring(writer, ptr->game_dir);
  bitwriter_write_cstring(writer, ptr->map_name);
  bitwriter_write_cstring(writer, ptr->sky_name);
  bitwriter_write_cstring(writer, ptr->host_name);

  if (version->game == l4d2 && version->l4d2_version >= 2147) {
    bitwriter_write_cstring(writer, ptr->mission_name);
    bitwriter_write_cstring(writer, ptr->mutation_name);
  }

  if (version->game == steampipe)
    bitwriter_write_bit(writer, ptr->has_replay);
}

static void handle_svc_sendtable(parser *thisptr, bitstream *stream, packet_net_message *message,
                                 blk* scrap) {
  struct demogobbler_svc_sendtable *ptr = &message->message_svc_sendtable;
  ptr->needs_decoder = bitstream_read_bit(stream);
  ptr->length = bitstream_read_uint(stream, 16);
  ptr->data = bitstream_fork_and_advance(stream, ptr->length);
  SEND_MESSAGE();
}

static void write_svc_sendtable(bitwriter *writer, demo_version_data *version,
                                packet_net_message *message) {
  writer->error = true;
  writer->error_message = "Writing not implemented for svc_sendtable";
}

static void handle_svc_classinfo(parser *thisptr, bitstream *stream, packet_net_message *message,
                                 blk* scrap) {
  struct demogobbler_svc_classinfo *ptr = &message->message_svc_classinfo;
  ptr->length = bitstream_read_uint(stream, 16);
  ptr->create_on_client = bitstream_read_bit(stream);
  unsigned int bits = highest_bit_index(ptr->length) + 1;

  // Could in theory have 32768 classes so use the allocator for this
  if (!ptr->create_on_client) {
    ptr->server_classes = demogobbler_arena_allocate(&thisptr->temp_arena, ptr->length * sizeof(struct demogobbler_svc_classinfo_serverclass), 1);
    for (unsigned int i = 0; i < ptr->length && !stream->overflow; ++i) {
      ptr->server_classes[i].class_id = bitstream_read_uint(stream, bits);
      COPY_STRING(ptr->server_classes[i].class_name);
      COPY_STRING(ptr->server_classes[i].datatable_name);
    }
  } else {
    ptr->server_classes = NULL;
  }
  SEND_MESSAGE();
}

static void write_svc_classinfo(bitwriter *writer, demo_version_data *version,
                                packet_net_message *message) {
  struct demogobbler_svc_classinfo *ptr = &message->message_svc_classinfo;
  bitwriter_write_uint(writer, ptr->length, 16);
  bitwriter_write_bit(writer, ptr->create_on_client);
  unsigned int bits = highest_bit_index(ptr->length) + 1;

  if (!ptr->create_on_client) {
    for (unsigned int i = 0; i < ptr->length; ++i) {
      bitwriter_write_uint(writer, ptr->server_classes[i].class_id, bits);
      bitwriter_write_cstring(writer, ptr->server_classes[i].class_name);
      bitwriter_write_cstring(writer, ptr->server_classes[i].datatable_name);
    }
  }
}

static void handle_svc_setpause(parser *thisptr, bitstream *stream, packet_net_message *message,
                                blk* scrap) {
  message->message_svc_setpause.paused = bitstream_read_bit(stream);
  SEND_MESSAGE();
}

static void write_svc_setpause(bitwriter *writer, demo_version_data *version,
                               packet_net_message *message) {
  struct demogobbler_svc_setpause *ptr = &message->message_svc_setpause;
  bitwriter_write_bit(writer, ptr->paused);
}

static void handle_svc_create_stringtable(parser *thisptr, bitstream *stream,
                                          packet_net_message *message, blk* scrap) {
  struct demogobbler_svc_create_stringtable *ptr = &message->message_svc_create_stringtable;
  COPY_STRING(ptr->name);
  ptr->max_entries = bitstream_read_uint(stream, 16);
  unsigned int num_entries_bits = highest_bit_index(ptr->max_entries) + 1;
  ptr->num_entries = bitstream_read_uint(stream, num_entries_bits);

  if (thisptr->demo_version.game == steampipe) {
    ptr->data_length = bitstream_read_varuint32(stream);
  } else {
    ptr->data_length =
        bitstream_read_uint(stream, thisptr->demo_version.stringtable_userdata_size_bits);
  }

  ptr->user_data_fixed_size = bitstream_read_bit(stream);

  if (ptr->user_data_fixed_size) {
    ptr->user_data_size = bitstream_read_uint(stream, 12);
    ptr->user_data_size_bits = bitstream_read_uint(stream, 4);
  } else {
    ptr->user_data_size = 0;
    ptr->user_data_size_bits = 0;
  }

  if (thisptr->demo_version.network_protocol >= 15) {
    ptr->flags = bitstream_read_uint(stream, thisptr->demo_version.stringtable_flags_bits);
  } else {
    ptr->flags = 0;
  }

  ptr->data = bitstream_fork_and_advance(stream, ptr->data_length);

  SEND_MESSAGE();
}

static void write_svc_create_stringtable(bitwriter *writer, demo_version_data *version,
                                         packet_net_message *message) {
  struct demogobbler_svc_create_stringtable *ptr = &message->message_svc_create_stringtable;
  bitwriter_write_cstring(writer, ptr->name);
  bitwriter_write_uint(writer, ptr->max_entries, 16);
  unsigned int num_entries_bits = highest_bit_index(ptr->max_entries) + 1;
  bitwriter_write_uint(writer, ptr->num_entries, num_entries_bits);

  if (version->game == steampipe) {
    bitwriter_write_varuint32(writer, ptr->data_length);
  } else {
    bitwriter_write_uint(writer, ptr->data_length, version->stringtable_userdata_size_bits);
  }

  bitwriter_write_bit(writer, ptr->user_data_fixed_size);

  if (ptr->user_data_fixed_size) {
    bitwriter_write_uint(writer, ptr->user_data_size, 12);
    bitwriter_write_uint(writer, ptr->user_data_size_bits, 4);
  }

  if (version->network_protocol >= 15) {
    bitwriter_write_uint(writer, ptr->flags, version->stringtable_flags_bits);
  }

  bitwriter_write_bitstream(writer, &ptr->data);
}

static void handle_svc_update_stringtable(parser *thisptr, bitstream *stream,
                                          packet_net_message *message, blk* scrap) {
  struct demogobbler_svc_update_stringtable *ptr = &message->message_svc_update_stringtable;
  ptr->table_id = bitstream_read_uint(stream, thisptr->demo_version.svc_update_stringtable_table_id_bits);
  ptr->exists = bitstream_read_bit(stream);
  if (ptr->exists) {
    ptr->changed_entries = bitstream_read_uint(stream, 16);
  } else {
    ptr->changed_entries = 1;
  }

  if (thisptr->demo_version.network_protocol <= 7) {
    ptr->data_length = bitstream_read_uint(stream, 16);
  } else {
    ptr->data_length = bitstream_read_uint(stream, 20);
  }
  ptr->data = bitstream_fork_and_advance(stream, ptr->data_length);
  SEND_MESSAGE();
}

static void write_svc_update_stringtable(bitwriter *writer, demo_version_data *version,
                                         packet_net_message *message) {
  struct demogobbler_svc_update_stringtable *ptr = &message->message_svc_update_stringtable;
  bitwriter_write_uint(writer, ptr->table_id, version->svc_update_stringtable_table_id_bits);
  bitwriter_write_bit(writer, ptr->exists);

  if (ptr->exists) {
    bitwriter_write_uint(writer, ptr->changed_entries, 16);
  }

  if (version->network_protocol <= 7) {
    bitwriter_write_uint(writer, ptr->data_length, 16);
  } else {
    bitwriter_write_uint(writer, ptr->data_length, 20);
  }

  bitwriter_write_bitstream(writer, &ptr->data);
}

static void handle_svc_voice_init(parser *thisptr, bitstream *stream, packet_net_message *message,
                                  blk* scrap) {
  struct demogobbler_svc_voice_init *ptr = &message->message_svc_voice_init;

  COPY_STRING(ptr->codec);
  ptr->quality = bitstream_read_uint(stream, 8);
  if (ptr->quality == 255) {
    // Steampipe version uses shorts
    if (thisptr->demo_version.game == steampipe) {
      ptr->unk = bitstream_read_uint(stream, 16);
    }
    // Protocol 4 version uses floats?
    else if (thisptr->demo_version.demo_protocol == 4) {
      ptr->unk = bitstream_read_float(stream);
    }
    // Not available on other versions
  }
  SEND_MESSAGE();
}

static void write_svc_voice_init(bitwriter *writer, demo_version_data *version,
                                 packet_net_message *message) {
  struct demogobbler_svc_voice_init *ptr = &message->message_svc_voice_init;
  bitwriter_write_cstring(writer, ptr->codec);
  bitwriter_write_uint(writer, ptr->quality, 8);

  if (ptr->quality == 255) {
    if (version->game == steampipe) {
      bitwriter_write_uint(writer, ptr->unk, 16);
    } else if (version->demo_protocol == 4) {
      bitwriter_write_float(writer, ptr->unk);
    }
  }
}

static void handle_svc_voice_data(parser *thisptr, bitstream *stream, packet_net_message *message,
                                  blk* scrap) {
  struct demogobbler_svc_voice_data *ptr = &message->message_svc_voice_data;
  ptr->client = bitstream_read_uint(stream, 8);
  ptr->proximity = bitstream_read_uint(stream, 8);
  ptr->length = bitstream_read_uint(stream, 16);
  ptr->data = bitstream_fork_and_advance(stream, ptr->length);
  SEND_MESSAGE();
}

static void write_svc_voice_data(bitwriter *writer, demo_version_data *version,
                                 packet_net_message *message) {
  writer->error = true;
  writer->error_message = "Writing not implemented for svc_voice_data";
}

static void handle_svc_sounds(parser *thisptr, bitstream *stream, packet_net_message *message,
                              blk* scrap) {
  struct demogobbler_svc_sounds *ptr = &message->message_svc_sounds;
  ptr->reliable_sound = bitstream_read_bit(stream);
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

static void write_svc_sounds(bitwriter *writer, demo_version_data *version,
                             packet_net_message *message) {
  struct demogobbler_svc_sounds *ptr = &message->message_svc_sounds;
  bitwriter_write_bit(writer, ptr->reliable_sound);
  if (ptr->reliable_sound) {
    bitwriter_write_uint(writer, ptr->length, 8);
  } else {
    bitwriter_write_uint(writer, ptr->sounds, 8);
    bitwriter_write_uint(writer, ptr->length, 16);
  }

  bitwriter_write_bitstream(writer, &ptr->data);
}

static void handle_svc_setview(parser *thisptr, bitstream *stream, packet_net_message *message,
                               blk* scrap) {
  struct demogobbler_svc_setview *ptr = &message->message_svc_setview;
  ptr->entity_index = bitstream_read_uint(stream, 11);
  SEND_MESSAGE();
}

static void write_svc_setview(bitwriter *writer, demo_version_data *version,
                              packet_net_message *message) {
  struct demogobbler_svc_setview *ptr = &message->message_svc_setview;
  bitwriter_write_uint(writer, ptr->entity_index, 11);
}

static void handle_svc_fixangle(parser *thisptr, bitstream *stream, packet_net_message *message,
                                blk* scrap) {
  struct demogobbler_svc_fixangle *ptr = &message->message_svc_fixangle;
  ptr->relative = bitstream_read_bit(stream);
  ptr->angle = bitstream_read_bitvector(stream, 16);
  SEND_MESSAGE();
}

static void write_svc_fixangle(bitwriter *writer, demo_version_data *version,
                               packet_net_message *message) {
  struct demogobbler_svc_fixangle *ptr = &message->message_svc_fixangle;
  bitwriter_write_bit(writer, ptr->relative);
  bitwriter_write_bitvector(writer, ptr->angle);
}

static void handle_svc_crosshair_angle(parser *thisptr, bitstream *stream,
                                       packet_net_message *message, blk* scrap) {
  struct demogobbler_svc_crosshair_angle *ptr = &message->message_svc_crosshair_angle;
  ptr->angle = bitstream_read_bitvector(stream, 16);
  SEND_MESSAGE();
}

static void write_svc_crosshair_angle(bitwriter *writer, demo_version_data *version,
                                      packet_net_message *message) {
  struct demogobbler_svc_crosshair_angle *ptr = &message->message_svc_crosshair_angle;
  bitwriter_write_bitvector(writer, ptr->angle);
}

static void handle_svc_bsp_decal(parser *thisptr, bitstream *stream, packet_net_message *message,
                                 blk* scrap) {
  struct demogobbler_svc_bsp_decal *ptr = &message->message_svc_bsp_decal;
  ptr->pos = bitstream_read_coordvector(stream);
  ptr->decal_texture_index = bitstream_read_uint(stream, 9);
  ptr->index_bool = bitstream_read_bit(stream);

  if (ptr->index_bool) {
    ptr->entity_index = bitstream_read_uint(stream, 11);
    ptr->model_index = bitstream_read_uint(stream, thisptr->demo_version.model_index_bits);
  }
  ptr->lowpriority = bitstream_read_bit(stream);
  SEND_MESSAGE();
}

static void write_svc_bsp_decal(bitwriter *writer, demo_version_data *version,
                                packet_net_message *message) {
  struct demogobbler_svc_bsp_decal *ptr = &message->message_svc_bsp_decal;
  bitwriter_write_coordvector(writer, ptr->pos);
  bitwriter_write_uint(writer, ptr->decal_texture_index, 9);
  bitwriter_write_bit(writer, ptr->index_bool);

  if (ptr->index_bool) {
    bitwriter_write_uint(writer, ptr->entity_index, 11);
    bitwriter_write_uint(writer, ptr->model_index, version->model_index_bits);
  }

  bitwriter_write_bit(writer, ptr->lowpriority);
}

static void handle_svc_user_message(parser *thisptr, bitstream *stream, packet_net_message *message,
                                    blk* scrap) {
  struct demogobbler_svc_user_message *ptr = &message->message_svc_user_message;
  ptr->msg_type = bitstream_read_uint(stream, 8);
  ptr->length = bitstream_read_uint(stream, thisptr->demo_version.svc_user_message_bits);
  ptr->data = bitstream_fork_and_advance(stream, ptr->length);
  SEND_MESSAGE();
}

static void write_svc_user_message(bitwriter *writer, demo_version_data *version,
                                   packet_net_message *message) {
  struct demogobbler_svc_user_message *ptr = &message->message_svc_user_message;
  bitwriter_write_uint(writer, ptr->msg_type, 8);
  bitwriter_write_uint(writer, ptr->length, version->svc_user_message_bits);
  bitwriter_write_bitstream(writer, &ptr->data);
}

static void handle_svc_entity_message(parser *thisptr, bitstream *stream,
                                      packet_net_message *message, blk* scrap) {
  struct demogobbler_svc_entity_message *ptr = &message->message_svc_entity_message;
  ptr->entity_index = bitstream_read_uint(stream, 11);
  ptr->class_id = bitstream_read_uint(stream, 9);
  ptr->length = bitstream_read_uint(stream, 11);
  ptr->data = bitstream_fork_and_advance(stream, ptr->length);
  SEND_MESSAGE();
}

static void write_svc_entity_message(bitwriter *writer, demo_version_data *version,
                                     packet_net_message *message) {
  struct demogobbler_svc_entity_message *ptr = &message->message_svc_entity_message;
  bitwriter_write_uint(writer, ptr->entity_index, 11);
  bitwriter_write_uint(writer, ptr->class_id, 9);
  bitwriter_write_uint(writer, ptr->length, 11);
  bitwriter_write_bitstream(writer, &ptr->data);
}

static void handle_svc_game_event(parser *thisptr, bitstream *stream, packet_net_message *message,
                                  blk* scrap) {
  struct demogobbler_svc_game_event *ptr = &message->message_svc_game_event;
  ptr->length = bitstream_read_uint(stream, 11);
  ptr->data = bitstream_fork_and_advance(stream, ptr->length);

  SEND_MESSAGE();
}

static void write_svc_game_event(bitwriter *writer, demo_version_data *version,
                                 packet_net_message *message) {
  struct demogobbler_svc_game_event *ptr = &message->message_svc_game_event;
  bitwriter_write_uint(writer, ptr->length, 11);
  bitwriter_write_bitstream(writer, &ptr->data);
}

static void handle_svc_packet_entities(parser *thisptr, bitstream *stream,
                                       packet_net_message *message, blk* scrap) {
  struct demogobbler_svc_packet_entities *ptr = &message->message_svc_packet_entities;
  ptr->max_entries = bitstream_read_uint(stream, 11);
  ptr->is_delta = bitstream_read_bit(stream);

  if (ptr->is_delta) {
    ptr->delta_from = bitstream_read_sint32(stream);
  } else {
    ptr->delta_from = -1;
  }

  ptr->base_line = bitstream_read_bit(stream);
  ptr->updated_entries = bitstream_read_uint(stream, 11);
  ptr->data_length = bitstream_read_uint(stream, 20);
  ptr->update_baseline = bitstream_read_bit(stream);
  ptr->data = bitstream_fork_and_advance(stream, ptr->data_length);
  SEND_MESSAGE();

  if(thisptr->m_settings.store_ents) {
    demogobbler_parse_packetentities(thisptr, ptr);
  }
}

static void write_svc_packet_entities(bitwriter *writer, demo_version_data *version,
                                      packet_net_message *message) {
  struct demogobbler_svc_packet_entities *ptr = &message->message_svc_packet_entities;
  bitwriter_write_uint(writer, ptr->max_entries, 11);
  bitwriter_write_bit(writer, ptr->is_delta);
  if (ptr->is_delta) {
    bitwriter_write_sint32(writer, ptr->delta_from);
  }

  bitwriter_write_bit(writer, ptr->base_line);
  bitwriter_write_uint(writer, ptr->updated_entries, 11);
  bitwriter_write_uint(writer, ptr->data_length, 20);
  bitwriter_write_bit(writer, ptr->update_baseline);
  bitwriter_write_bitstream(writer, &ptr->data);
}

static void handle_svc_temp_entities(parser *thisptr, bitstream *stream,
                                     packet_net_message *message, blk* scrap) {
  struct demogobbler_svc_temp_entities *ptr = &message->message_svc_temp_entities;
  ptr->num_entries = bitstream_read_uint(stream, 8);

  if (thisptr->demo_version.game == steampipe) {
    ptr->data_length = bitstream_read_varuint32(stream);
  } else if (thisptr->demo_version.game == l4d2) {
    ptr->data_length = bitstream_read_uint(stream, 18);
  } else {
    ptr->data_length = bitstream_read_uint(stream, 17);
  }
  ptr->data = bitstream_fork_and_advance(stream, ptr->data_length);

  SEND_MESSAGE();
}

static void write_svc_temp_entities(bitwriter *writer, demo_version_data *version,
                                    packet_net_message *message) {
  struct demogobbler_svc_temp_entities *ptr = &message->message_svc_temp_entities;
  bitwriter_write_uint(writer, ptr->num_entries, 8);

  if (version->game == steampipe) {
    bitwriter_write_varuint32(writer, ptr->data_length);
  } else if (version->game == l4d2) {
    bitwriter_write_uint(writer, ptr->data_length, 18);
  } else {
    bitwriter_write_uint(writer, ptr->data_length, 17);
  }
  bitwriter_write_bitstream(writer, &ptr->data);
}

static void handle_svc_prefetch(parser *thisptr, bitstream *stream, packet_net_message *message,
                                blk* scrap) {
  struct demogobbler_svc_prefetch *ptr = &message->message_svc_prefetch;
  ptr->sound_index = bitstream_read_uint(stream, thisptr->demo_version.svc_prefetch_bits);
  SEND_MESSAGE();
}

static void write_svc_prefetch(bitwriter *writer, demo_version_data *version,
                               packet_net_message *message) {
  struct demogobbler_svc_prefetch *ptr = &message->message_svc_prefetch;
  bitwriter_write_uint(writer, ptr->sound_index, version->svc_prefetch_bits);
}

static void handle_svc_menu(parser *thisptr, bitstream *stream, packet_net_message *message,
                            blk* scrap) {
  struct demogobbler_svc_menu *ptr = &message->message_svc_menu;
  ptr->menu_type = bitstream_read_uint(stream, 16);
  ptr->data_length = bitstream_read_uint32(stream);
  ptr->data = bitstream_fork_and_advance(stream, ptr->data_length);
  SEND_MESSAGE();
}

static void write_svc_menu(bitwriter *writer, demo_version_data *version,
                           packet_net_message *message) {
  writer->error = true;
  writer->error_message = "Writing not implemented for svc_menu";
}

static void handle_svc_game_event_list(parser *thisptr, bitstream *stream,
                                       packet_net_message *message, blk* scrap) {
  struct demogobbler_svc_game_event_list *ptr = &message->message_svc_game_event_list;
  ptr->events = bitstream_read_uint(stream, 9);
  ptr->length = bitstream_read_uint(stream, 20);
  ptr->data = bitstream_fork_and_advance(stream, ptr->length);
  SEND_MESSAGE();
}

static void write_svc_game_event_list(bitwriter *writer, demo_version_data *version,
                                      packet_net_message *message) {
  struct demogobbler_svc_game_event_list *ptr = &message->message_svc_game_event_list;
  bitwriter_write_uint(writer, ptr->events, 9);
  bitwriter_write_uint(writer, ptr->length, 20);
  bitwriter_write_bitstream(writer, &ptr->data);
}

static void handle_svc_get_cvar_value(parser *thisptr, bitstream *stream,
                                      packet_net_message *message, blk* scrap) {
  struct demogobbler_svc_get_cvar_value *ptr = &message->message_svc_get_cvar_value;
  ptr->cookie = bitstream_read_sint32(stream);
  COPY_STRING(ptr->cvar_name);
  SEND_MESSAGE();
}

static void write_svc_get_cvar_value(bitwriter *writer, demo_version_data *version,
                                     packet_net_message *message) {
  struct demogobbler_svc_get_cvar_value *ptr = &message->message_svc_get_cvar_value;
  bitwriter_write_sint32(writer, ptr->cookie);
  bitwriter_write_cstring(writer, ptr->cvar_name);
}

static void handle_net_splitscreen_user(parser *thisptr, bitstream *stream,
                                        packet_net_message *message, blk* scrap) {
  message->message_net_splitscreen_user.unk = bitstream_read_bit(stream);
  SEND_MESSAGE();
}

static void write_net_splitscreen_user(bitwriter *writer, demo_version_data *version,
                                       packet_net_message *message) {
  writer->error = true;
  writer->error_message = "Writing not implemented for net_splitscreen_user";
}

static void handle_svc_splitscreen(parser *thisptr, bitstream *stream, packet_net_message *message,
                                   blk* scrap) {
  struct demogobbler_svc_splitscreen *ptr = &message->message_svc_splitscreen;
  ptr->remove_user = bitstream_read_bit(stream);
  ptr->data_length = bitstream_read_uint(stream, 11);
  ptr->data = bitstream_fork_and_advance(stream, ptr->data_length);
  SEND_MESSAGE();
}

static void write_svc_splitscreen(bitwriter *writer, demo_version_data *version,
                                  packet_net_message *message) {
  writer->error = true;
  writer->error_message = "Writing not implemented for svc_splitscreen";
}

static void handle_svc_paintmap_data(parser *thisptr, bitstream *stream,
                                     packet_net_message *message, blk* scrap) {
  thisptr->error = true;
  thisptr->error_message = "svc_paintmap_data parsing is not implemented";
  SEND_MESSAGE();
}

static void write_svc_paintmap_data(bitwriter *writer, demo_version_data *version,
                                    packet_net_message *message) {
  writer->error = true;
  writer->error_message = "Writing not implemented for svc_paintmap_data";
}

static void handle_svc_cmd_key_values(parser *thisptr, bitstream *stream,
                                      packet_net_message *message, blk* scrap) {
  struct demogobbler_svc_cmd_key_values *ptr = &message->message_svc_cmd_key_values;
  ptr->data_length = bitstream_read_uint32(stream);
  ptr->data = bitstream_fork_and_advance(stream, ptr->data_length * 8);
  SEND_MESSAGE();
}

static void write_svc_cmd_key_values(bitwriter *writer, demo_version_data *version,
                                     packet_net_message *message) {
  struct demogobbler_svc_cmd_key_values *ptr = &message->message_svc_cmd_key_values;
  bitwriter_write_uint32(writer, ptr->data_length);
  bitwriter_write_bitstream(writer, &ptr->data);
}

static net_message_type version_get_message_type(parser *thisptr, unsigned int value) {
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

void demogobbler_bitwriter_write_netmessage(bitwriter *writer, demo_version_data *version,
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
    bitwriter_write_uint(writer, type_out, version->netmessage_type_bits);

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

static vector_array init_netmsg_array() {
  vector_array arr = demogobbler_va_create_(
      NULL, 0, sizeof(packet_net_message), alignof(packet_net_message));

  return arr;
}

void parse_netmessages(parser *thisptr, demogobbler_packet* packet) {
  void* data = packet->data;
  size_t size = packet->size_bytes;
  bitstream stream = demogobbler_bitstream_create(data, size * 8);
  // fprintf(stderr, "packet start:\n");

  // We allocate a single scrap buffer for the duration of parsing the packet that is as large as
  // the whole packet. Should never run out of space as long as we don't make things larger as we
  // read it out
  blk scrap_blk;
  scrap_blk.address = demogobbler_arena_allocate(&thisptr->temp_arena, size, 1);
  scrap_blk.size = size;
  unsigned int bits = thisptr->demo_version.netmessage_type_bits;

  vector_array packet_arr = init_netmsg_array(&thisptr->memory_arena);

  while (demogobbler_bitstream_bits_left(&stream) > bits && !thisptr->error && !stream.overflow) {
    if (scrap_blk.address == NULL) {
      thisptr->error = true;
      thisptr->error_message = "Unable to allocate scrap block";
      break;
    }

    unsigned int type_index = bitstream_read_uint(&stream, bits);
    net_message_type type = version_get_message_type(thisptr, type_index);
    //fprintf(stderr, "%d : %d\n", type_index, type);
    packet_net_message* message = demogobbler_va_push_back_empty(&packet_arr);
    memset(message, 0, sizeof(packet_net_message));
    message->_mtype = type_index;
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

  if(!thisptr->error) {
    packet_parsed parsed;
    memset(&parsed, 0, sizeof(parsed));
    parsed.messages = packet_arr.ptr;
    parsed.message_count = packet_arr.count_elements;
    parsed.orig = packet;
    parsed.leftover_bits = stream;

    if(thisptr->m_settings.packet_parsed_handler) {
      thisptr->m_settings.packet_parsed_handler(&thisptr->state, &parsed);
    }
  }

  // Ignore errors on negative tick packets, these are known to be bad
  if (packet->preamble.tick < 0 && packet->preamble.converted_type == demogobbler_type_packet && thisptr->error) {
    //printf("%s\n", thisptr->error_message);
    thisptr->error = false;
  }

  // fprintf(stderr, "packet end:\n");
  demogobbler_va_free(&packet_arr);
}
