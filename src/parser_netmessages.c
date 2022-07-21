#include "parser_netmessages.h"
#include "utils.h"
#include <math.h>

// clang-format off
static net_message_type old_protocol_message_list[] =
{
  net_nop,
  net_disconnect,
  net_file,
  net_tick,
  net_stringcmd,
  net_setconvar,
  net_signonstate,
  svc_print,
  svc_serverinfo,
  svc_sendtable,
  svc_classinfo,
  svc_setpause,
  svc_create_stringtable,
  svc_update_stringtable,
  svc_voice_init,
  svc_voice_data,
  svc_invalid, // svc_HLTV
  svc_sounds,
  svc_setview,
  svc_fixangle,
  svc_crosshair_angle,
  svc_bsp_decal,
  svc_invalid, // svc_terrainmod
  svc_user_message,
  svc_entity_message,
  svc_game_event,
  svc_packet_entities,
  svc_temp_entities,
  svc_prefetch,
  svc_menu,
  svc_game_event_list,
  svc_get_cvar_value
};
// clang-format on

static net_message_type parser_get_message_type(parser *thisptr, unsigned int value) {
  demo_version version = thisptr->_demo_version;
  net_message_type *list;
  size_t count;

  if (version == orangebox) {
    list = old_protocol_message_list;
    count = ARRAYSIZE(old_protocol_message_list);
  } else {
    list = NULL;
    count = 0;
  }

  if (value >= count || list[value] == svc_invalid) {
    thisptr->error = true;
    thisptr->error_message = "Encountered bad net message type.\n";
    return svc_invalid;
  } else {
    return list[value];
  }
}

#define TOSS_STRING() bitstream_read_cstring(stream, scrap, 4096);
#define COPY_STRING(member) prev_string = scrap; scrap += bitstream_read_cstring(stream, scrap, 4096); \
  message->message_ ## member = prev_string;
#define SEND_MESSAGE() if(!stream->overflow && thisptr->m_settings.packet_net_message_handler) thisptr->m_settings.packet_net_message_handler(thisptr->parent->client_state, message);

static void handle_net_nop(parser* thisptr, bitstream* stream, packet_net_message* message, char* scrap) {
  SEND_MESSAGE();
}

static void handle_net_disconnect(parser* thisptr, bitstream* stream, packet_net_message* message, char* scrap) {
  char* prev_string = NULL;
  COPY_STRING(net_disconnect.text);
  SEND_MESSAGE();
}

static void handle_net_file(parser* thisptr, bitstream* stream, packet_net_message* message, char* scrap) {
  char* prev_string = NULL;
  message->message_net_file.transfer_id = bitstream_read_uint32(stream);
  COPY_STRING(net_file.filename);
  // TODO: Check number of bits here depending on game
  message->message_net_file.file_requested = bitstream_read_uint(stream, 1);
  SEND_MESSAGE();
}

static void handle_net_tick(parser* thisptr, bitstream* stream, packet_net_message* message, char* scrap) {
  const float net_tick_scale_up = 100000.0f;
  message->message_net_tick.tick = bitstream_read_uint32(stream);
  // TODO: Not in OE
  message->message_net_tick.host_frame_time = bitstream_read_uint(stream, 16);
  message->message_net_tick.host_frame_time_std_dev = bitstream_read_uint(stream, 16);
  SEND_MESSAGE();
}

static void handle_net_stringcmd(parser* thisptr, bitstream* stream, packet_net_message* message, char* scrap) {
  char* prev_string = NULL;
  COPY_STRING(net_stringcmd.command);
  SEND_MESSAGE();
}

static void handle_net_setconvar(parser* thisptr, bitstream* stream, packet_net_message* message, char* scrap) {
  message->message_net_setconvar.count = bitstream_read_uint(stream, 8);
  for(size_t i=0; i < message->message_net_setconvar.count; ++i) {
    TOSS_STRING();
    TOSS_STRING();
  }
  SEND_MESSAGE();
}

static void handle_net_signonstate(parser* thisptr, bitstream* stream, packet_net_message* message, char* scrap) {
  message->message_net_signonstate.signon_state = bitstream_read_uint(stream, 8);
  message->message_net_signonstate.spawn_count = bitstream_read_sint32(stream);

  // TODO: Add new protocol stuff

  // Uncrafted: GameState.ClientSoundSequence = 1; reset sound sequence number after receiving SignOn sounds
  SEND_MESSAGE();
}

static void handle_svc_print(parser* thisptr, bitstream* stream, packet_net_message* message, char* scrap) {
  char* prev_string = NULL;
  COPY_STRING(svc_print.message);
  SEND_MESSAGE();
}

static void handle_svc_serverinfo(parser* thisptr, bitstream* stream, packet_net_message* message, char* scrap) {
  char* prev_string = NULL;
  message->message_svc_serverinfo.network_protocol = bitstream_read_uint(stream, 16);
  message->message_svc_serverinfo.server_count = bitstream_read_uint32(stream);
  message->message_svc_serverinfo.is_hltv = bitstream_read_uint(stream, 1);
  message->message_svc_serverinfo.is_dedicated = bitstream_read_uint(stream, 1);
  // TODO: l4d add unknown bit
  message->message_svc_serverinfo.client_crc = bitstream_read_sint32(stream);
  // TODO: new demo protocol, string table crc
  message->message_svc_serverinfo.max_classes = bitstream_read_uint(stream, 16);
  // TODO: Add MD5 for network protocol 24
  message->message_svc_serverinfo.map_crc = bitstream_read_uint32(stream);
  message->message_svc_serverinfo.player_count = bitstream_read_uint(stream, 8);
  message->message_svc_serverinfo.max_clients = bitstream_read_uint(stream, 8);
  message->message_svc_serverinfo.tick_interval = bitstream_read_float(stream);
  message->message_svc_serverinfo.platform = bitstream_read_uint(stream, 8);
  COPY_STRING(svc_serverinfo.game_dir);
  COPY_STRING(svc_serverinfo.map_name);
  COPY_STRING(svc_serverinfo.sky_name);
  COPY_STRING(svc_serverinfo.host_name);

  // TODO: Some shit is missing
  SEND_MESSAGE();
}

static void handle_svc_sendtable(parser* thisptr, bitstream* stream, packet_net_message* message, char* scrap) {
  thisptr->error = true;
  thisptr->error_message = "svc_sendtable parsing is not implemented";
  SEND_MESSAGE();
}

static void handle_svc_classinfo(parser* thisptr, bitstream* stream, packet_net_message* message, char* scrap) {
  message->message_svc_classinfo.length = bitstream_read_uint(stream, 16);
  message->message_svc_classinfo.create_on_client = bitstream_read_uint(stream, 1);
  unsigned int bits = log2(message->message_svc_classinfo.length) + 1;

  if(!message->message_svc_classinfo.create_on_client)
  {
    for(unsigned int i = 0; i < message->message_svc_classinfo.length; ++i) {
      unsigned int class_id = bitstream_read_uint(stream, bits);
      TOSS_STRING(); // Class name
      TOSS_STRING(); // Datatable name
    }
  }
  SEND_MESSAGE();
}

static void handle_svc_setpause(parser* thisptr, bitstream* stream, packet_net_message* message, char* scrap) {
  message->message_svc_setpause.paused = bitstream_read_uint(stream, 1);
  SEND_MESSAGE();
}

static void handle_svc_create_stringtable(parser* thisptr, bitstream* stream, packet_net_message* message, char* scrap) {
  char* prev_string = NULL;
  COPY_STRING(svc_create_stringtable.name);
  unsigned int max_entries = bitstream_read_uint(stream, 16);
  unsigned int num_entries_bits = log2(max_entries) + 1;
  message->message_svc_create_stringtable.max_entries = max_entries;
  message->message_svc_create_stringtable.num_entries = bitstream_read_uint(stream, num_entries_bits);
  // 21 bits for l4d2
  unsigned int data_length = bitstream_read_uint(stream, 20);
  message->message_svc_create_stringtable.data_length = data_length;
  bool fixed_size_data = bitstream_read_uint(stream, 1);

  if(fixed_size_data) {
    message->message_svc_create_stringtable.user_data_fixed_size = bitstream_read_uint(stream, 12);
    message->message_svc_create_stringtable.user_data_size_bits = bitstream_read_uint(stream, 4);
  }
  else {
    message->message_svc_create_stringtable.user_data_fixed_size = 0;
    message->message_svc_create_stringtable.user_data_size_bits = 0;
  }

  // TODO: Net protocol >= 15 only
  message->message_svc_create_stringtable.flags = bitstream_read_uint(stream, 1); // 2 bits in new demo protocol
  message->message_svc_create_stringtable.data = *stream;
  message->message_svc_create_stringtable.data.bitoffset = 0;
  message->message_svc_create_stringtable.data.bitsize = data_length;
  demogobbler_bitstream_advance(stream, data_length);

  SEND_MESSAGE();
}

static void handle_svc_update_stringtable(parser* thisptr, bitstream* stream, packet_net_message* message, char* scrap) {
  thisptr->error = true;
  thisptr->error_message = "svc_update_stringtable parsing is not implemented";
  SEND_MESSAGE();
}

static void handle_svc_voice_init(parser* thisptr, bitstream* stream, packet_net_message* message, char* scrap) {
  thisptr->error = true;
  thisptr->error_message = "svc_voice_init parsing is not implemented";
  SEND_MESSAGE();
}

static void handle_svc_voice_data(parser* thisptr, bitstream* stream, packet_net_message* message, char* scrap) {
  thisptr->error = true;
  thisptr->error_message = "svc_voice_data parsing is not implemented";
  SEND_MESSAGE();
}

static void handle_svc_sounds(parser* thisptr, bitstream* stream, packet_net_message* message, char* scrap) {
  thisptr->error = true;
  thisptr->error_message = "svc_sounds parsing is not implemented";
  SEND_MESSAGE();
}

static void handle_svc_setview(parser* thisptr, bitstream* stream, packet_net_message* message, char* scrap) {
  thisptr->error = true;
  thisptr->error_message = "svc_setview parsing is not implemented";
  SEND_MESSAGE();
}

static void handle_svc_fixangle(parser* thisptr, bitstream* stream, packet_net_message* message, char* scrap) {
  thisptr->error = true;
  thisptr->error_message = "svc_fixangle parsing is not implemented";
  SEND_MESSAGE();
}

static void handle_svc_crosshair_angle(parser* thisptr, bitstream* stream, packet_net_message* message, char* scrap) {
  thisptr->error = true;
  thisptr->error_message = "svc_crosshair_angle parsing is not implemented";
  SEND_MESSAGE();
}

static void handle_svc_bsp_decal(parser* thisptr, bitstream* stream, packet_net_message* message, char* scrap) {
  thisptr->error = true;
  thisptr->error_message = "svc_bsp_decal parsing is not implemented";
  SEND_MESSAGE();
}

static void handle_svc_user_message(parser* thisptr, bitstream* stream, packet_net_message* message, char* scrap) {
  thisptr->error = true;
  thisptr->error_message = "svc_user_message parsing is not implemented";
  SEND_MESSAGE();
}

static void handle_svc_entity_message(parser* thisptr, bitstream* stream, packet_net_message* message, char* scrap) {
  thisptr->error = true;
  thisptr->error_message = "svc_entity_message parsing is not implemented";
  SEND_MESSAGE();
}

static void handle_svc_game_event(parser* thisptr, bitstream* stream, packet_net_message* message, char* scrap) {
  thisptr->error = true;
  thisptr->error_message = "svc_game_event parsing is not implemented";
  SEND_MESSAGE();
}

static void handle_svc_packet_entities(parser* thisptr, bitstream* stream, packet_net_message* message, char* scrap) {
  thisptr->error = true;
  thisptr->error_message = "svc_packet_entities parsing is not implemented";
  SEND_MESSAGE();
}

static void handle_svc_temp_entities(parser* thisptr, bitstream* stream, packet_net_message* message, char* scrap) {
  thisptr->error = true;
  thisptr->error_message = "svc_temp_entities parsing is not implemented";
  SEND_MESSAGE();
}

static void handle_svc_prefetch(parser* thisptr, bitstream* stream, packet_net_message* message, char* scrap) {
  thisptr->error = true;
  thisptr->error_message = "svc_prefetch parsing is not implemented";
  SEND_MESSAGE();
}

static void handle_svc_menu(parser* thisptr, bitstream* stream, packet_net_message* message, char* scrap) {
  thisptr->error = true;
  thisptr->error_message = "svc_menu parsing is not implemented";
  SEND_MESSAGE();
}

static void handle_svc_game_event_list(parser* thisptr, bitstream* stream, packet_net_message* message, char* scrap) {
  thisptr->error = true;
  thisptr->error_message = "svc_game_event_list parsing is not implemented";
  SEND_MESSAGE();
}

static void handle_svc_get_cvar_value(parser* thisptr, bitstream* stream, packet_net_message* message, char* scrap) {
  thisptr->error = true;
  thisptr->error_message = "svc_get_cvar_value parsing is not implemented";
  SEND_MESSAGE();
}

static void handle_net_splitscreen_user(parser* thisptr, bitstream* stream, packet_net_message* message, char* scrap) {
  thisptr->error = true;
  thisptr->error_message = "net_splitscreen_user parsing is not implemented";
  SEND_MESSAGE();
}

void parse_netmessages(parser *thisptr, void *data, size_t size) {
  bitstream stream = demogobbler_bitstream_create(data, size * 8);

  // We allocate a single scrap buffer for the duration of parsing the packet that is as large as the whole packet
  // Should never run out of space as long as we don't make things larger as we read it out
  blk scrap_blk = allocator_alloc(&thisptr->allocator, size);
  char* scrap = scrap_blk.address;

  while (demogobbler_bitstream_bits_left(&stream) >= 6 && !thisptr->error && !stream.overflow) {
    unsigned int type_index;
    demogobbler_bitstream_read_bits(&stream, &type_index, 6);
    net_message_type type = parser_get_message_type(thisptr, type_index);
    packet_net_message message;
    message._mtype = type_index;
    message.mtype = type;


#define DECLARE_SWITCH_STATEMENT(message_type)                                                     \
  case message_type:                                                                               \
    handle_##message_type(thisptr, &stream, &message, scrap);                                                       \
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

  if(stream.overflow && !thisptr->error) {
    thisptr->error = true;
    thisptr->error_message = "Bitstream overflowed during packet parsing";
  }

  allocator_dealloc(&thisptr->allocator, scrap_blk);
}