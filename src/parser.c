#include "parser.h"
#include "demogobbler.h"
#include "filereader.h"
#include "packettypes.h"
#include "stddef.h"

#define thisreader &thisptr->m_reader

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
}

void parser_parse(parser *thisptr, const char *filepath) {
  if (!filepath)
    return;

  FILE *stream = fopen(filepath, "rb");

  if (stream) {
    filereader_init(&thisptr->m_reader, stream);
    _parse_header(thisptr);
    _parser_mainloop(thisptr);
    filereader_free(&thisptr->m_reader);
  }
}

void _parse_header(parser *thisptr) {
  demogobbler_header header;
  filereader_readdata(&thisptr->m_reader, header.ID, 8);
  thisptr->demo_protocol = header.demo_protocol = filereader_readint32(&thisptr->m_reader);
  thisptr->net_protocol = header.net_protocol = filereader_readint32(&thisptr->m_reader);

  if (thisptr->m_settings.header_handler) {
    filereader_readdata(&thisptr->m_reader, header.server_name, 260);
    filereader_readdata(&thisptr->m_reader, header.client_name, 260);
    filereader_readdata(&thisptr->m_reader, header.map_name, 260);
    filereader_readdata(&thisptr->m_reader, header.game_directory, 260);
    header.seconds = filereader_readfloat(&thisptr->m_reader);
    header.tick_count = filereader_readint32(&thisptr->m_reader);
    header.event_count = filereader_readint32(&thisptr->m_reader);
    header.signon_length = filereader_readint32(&thisptr->m_reader);

    thisptr->m_settings.header_handler(&header);
  }

  filereader_skipto(&thisptr->m_reader, 0x430);
}

bool _parse_anymessage(parser *thisptr) {
  uint8_t type = filereader_readbyte(&thisptr->m_reader);

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
    break;
  }

  return type != demogobbler_type_stop &&
         !thisptr->m_reader.eof; // Return false when done parsing demo, or when at eof
}

#define PARSE_PREAMBLE() message.preamble.tick = filereader_readint32(thisreader);

void _parser_mainloop(parser *thisptr) {
  // Add check if the only thing we care about is the header

  while (_parse_anymessage(thisptr))
    ;
}

void _parse_consolecmd(parser *thisptr) {
  demogobbler_consolecmd message;
  message.preamble.type = demogobbler_type_consolecmd;
  PARSE_PREAMBLE();

  if (thisptr->m_settings.consolecmd_handler) {
    message.size_bytes = filereader_readint32(thisreader);
    message.console_cmd = NULL;
    filereader_skipbytes(thisreader, message.size_bytes);
    thisptr->m_settings.consolecmd_handler(&message);
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
    message.data = NULL;
    filereader_skipbytes(thisreader, message.size_bytes);
    thisptr->m_settings.customdata_handler(&message);
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
    message.data = NULL;
    filereader_skipbytes(thisreader, message.size_bytes);
    thisptr->m_settings.datatables_handler(&message);
  } else {
    message.size_bytes = filereader_readint32(thisreader);
    filereader_skipbytes(thisreader, message.size_bytes);
  }
}

void _parse_packet(parser *thisptr, enum demogobbler_type type) {
  demogobbler_packet message;
  message.preamble.type = type;
  PARSE_PREAMBLE();

  if (thisptr->m_settings.packet_handler) {
    filereader_skipbytes(thisreader, 76); // cmdinfo
    message.in_sequence = filereader_readint32(thisreader);
    message.out_sequence = filereader_readint32(thisreader);
    message.size_bytes = filereader_readint32(thisreader);
    message.data = NULL;
    filereader_skipbytes(thisreader, message.size_bytes);
    thisptr->m_settings.packet_handler(&message);
  } else {
    filereader_skipbytes(thisreader, 84);
    message.size_bytes = filereader_readint32(thisreader);
    filereader_skipbytes(thisreader, message.size_bytes);
  }
}

void _parse_stop(parser *thisptr) {

  if (thisptr->m_settings.stop_handler) {
    demogobbler_stop message;
    message.remaining_data = NULL;
    message.size_bytes = 0;

    thisptr->m_settings.stop_handler(&message);
  }
}

void _parse_stringtables(parser *thisptr, int32_t type) {
  demogobbler_stringtables message;
  message.preamble.type = type;
  PARSE_PREAMBLE();

  if (thisptr->m_settings.stringtables_handler) {
    message.size_bytes = filereader_readint32(thisreader);
    message.data = NULL;
    filereader_skipbytes(thisreader, message.size_bytes);
    thisptr->m_settings.stringtables_handler(&message);
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
    message.data = NULL;
    filereader_skipbytes(thisreader, message.size_bytes);
    thisptr->m_settings.usercmd_handler(&message);
  } else {
    filereader_skipbytes(thisreader, 4);
    int32_t size = filereader_readint32(thisreader);
    filereader_skipbytes(thisreader, size);
  }
}
