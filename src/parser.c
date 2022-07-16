#include "parser.h"
#include "demogobbler.h"
#include "filereader.h"
#include "packettypes.h"
#include "utils.h"
#include "version_utils.h"
#include <stddef.h>
#include <string.h>

const int STACK_SIZE = 1 << 13;

#define thisreader &thisptr->m_reader
#define thisallocator &thisptr->allocator

void _parser_mainloop(parser *thisptr);
void _parse_header(parser *thisptr);
void _parse_consolecmd(parser *thisptr);
void _parse_customdata(parser *thisptr);
void _parse_datatables(parser *thisptr);
void _parse_packet(parser *thisptr, enum demogobbler_type type);
void _parse_stop(parser *thisptr);
void _parse_stringtables(parser *thisptr, int32_t type);
void _parse_synctick(parser *thisptr);
void _parse_usercmd(parser *thisptr);
bool _parse_anymessage(parser *thisptr);

void parser_init(parser *thisptr, demogobbler_settings *settings) {
  thisptr->m_settings = *settings;
  thisptr->error = false;
  thisptr->error_message = NULL;
  allocator_init(&thisptr->allocator, STACK_SIZE);
}

void parser_parse(parser *thisptr, void *stream, input_interface input) {
  if (stream) {
    filereader_init(thisreader, stream, input);
    _parse_header(thisptr);
    _parser_mainloop(thisptr);
    filereader_free(thisreader);
  }
}

void parser_free(parser *thisptr) { allocator_free(&thisptr->allocator); }

void _parse_header(parser *thisptr) {
  demogobbler_header header;
  filereader_readdata(thisreader, header.ID, 8);
  thisptr->demo_protocol = header.demo_protocol = filereader_readint32(thisreader);
  thisptr->net_protocol = header.net_protocol = filereader_readint32(thisreader);

  filereader_readdata(thisreader, header.server_name, 260);
  filereader_readdata(thisreader, header.client_name, 260);
  filereader_readdata(thisreader, header.map_name, 260);
  filereader_readdata(thisreader, header.game_directory, 260);
  header.seconds = filereader_readfloat(thisreader);
  header.tick_count = filereader_readint32(thisreader);
  header.frame_count = filereader_readint32(thisreader);
  header.signon_length = filereader_readint32(thisreader);

  thisptr->_demo_version = get_demo_version(&header);

  if (thisptr->m_settings.demo_version_handler) {
    thisptr->m_settings.demo_version_handler(thisptr->_demo_version);
  }

  if (thisptr->m_settings.header_handler) {
    thisptr->m_settings.header_handler(&header);
  }
}

bool _parse_anymessage(parser *thisptr) {
  uint8_t type = filereader_readbyte(thisreader);

  switch (type) {
  case demogobbler_type_consolecmd:
    _parse_consolecmd(thisptr);
    break;
  case demogobbler_type_datatables:
    _parse_datatables(thisptr);
    break;
  case demogobbler_type_packet:
    _parse_packet(thisptr, type);
    break;
  case demogobbler_type_signon:
    _parse_packet(thisptr, type);
    break;
  case demogobbler_type_stop:
    _parse_stop(thisptr);
    break;
  case demogobbler_type_synctick:
    _parse_synctick(thisptr);
    break;
  case demogobbler_type_usercmd:
    _parse_usercmd(thisptr);
    break;
  case 8:
    if (thisptr->demo_protocol < 4) {
      _parse_stringtables(thisptr, 8);
    } else {
      _parse_customdata(thisptr);
    }
    break;
  case 9:
    _parse_stringtables(thisptr, 9);
    break;
  default:
    thisptr->error = true;
    thisptr->error_message = "Invalid message type";
    break;
  }

  return type != demogobbler_type_stop && !thisptr->m_reader.eof &&
         !thisptr->error; // Return false when done parsing demo, or when at eof
}

#define PARSE_PREAMBLE()                                                                           \
  message.preamble.tick = filereader_readint32(thisreader);                                        \
  if (version_has_slot_in_preamble(thisptr->_demo_version))                                        \
    message.preamble.slot = filereader_readbyte(thisreader);

void _parser_mainloop(parser *thisptr) {
  // Add check if the only thing we care about is the header

  demogobbler_settings *settings = &thisptr->m_settings;
  bool should_parse = false;

#define NULL_CHECK(name)                                                                           \
  if (settings->name##_handler)                                                                    \
    should_parse = true;

  NULL_CHECK(consolecmd);
  NULL_CHECK(customdata);
  NULL_CHECK(datatables);
  NULL_CHECK(packet);
  NULL_CHECK(synctick);
  NULL_CHECK(stop);
  NULL_CHECK(stringtables);
  NULL_CHECK(usercmd);

#undef NULL_CHECK

  if (!should_parse)
    return;

  while (_parse_anymessage(thisptr))
    ;
}

void _parse_consolecmd(parser *thisptr) {
  demogobbler_consolecmd message;
  message.preamble.type = demogobbler_type_consolecmd;
  PARSE_PREAMBLE();

  if (thisptr->m_settings.consolecmd_handler) {
    message.size_bytes = filereader_readint32(thisreader);

    blk block = allocator_alloc(&thisptr->allocator, message.size_bytes);
    filereader_readdata(thisreader, block.address, message.size_bytes);
    message.data = block.address;
    thisptr->m_settings.consolecmd_handler(&message);
    allocator_dealloc(thisallocator, block);

  } else {
    message.size_bytes = filereader_readint32(thisreader);
    filereader_skipbytes(thisreader, message.size_bytes);
  }
}

void _parse_customdata(parser *thisptr) {
  demogobbler_customdata message;
  message.preamble.type = 8;
  PARSE_PREAMBLE();

  if (thisptr->m_settings.customdata_handler) {
    message.unknown = filereader_readint32(thisreader);
    message.size_bytes = filereader_readint32(thisreader);

    blk block = allocator_alloc(&thisptr->allocator, message.size_bytes);
    filereader_readdata(thisreader, block.address, message.size_bytes);
    message.data = block.address;
    thisptr->m_settings.customdata_handler(&message);
    allocator_dealloc(thisallocator, block);
  } else {
    filereader_skipbytes(thisreader, 4);
    message.size_bytes = filereader_readint32(thisreader);
    filereader_skipbytes(thisreader, message.size_bytes);
  }
}

void _parse_datatables(parser *thisptr) {
  demogobbler_datatables message;
  message.preamble.type = demogobbler_type_datatables;
  PARSE_PREAMBLE();

  if (thisptr->m_settings.datatables_handler) {
    message.size_bytes = filereader_readint32(thisreader);

    blk block = allocator_alloc(&thisptr->allocator, message.size_bytes);
    filereader_readdata(thisreader, block.address, message.size_bytes);
    message.data = block.address;
    thisptr->m_settings.datatables_handler(&message);
    allocator_dealloc(thisallocator, block);
  } else {
    message.size_bytes = filereader_readint32(thisreader);
    filereader_skipbytes(thisreader, message.size_bytes);
  }
}

void _parse_cmdinfo(parser *thisptr, demogobbler_cmdinfo *cmdinfo) {
  cmdinfo->interp_flags = filereader_readint32(thisreader);
  cmdinfo->view_origin = filereader_readvector(thisreader);
  cmdinfo->view_angles = filereader_readvector(thisreader);
  cmdinfo->local_viewangles = filereader_readvector(thisreader);
  cmdinfo->view_origin2 = filereader_readvector(thisreader);
  cmdinfo->view_angles2 = filereader_readvector(thisreader);
  cmdinfo->local_viewangles2 = filereader_readvector(thisreader);
}

void _parse_packet(parser *thisptr, enum demogobbler_type type) {
  demogobbler_packet message;
  message.preamble.type = type;
  PARSE_PREAMBLE();

  for (int i = 0; i < version_cmdinfo_size(thisptr->_demo_version); ++i) {
    _parse_cmdinfo(thisptr, &message.cmdinfo[i]);
  }

  message.in_sequence = filereader_readint32(thisreader);
  message.out_sequence = filereader_readint32(thisreader);
  message.size_bytes = filereader_readint32(thisreader);

  if (thisptr->m_settings.packet_handler) {
    blk block = allocator_alloc(&thisptr->allocator, message.size_bytes);
    filereader_readdata(thisreader, block.address, message.size_bytes);
    message.data = block.address;
    thisptr->m_settings.packet_handler(&message);
    allocator_dealloc(thisallocator, block);
  } else {
    filereader_skipbytes(thisreader, message.size_bytes);
  }
}

void _parse_stop(parser *thisptr) {

  if (thisptr->m_settings.stop_handler) {
    demogobbler_stop message;

    const int bytes_per_read = 4096;
    size_t bytes = 0;
    size_t bytesReadIt;
    blk block = allocator_alloc(&thisptr->allocator, bytes_per_read);

    do {
      if (bytes + bytes_per_read > block.size) {
        block = allocator_realloc(&thisptr->allocator, block, bytes + bytes_per_read);
      }

      bytesReadIt =
          filereader_readdata(thisreader, (uint8_t *)block.address + bytes, bytes_per_read);
      bytes += bytesReadIt;
    } while (bytesReadIt == bytes_per_read);
    message.size_bytes = bytes;
    message.data = block.address;

    thisptr->m_settings.stop_handler(&message);
    allocator_dealloc(thisallocator, block);
  }
}

void _parse_stringtables(parser *thisptr, int32_t type) {
  demogobbler_stringtables message;
  message.preamble.type = type;
  PARSE_PREAMBLE();

  if (thisptr->m_settings.stringtables_handler) {
    message.size_bytes = filereader_readint32(thisreader);

    blk block = allocator_alloc(&thisptr->allocator, message.size_bytes);
    filereader_readdata(thisreader, block.address, message.size_bytes);
    message.data = block.address;
    thisptr->m_settings.stringtables_handler(&message);
    allocator_dealloc(thisallocator, block);
  } else {
    message.size_bytes = filereader_readint32(thisreader);
    filereader_skipbytes(thisreader, message.size_bytes);
  }
}

void _parse_synctick(parser *thisptr) {
  demogobbler_synctick message;
  message.preamble.type = demogobbler_type_synctick;
  PARSE_PREAMBLE();

  if (thisptr->m_settings.synctick_handler) {
    thisptr->m_settings.synctick_handler(&message);
  }
}

void _parse_usercmd(parser *thisptr) {
  demogobbler_usercmd message;
  message.preamble.type = demogobbler_type_usercmd;
  PARSE_PREAMBLE();

  if (thisptr->m_settings.usercmd_handler) {
    message.cmd = filereader_readint32(thisreader);
    message.size_bytes = filereader_readint32(thisreader);

    blk block = allocator_alloc(&thisptr->allocator, message.size_bytes);
    filereader_readdata(thisreader, block.address, message.size_bytes);
    message.data = block.address;
    thisptr->m_settings.usercmd_handler(&message);
    allocator_dealloc(thisallocator, block);
  } else {
    filereader_skipbytes(thisreader, 4);
    int32_t size = filereader_readint32(thisreader);
    filereader_skipbytes(thisreader, size);
  }
}
