#include "demogobbler/parser.h"
#include "alignof_wrapper.h"
#include "demogobbler/allocator.h"
#include "demogobbler/streams.h"
#include "demogobbler.h"
#include "demogobbler/filereader.h"
#include "demogobbler/packettypes.h"
#include "hashtable.h"
#include "parser_datatables.h"
#include "parser_netmessages.h"
#include "parser_stringtables.h"
#include "utils.h"
#include "version_utils.h"
#include <stddef.h>
#include <string.h>

#define thisreader &thisptr->m_reader

// Optimization, read the cmdinfo bit as a uin32_t array instead of picking the individual words out
// Should be fine I think, but I'll check that the size and alignment match here just in case
// Assumes little endian I suppose but probably so does many other parts of this codebase
static_assert(sizeof(struct dg_cmdinfo_raw) == sizeof(dg_cmdinfo), "Bad length in cmdinfo_raw");
static_assert(alignof(struct dg_cmdinfo_raw) == 4, "Bad alignment in cmdinfo_raw");
static_assert(alignof(struct dg_cmdinfo_raw) == alignof(dg_cmdinfo),
              "Bad alignment in cmdinfo_raw");

void _parser_mainloop(dg_parser *thisptr);
void _parse_header(dg_parser *thisptr);
void _parse_consolecmd(dg_parser *thisptr);
void _parse_customdata(dg_parser *thisptr);
void _parse_datatables(dg_parser *thisptr);
void _parse_packet(dg_parser *thisptr, enum dg_type type);
void _parse_stop(dg_parser *thisptr);
void _parse_stringtables(dg_parser *thisptr, enum dg_type type);
void _parse_synctick(dg_parser *thisptr);
void _parse_usercmd(dg_parser *thisptr);
bool _parse_anymessage(dg_parser *thisptr);

static void set_allocator_funcs(dg_alloc_state* state)
{
  if(state->alloc == NULL)
    state->alloc = (func_dg_alloc)dg_arena_allocate;
  if(state->attach == NULL)
    state->attach = (func_dg_attach)dg_arena_attach;
  if(state->clear == NULL)
    state->clear = (func_dg_clear)dg_arena_clear;
  if(state->realloc == NULL)
    state->realloc = (func_dg_realloc)dg_arena_reallocate;
}

dg_parse_result dg_parse(dg_settings *settings, void *stream, dg_input_interface dg_input_interface) {
  const uint32_t INITIAL_SIZE = 1 << 17;
  dg_arena temp_arena = dg_arena_create(INITIAL_SIZE);
  dg_arena permanent_arena = dg_arena_create(INITIAL_SIZE);

  if(settings->permanent_alloc_state.allocator == NULL)
  {
    settings->permanent_alloc_state.allocator = &permanent_arena;
  }

  set_allocator_funcs(&settings->permanent_alloc_state);

  if(settings->temp_alloc_state.allocator == NULL)
  {
    settings->temp_alloc_state.allocator = &temp_arena;
  }

  set_allocator_funcs(&settings->temp_alloc_state);

  dg_parse_result out;
  memset(&out, 0, sizeof(out));
  dg_parser dg_parser;
  dg_parser_init(&dg_parser, settings);
  dg_parser_parse(&dg_parser, stream, dg_input_interface);
  out.error = dg_parser.error;
  out.error_message = dg_parser.error_message;

  dg_arena_free(&permanent_arena);
  dg_arena_free(&temp_arena);

  return out;
}

dg_parse_result dg_parse_file(dg_settings *settings, const char *filepath) {
  dg_parse_result out;
  memset(&out, 0, sizeof(out));
  FILE *file = fopen(filepath, "rb");

  if (file) {
    dg_input_interface input;
    input.read = dg_fstream_read;
    input.seek = dg_fstream_seek;

    out = dg_parse(settings, file, input);

    fclose(file);
  } else {
    out.error = true;
    out.error_message = "Unable to open file";
  }

  return out;
}

dg_parse_result dg_parse_buffer(dg_settings *settings, void *buffer, size_t size) {
  dg_parse_result out;
  memset(&out, 0, sizeof(out));

  if (buffer) {
    dg_input_interface input;
    input.read = dg_buffer_stream_read;
    input.seek = dg_buffer_stream_seek;

    buffer_stream stream;
    dg_buffer_stream_init(&stream, buffer, size);
    out = dg_parse(settings, &stream, input);
  } else {
    out.error = true;
    out.error_message = "Buffer was NULL";
  }

  return out;
}

void dg_settings_init(dg_settings *settings) { memset(settings, 0, sizeof(dg_settings)); }

dg_alloc_state* dg_parser_temp_allocator(dg_parser *thisptr)
{
  return &thisptr->m_settings.temp_alloc_state;
}

dg_alloc_state* dg_parser_perm_allocator(dg_parser *thisptr)
{
  return &thisptr->m_settings.permanent_alloc_state;
}

void* dg_parser_temp_allocate(dg_parser *thisptr, uint32_t size, uint32_t alignment)
{
  dg_alloc_state *a = dg_parser_temp_allocator(thisptr);
  return dg_alloc_allocate(a, size, alignment);
}

static void init_parsing_funcs(dg_parser *thisptr) {
  if (thisptr->_parser_funcs.parse_consolecmd == NULL)
    thisptr->_parser_funcs.parse_consolecmd = _parse_consolecmd;
  if (thisptr->_parser_funcs.parse_customdata == NULL)
    thisptr->_parser_funcs.parse_customdata = _parse_customdata;
  if (thisptr->_parser_funcs.parse_datatables == NULL)
    thisptr->_parser_funcs.parse_datatables = _parse_datatables;
  if (thisptr->_parser_funcs.parse_packet == NULL)
    thisptr->_parser_funcs.parse_packet = _parse_packet;
  if (thisptr->_parser_funcs.parse_stop == NULL)
    thisptr->_parser_funcs.parse_stop = _parse_stop;
  if (thisptr->_parser_funcs.parse_stringtables == NULL)
    thisptr->_parser_funcs.parse_stringtables = _parse_stringtables;
  if (thisptr->_parser_funcs.parse_synctick == NULL)
    thisptr->_parser_funcs.parse_synctick = _parse_synctick;
  if (thisptr->_parser_funcs.parse_usercmd == NULL)
    thisptr->_parser_funcs.parse_usercmd = _parse_usercmd;
}
void dg_parser_init(dg_parser *thisptr, dg_settings *settings) {
  memset(thisptr, 0, sizeof(*thisptr));
  thisptr->state.client_state = settings->client_state;
  thisptr->m_settings = *settings;
  thisptr->_parser_funcs = settings->funcs;
  init_parsing_funcs(thisptr);
}

void dg_parser_parse(dg_parser *thisptr, void *stream, dg_input_interface input) {
  if (stream) {
    enum { FILE_BUFFER_SIZE = 1 << 15 };
    uint8_t buffer[FILE_BUFFER_SIZE / sizeof(uint8_t)];
    dg_filereader_init(thisreader, buffer, sizeof(buffer), stream, input);
    _parse_header(thisptr);
    _parser_mainloop(thisptr);
  }
}

void dg_parser_update_l4d2_version(dg_parser *thisptr, int l4d2_version) {
  thisptr->demo_version.l4d2_version = l4d2_version;
  thisptr->demo_version.l4d2_version_finalized = true;
  version_update_build_info(&thisptr->demo_version);
  if (thisptr->m_settings.demo_version_handler) {
    thisptr->m_settings.demo_version_handler(&thisptr->state, thisptr->demo_version);
  }
}

dg_parse_result dg_parser_add_stringtable(dg_parser *thisptr, dg_sentry* table) {
  dg_parse_result result;
  memset(&result, 0, sizeof(result));

  if(thisptr->state.stringtables_count >= MAX_STRINGTABLES) {
    result.error = true;
    result.error_message = "demo had too many stringtables";
  }

  dg_stringtable_data* data = thisptr->state.stringtables + thisptr->state.stringtables_count;
  data->flags = table->flags;
  data->max_entries = table->max_entries;
  data->user_data_fixed_size = table->user_data_fixed_size;
  data->user_data_size_bits = table->user_data_size_bits;
  ++thisptr->state.stringtables_count;

  return result;
}

size_t _parser_read_length(dg_parser *thisptr) {
  int32_t result = dg_filereader_readint32(thisreader);
  int32_t max_len = 1 << 25; // more than 32 megabytes is probably an error
  if (result < 0 || result > max_len) {
    thisptr->error = true;
    thisptr->error_message = "Length was invalid\n";
    return 0;
  } else {
    return result;
  }
}

void _parse_header(dg_parser *thisptr) {
  dg_header header;
  dg_filereader_readdata(thisreader, header.ID, 8);
  header.demo_protocol = dg_filereader_readint32(thisreader);
  header.net_protocol = dg_filereader_readint32(thisreader);

  dg_filereader_readdata(thisreader, header.server_name, 260);
  dg_filereader_readdata(thisreader, header.client_name, 260);
  dg_filereader_readdata(thisreader, header.map_name, 260);
  dg_filereader_readdata(thisreader, header.game_directory, 260);
  header.seconds = dg_filereader_readfloat(thisreader);
  header.tick_count = dg_filereader_readint32(thisreader);
  header.frame_count = dg_filereader_readint32(thisreader);
  header.signon_length = dg_filereader_readint32(thisreader);

  // Add null terminators if they are missing
  header.ID[7] = header.server_name[259] = header.client_name[259] = header.map_name[259] =
      header.game_directory[259] = '\0';

  thisptr->demo_version = dg_get_demo_version(&header);

  if (thisptr->m_settings.demo_version_handler) {
    thisptr->m_settings.demo_version_handler(&thisptr->state, thisptr->demo_version);
  }

  if (thisptr->m_settings.header_handler) {
    thisptr->m_settings.header_handler(&thisptr->state, &header);
  }
}

bool _parse_anymessage(dg_parser *thisptr) {
  uint8_t type = dg_filereader_readbyte(thisreader);

  // Don't clear if temp allocator is the same as permanent allocator
  if(thisptr->m_settings.temp_alloc_state.allocator != thisptr->m_settings.permanent_alloc_state.allocator)
    dg_alloc_clear(dg_parser_temp_allocator(thisptr));

  switch (type) {
  case dg_type_consolecmd:
    thisptr->_parser_funcs.parse_consolecmd(thisptr);
    break;
  case dg_type_datatables:
    thisptr->_parser_funcs.parse_datatables(thisptr);
    break;
  case dg_type_packet:
    thisptr->_parser_funcs.parse_packet(thisptr, type);
    break;
  case dg_type_signon:
    thisptr->_parser_funcs.parse_packet(thisptr, type);
    break;
  case dg_type_stop:
    thisptr->_parser_funcs.parse_stop(thisptr);
    break;
  case dg_type_synctick:
    thisptr->_parser_funcs.parse_synctick(thisptr);
    break;
  case dg_type_usercmd:
    thisptr->_parser_funcs.parse_usercmd(thisptr);
    break;
  case 8:
    if (thisptr->demo_version.demo_protocol < 4) {
      thisptr->_parser_funcs.parse_stringtables(thisptr, 8);
    } else {
      thisptr->_parser_funcs.parse_customdata(thisptr);
    }
    break;
  case 9:
    thisptr->_parser_funcs.parse_stringtables(thisptr, 9);
    break;
  default:
    thisptr->error = true;
    thisptr->error_message = "Invalid message type";
    break;
  }

  return type != dg_type_stop && !thisptr->m_reader.eof &&
         !thisptr->error; // Return false when done parsing demo, or when at eof
}

static void parser_free_state(dg_parser *thisptr) {
  dg_estate_free(&thisptr->state.entity_state);
}

#define PARSE_PREAMBLE()                                                                           \
  message.preamble.tick = dg_filereader_readint32(thisreader);                                     \
  if (thisptr->demo_version.has_slot_in_preamble)                                                  \
    message.preamble.slot = dg_filereader_readbyte(thisreader);

void _parser_mainloop(dg_parser *thisptr) {
  // Add check if the only thing we care about is the header

  dg_settings *settings = &thisptr->m_settings;
  bool should_parse = false;

#define NULL_CHECK(name)                                                                           \
  if (settings->name##_handler)                                                                    \
    should_parse = true;

  NULL_CHECK(packet_parsed);
  NULL_CHECK(consolecmd);
  NULL_CHECK(customdata);
  NULL_CHECK(datatables);
  NULL_CHECK(datatables_parsed);
  NULL_CHECK(packet);
  NULL_CHECK(synctick);
  NULL_CHECK(stop);
  NULL_CHECK(stringtables);
  NULL_CHECK(stringtables_parsed);
  NULL_CHECK(usercmd);
  NULL_CHECK(flattened_props);

  if (settings->parse_packetentities || settings->packetentities_parsed_handler) {
    settings->parse_packetentities = true; // Entity state init handler => we should store ents
    should_parse = true;
    thisptr->parse_netmessages = true;
  }

  if (!thisptr->demo_version.l4d2_version_finalized && settings->demo_version_handler) {
    should_parse = true;
  }

  if (settings->packet_parsed_handler) {
    should_parse = true;
    thisptr->parse_netmessages = true;
  }

#undef NULL_CHECK

  if (!should_parse)
    return;

  while (_parse_anymessage(thisptr))
    ;
  parser_free_state(thisptr);
}

#define READ_MESSAGE_DATA()                                                                        \
  {                                                                                                \
    size_t read_bytes = dg_filereader_readdata(thisreader, block, message.size_bytes);             \
    if (read_bytes != message.size_bytes) {                                                        \
      thisptr->error = true;                                                                       \
      thisptr->error_message = "Message could not be read fully, reached end of file.";            \
    }                                                                                              \
    message.data = block;                                                                          \
  }

void _parse_consolecmd(dg_parser *thisptr) {
  dg_consolecmd message;
  message.preamble.type = message.preamble.converted_type = dg_type_consolecmd;
  PARSE_PREAMBLE();
  message.size_bytes = _parser_read_length(thisptr);

  if (message.size_bytes > 0) {
    ;
    if (thisptr->m_settings.consolecmd_handler) {
      void *block = dg_parser_temp_allocate(thisptr, message.size_bytes, 1);
      READ_MESSAGE_DATA();

      if (!thisptr->error) {
        message.data[message.size_bytes - 1] = '\0'; // Add null terminator in-case malformed data
        thisptr->m_settings.consolecmd_handler(&thisptr->state, &message);
      }

    } else {
      dg_filereader_skipbytes(thisreader, message.size_bytes);
    }
  } else {
    thisptr->error = true;
    thisptr->error_message = "Invalid consolecommand length";
  }
}

void _parse_customdata(dg_parser *thisptr) {
  dg_customdata message;
  message.preamble.type = message.preamble.converted_type = dg_type_customdata;
  PARSE_PREAMBLE();

  message.unknown = dg_filereader_readint32(thisreader);
  message.size_bytes = _parser_read_length(thisptr);

  if (thisptr->m_settings.customdata_handler && message.size_bytes > 0) {
    void *block = dg_parser_temp_allocate(thisptr, message.size_bytes, 1);
    READ_MESSAGE_DATA();
    if (!thisptr->error) {
      thisptr->m_settings.customdata_handler(&thisptr->state, &message);
    }
  } else {
    dg_filereader_skipbytes(thisreader, message.size_bytes);
  }
}

void _parse_datatables(dg_parser *thisptr) {
  dg_datatables message;
  message.preamble.type = message.preamble.converted_type = dg_type_datatables;
  PARSE_PREAMBLE();

  message.size_bytes = _parser_read_length(thisptr);

  bool has_datatable_handler = thisptr->m_settings.datatables_handler ||
                               thisptr->m_settings.datatables_parsed_handler ||
                               thisptr->m_settings.parse_packetentities ||
                               thisptr->m_settings.flattened_props_handler;

  if (has_datatable_handler && message.size_bytes > 0) {
    void *block = dg_parser_temp_allocate(thisptr, message.size_bytes, 1);
    READ_MESSAGE_DATA();
    if (!thisptr->error) {

      if (thisptr->m_settings.datatables_handler)
        thisptr->m_settings.datatables_handler(&thisptr->state, &message);

      if (thisptr->m_settings.datatables_parsed_handler || thisptr->m_settings.parse_packetentities || thisptr->m_settings.flattened_props_handler)
        parse_datatables(thisptr, &message);
    }
  } else {
    dg_filereader_skipbytes(thisreader, message.size_bytes);
  }
}

static void _parse_cmdinfo(dg_parser *thisptr, dg_packet *packet, size_t i) {
  bool should_parse_netmessages =
      !thisptr->demo_version.l4d2_version_finalized || thisptr->parse_netmessages;
  if (thisptr->m_settings.packet_handler || should_parse_netmessages) {
    dg_filereader_readdata(thisreader, packet->cmdinfo_raw[i].data,
                           sizeof(packet->cmdinfo_raw[i].data));
  } else {
    dg_filereader_skipbytes(thisreader, sizeof(struct dg_cmdinfo_raw));
  }
}

void _parse_packet(dg_parser *thisptr, enum dg_type type) {
  dg_packet message;
  message.preamble.type = message.preamble.converted_type = type;
  PARSE_PREAMBLE();
  message.cmdinfo_size = thisptr->demo_version.cmdinfo_size;
  bool should_parse_netmessages =
      !thisptr->demo_version.l4d2_version_finalized || thisptr->parse_netmessages;

  for (int i = 0; i < message.cmdinfo_size; ++i) {
    _parse_cmdinfo(thisptr, &message, i);
  }

  message.in_sequence = dg_filereader_readint32(thisreader);
  message.out_sequence = dg_filereader_readint32(thisreader);
  message.size_bytes = _parser_read_length(thisptr);

  if ((thisptr->m_settings.packet_handler || should_parse_netmessages) && message.size_bytes > 0) {
    void *block = dg_parser_temp_allocate(thisptr, message.size_bytes, 1);
    READ_MESSAGE_DATA();
    if (!thisptr->error) {

      if (thisptr->m_settings.packet_handler) {
        thisptr->m_settings.packet_handler(&thisptr->state, &message);
      }

      if (should_parse_netmessages) {
        parse_netmessages(thisptr, &message);
      }
    }

  } else {
    dg_filereader_skipbytes(thisreader, message.size_bytes);
  }
}

void _parse_stop(dg_parser *thisptr) {

  if (thisptr->m_settings.stop_handler) {
    dg_stop message;

    const size_t bytes_per_read = 4096;
    size_t bytes = 0;
    size_t bytes_reserved = 0;
    size_t bytesReadIt;
    void *ptr = NULL;

    do {
      size_t new_reserve_size = bytes_reserved + bytes_per_read;
      dg_alloc_state* a = dg_parser_temp_allocator(thisptr);
      ptr = dg_alloc_reallocate(a, ptr, bytes_reserved, new_reserve_size, 1);
      bytes_reserved = new_reserve_size;

      bytesReadIt = dg_filereader_readdata(thisreader, (uint8_t *)ptr + bytes, bytes_per_read);
      bytes += bytesReadIt;
    } while (bytesReadIt == bytes_per_read);
    message.size_bytes = bytes;
    message.data = ptr;

    thisptr->m_settings.stop_handler(&thisptr->state, &message);
  }
}

void _parse_stringtables(dg_parser *thisptr, enum dg_type type) {
  dg_stringtables message;
  message.preamble.type = type;
  message.preamble.converted_type = dg_type_stringtables;
  PARSE_PREAMBLE();
  message.size_bytes = _parser_read_length(thisptr);
  bool should_parse =
      thisptr->m_settings.stringtables_parsed_handler || thisptr->m_settings.stringtables_handler;

  if (should_parse && message.size_bytes > 0) {
    void *block = dg_parser_temp_allocate(thisptr, message.size_bytes, 1);
    READ_MESSAGE_DATA();
    if (!thisptr->error) {
      if (thisptr->m_settings.stringtables_handler)
        thisptr->m_settings.stringtables_handler(&thisptr->state, &message);
      if (thisptr->m_settings.stringtables_parsed_handler) {
        dg_parser_parse_stringtables(thisptr, &message);
      }
    }
  } else {
    dg_filereader_skipbytes(thisreader, message.size_bytes);
  }
}

void _parse_synctick(dg_parser *thisptr) {
  dg_synctick message;
  message.preamble.type = message.preamble.converted_type = dg_type_synctick;
  PARSE_PREAMBLE();

  if (thisptr->m_settings.synctick_handler) {
    thisptr->m_settings.synctick_handler(&thisptr->state, &message);
  }
}

void _parse_usercmd(dg_parser *thisptr) {
  dg_usercmd message;
  message.preamble.type = message.preamble.converted_type = dg_type_usercmd;
  PARSE_PREAMBLE();

  message.cmd = dg_filereader_readint32(thisreader);
  message.size_bytes = dg_filereader_readint32(thisreader);

  if (thisptr->m_settings.usercmd_handler) {
    if (message.size_bytes > 0) {
      void *block = dg_parser_temp_allocate(thisptr, message.size_bytes, 1);
      READ_MESSAGE_DATA();
    } else {
      message.data = NULL;
    }
    if (!thisptr->error) {
      thisptr->m_settings.usercmd_handler(&thisptr->state, &message);
    }
  } else {
    dg_filereader_skipbytes(thisreader, message.size_bytes);
  }
}
