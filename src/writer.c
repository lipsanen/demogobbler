#include "demogobbler.h"
#include "streams.h"
#include "utils.h"
#include "version_utils.h"
#include <stdlib.h>
#include <string.h>

void demogobbler_writer_init(writer *thisptr) { memset(thisptr, 0, sizeof(writer)); }

void demogobbler_writer_open_file(writer *thisptr, const char *filepath) {
  thisptr->_stream = fopen(filepath, "wb");
  thisptr->output_funcs = (output_interface){fstream_write};
  thisptr->_custom_stream = false;

  if (!thisptr->_stream) {
    thisptr->error_set = true;
    thisptr->error_message = "Unable to open file";
  }
}
void demogobbler_writer_open(writer *thisptr, void *stream, output_interface output_interface) {
  thisptr->_stream = stream;
  thisptr->output_funcs = output_interface;
  thisptr->_custom_stream = true;
}

void demogobbler_writer_close(writer *thisptr) {
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
  WRITE_INT32(preamble.tick); \
  if(version_has_slot_in_preamble(thisptr->version)) WRITE_BYTE(preamble.slot);

#define WRITE_DATA()                                                                               \
  WRITE_INT32(size_bytes);                                                                         \
  thisptr->output_funcs.write(thisptr->_stream, message->data, message->size_bytes)

void demogobbler_write_consolecmd(writer *thisptr, demogobbler_consolecmd *message) {
  WRITE_PREAMBLE();
  WRITE_DATA();
}

void demogobbler_write_customdata(writer *thisptr, demogobbler_customdata *message) {
  WRITE_PREAMBLE();
  WRITE_INT32(unknown);
  WRITE_DATA();
}

void demogobbler_write_datatables(writer *thisptr, demogobbler_datatables *message) {
  WRITE_PREAMBLE();
  WRITE_DATA();
}

void demogobbler_write_header(writer *thisptr, demogobbler_header *message) {
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

void demogobbler_write_packet(writer *thisptr, demogobbler_packet *message) {
  WRITE_PREAMBLE();

  for (int i = 0; i < version_cmdinfo_size(thisptr->version); ++i) {
    demogobbler_cmdinfo *cmdinfo = &message->cmdinfo[i];
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

void demogobbler_write_synctick(writer *thisptr, demogobbler_synctick *message) {
  WRITE_PREAMBLE();
}

void demogobbler_write_stop(writer *thisptr, demogobbler_stop *message) {
  enum demogobbler_type t = demogobbler_type_stop;
  thisptr->output_funcs.write(thisptr->_stream, &t, 1);
  thisptr->output_funcs.write(thisptr->_stream, message->data, message->size_bytes);
}

void demogobbler_write_stringtables(writer *thisptr, demogobbler_stringtables *message) {
  WRITE_PREAMBLE();
  WRITE_DATA();
}

void demogobbler_write_usercmd(writer *thisptr, demogobbler_usercmd *message) {
  WRITE_PREAMBLE();
  WRITE_INT32(cmd);
  WRITE_DATA();
}

void demogobbler_writer_free(writer *thisptr) { demogobbler_writer_close(thisptr); }
