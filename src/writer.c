#include "demogobbler.h"
#include "demogobbler/bitstream.h"
#include "demogobbler/bitwriter.h"
#include "demogobbler/streams.h"
#include "utils.h"
#include "version_utils.h"
#include <stdlib.h>
#include <string.h>

void dg_writer_init(writer *thisptr) {
  memset(thisptr, 0, sizeof(writer));
  dg_bitwriter_init(&thisptr->bitwriter, 32768);
}

void dg_writer_open_file(writer *thisptr, const char *filepath) {
  thisptr->_stream = fopen(filepath, "wb");
  thisptr->output_funcs = (output_interface){dg_fstream_write};
  thisptr->_custom_stream = false;

  if (!thisptr->_stream) {
    thisptr->error = true;
    thisptr->error_message = "Unable to open file";
  }
}
void dg_writer_open(writer *thisptr, void *stream, output_interface output_interface) {
  thisptr->_stream = stream;
  thisptr->output_funcs = output_interface;
  thisptr->_custom_stream = true;
}

void dg_writer_close(writer *thisptr) {
  dg_bitwriter_free(&thisptr->bitwriter);
  if (!thisptr->_custom_stream && thisptr->_stream) {
    fclose(thisptr->_stream);
    thisptr->_stream = NULL;
  }
}

#define WRITE_BYTE(field) thisptr->output_funcs.write(thisptr->_stream, &message->field, 1);
#define WRITE_INT32(field) thisptr->output_funcs.write(thisptr->_stream, &message->field, 4);
#define WRITE_STRING(field, length)                                                                \
  thisptr->output_funcs.write(thisptr->_stream, &message->field, length);
#define WRITE_CMDINFO_VEC(field)                                                                   \
  thisptr->output_funcs.write(thisptr->_stream, &(cmdinfo->field.x), 4);                           \
  thisptr->output_funcs.write(thisptr->_stream, &(cmdinfo->field.y), 4);                           \
  thisptr->output_funcs.write(thisptr->_stream, &(cmdinfo->field.z), 4);
#define WRITE_PREAMBLE()                                                                           \
  WRITE_BYTE(preamble.type);                                                                       \
  WRITE_INT32(preamble.tick);                                                                      \
  if (thisptr->version.has_slot_in_preamble)                                                       \
    WRITE_BYTE(preamble.slot);

#define WRITE_DATA()                                                                               \
  WRITE_INT32(size_bytes);                                                                         \
  if (message->size_bytes > 0)                                                                     \
    thisptr->output_funcs.write(thisptr->_stream, message->data, message->size_bytes);

void dg_write_preamble(writer *thisptr, dg_message_preamble preamble) {
  thisptr->output_funcs.write(thisptr->_stream, &preamble.type, 1);
  thisptr->output_funcs.write(thisptr->_stream, &preamble.tick, 4);
  if (thisptr->version.has_slot_in_preamble) {
    thisptr->output_funcs.write(thisptr->_stream, &preamble.slot, 1);
  }
}

void dg_write_consolecmd(writer *thisptr, dg_consolecmd *message) {
  WRITE_PREAMBLE();
  WRITE_DATA();
}

void dg_write_customdata(writer *thisptr, dg_customdata *message) {
  WRITE_PREAMBLE();
  WRITE_INT32(unknown);
  WRITE_DATA();
}

void dg_write_datatables(writer *thisptr, dg_datatables *message) {
  WRITE_PREAMBLE();
  WRITE_DATA();
}

void dg_write_header(writer *thisptr, dg_header *message) {
  WRITE_STRING(ID, 8);
  WRITE_INT32(demo_protocol);
  WRITE_INT32(net_protocol);
  WRITE_STRING(server_name, 260);
  WRITE_STRING(client_name, 260);
  WRITE_STRING(map_name, 260);
  WRITE_STRING(game_directory, 260);
  WRITE_INT32(seconds);
  WRITE_INT32(tick_count);
  WRITE_INT32(frame_count);
  WRITE_INT32(signon_length);
}

void dg_write_packet(writer *thisptr, dg_packet *message) {
  WRITE_PREAMBLE();

  for (int i = 0; i < thisptr->version.cmdinfo_size; ++i) {
    dg_cmdinfo *cmdinfo = &message->cmdinfo[i];
    thisptr->output_funcs.write(thisptr->_stream, &cmdinfo->interp_flags, 4);

    WRITE_CMDINFO_VEC(view_origin);
    WRITE_CMDINFO_VEC(view_angles);
    WRITE_CMDINFO_VEC(local_viewangles);
    WRITE_CMDINFO_VEC(view_origin2);
    WRITE_CMDINFO_VEC(view_angles2);
    WRITE_CMDINFO_VEC(local_viewangles2);
  }

  WRITE_INT32(in_sequence);
  WRITE_INT32(out_sequence);
  WRITE_DATA();
}

void dg_write_packet_parsed(writer *thisptr, packet_parsed *message_parsed) {
  dg_packet *message = &message_parsed->orig;
  thisptr->bitwriter.bitoffset = 0;
  WRITE_BYTE(preamble.type);
  WRITE_INT32(preamble.tick);
  if (thisptr->version.has_slot_in_preamble)
    WRITE_BYTE(preamble.slot);

  for (int i = 0; i < thisptr->version.cmdinfo_size; ++i) {
    dg_cmdinfo *cmdinfo = &message->cmdinfo[i];
    thisptr->output_funcs.write(thisptr->_stream, &cmdinfo->interp_flags, 4);

    WRITE_CMDINFO_VEC(view_origin);
    WRITE_CMDINFO_VEC(view_angles);
    WRITE_CMDINFO_VEC(local_viewangles);
    WRITE_CMDINFO_VEC(view_origin2);
    WRITE_CMDINFO_VEC(view_angles2);
    WRITE_CMDINFO_VEC(local_viewangles2);
  }

  WRITE_INT32(in_sequence);
  WRITE_INT32(out_sequence);

#ifdef GROUND_TRUTH_CHECK
  if(thisptr->expect_equal) {
    thisptr->bitwriter.truth_data = message_parsed->orig.data;
    thisptr->bitwriter.truth_data_offset = 0;
    thisptr->bitwriter.truth_size_bits = message_parsed->orig.size_bytes * 8;
  }
#endif

  for (uint32_t i = 0; i < message_parsed->message_count; ++i) {
    packet_net_message *netmsg = message_parsed->messages + i;
    dg_bitwriter_write_netmessage(&thisptr->bitwriter, &thisptr->version, netmsg);
  }

  if (thisptr->bitwriter.bitoffset % 8 != 0) {
    uint32_t expected_bits =
        thisptr->bitwriter.bitoffset + dg_bitstream_bits_left(&message_parsed->leftover_bits);
    if (expected_bits == message->size_bytes * 8) {
      // grab leftover bits from original stream if number of bytes matches
      dg_bitwriter_write_bitstream(&thisptr->bitwriter, &message_parsed->leftover_bits);
    } else {
      // otherwise write 0 for the last bits
      dg_bitwriter_write_uint(&thisptr->bitwriter, 0, 8 - thisptr->bitwriter.bitoffset % 8);
    }
  }

  uint32_t bytes = thisptr->bitwriter.bitoffset / 8;
  thisptr->output_funcs.write(thisptr->_stream, &bytes, 4);
  thisptr->output_funcs.write(thisptr->_stream, thisptr->bitwriter.ptr, bytes);
#ifdef GROUND_TRUTH_CHECK
  if(thisptr->expect_equal) {
      thisptr->bitwriter.truth_data = NULL;
  }
#endif
}

void dg_write_synctick(writer *thisptr, dg_synctick *message) { WRITE_PREAMBLE(); }

void dg_write_stop(writer *thisptr, dg_stop *message) {
  enum dg_type t = dg_type_stop;
  thisptr->output_funcs.write(thisptr->_stream, &t, 1);
  thisptr->output_funcs.write(thisptr->_stream, message->data, message->size_bytes);
}

void dg_write_stringtables(writer *thisptr, dg_stringtables *message) {
  WRITE_PREAMBLE();
  WRITE_DATA();
}

void dg_write_usercmd(writer *thisptr, dg_usercmd *message) {
  WRITE_PREAMBLE();
  WRITE_INT32(cmd);
  WRITE_DATA();
}

void dg_writer_free(writer *thisptr) {
  dg_writer_close(thisptr);
  dg_bitwriter_free(&thisptr->bitwriter);
}

void dg_write_byte(writer *thisptr, uint8_t value) {
  thisptr->output_funcs.write(thisptr->_stream, &value, 1);
}

void dg_write_short(writer *thisptr, uint16_t value) {
  thisptr->output_funcs.write(thisptr->_stream, &value, 2);
}

void dg_write_int32(writer *thisptr, int32_t value) {
  thisptr->output_funcs.write(thisptr->_stream, &value, 4);
}

void dg_write_data(writer *thisptr, void *src, uint32_t bytes) {
  thisptr->output_funcs.write(thisptr->_stream, src, bytes);
}

void dg_write_string(writer *thisptr, const char *str) {
  while (*str) {
    thisptr->output_funcs.write(thisptr->_stream, str, 1);
    ++str;
  }
  thisptr->output_funcs.write(thisptr->_stream, str, 1);
}
