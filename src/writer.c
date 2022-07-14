#include "demogobbler.h"
#include <stdlib.h>
#include <string.h>

void demogobbler_writer_init(writer *thisptr) { memset(thisptr, 0, sizeof(writer)); }

void demogobbler_writer_open_file(writer *thisptr, const char *filepath) {
  thisptr->stream = fopen(filepath, "wb");

  if (!thisptr->stream) {
    thisptr->error_set = true;
    thisptr->last_error_message = "Unable to open file";
  }
}

void demogobbler_writer_close(writer *thisptr) {
  if (thisptr->stream) {
    fclose(thisptr->stream);
    thisptr->stream = NULL;
  }
}

#define WRITE_BYTE(field) fwrite(&message->field, 1, 1, thisptr->stream);
#define WRITE_INT32(field) fwrite(&message->field, 1, 4, thisptr->stream);
#define WRITE_STRING(field, length) fwrite(&message->field, 1, length, thisptr->stream);
#define WRITE_CMDINFO_VEC(field)                                                                   \
  fwrite(&(cmdinfo->field.x), 1, 4, thisptr->stream);                                              \
  fwrite(&(cmdinfo->field.y), 1, 4, thisptr->stream);                                              \
  fwrite(&(cmdinfo->field.z), 1, 4, thisptr->stream);
#define WRITE_PREAMBLE()                                                                           \
  WRITE_BYTE(preamble.type);                                                                       \
  WRITE_INT32(preamble.tick);
#define WRITE_DATA()                                                                               \
  WRITE_INT32(size_bytes);                                                                         \
  fwrite(message->data, 1, message->size_bytes, thisptr->stream)

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
  demogobbler_cmdinfo *cmdinfo = &message->cmdinfo[0];
  fwrite(&cmdinfo->interp_flags, 1, 4, thisptr->stream);
  WRITE_CMDINFO_VEC(view_origin);
  WRITE_CMDINFO_VEC(view_angles);
  WRITE_CMDINFO_VEC(local_viewangles);
  WRITE_CMDINFO_VEC(view_origin2);
  WRITE_CMDINFO_VEC(view_angles2);
  WRITE_CMDINFO_VEC(local_viewangles2);
  WRITE_INT32(in_sequence);
  WRITE_INT32(out_sequence);

  WRITE_DATA();
}

void demogobbler_write_synctick(writer *thisptr, demogobbler_synctick *message) {
  WRITE_PREAMBLE();
}

void demogobbler_write_stop(writer *thisptr, demogobbler_stop *message) {
  enum demogobbler_type t = demogobbler_type_stop;
  fwrite(&t, 1, 1, thisptr->stream);
  fwrite(message->data, 1, message->size_bytes, thisptr->stream);
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

void demogobbler_writer_free(writer *thisptr) {
  if (thisptr->stream) {
    fclose(thisptr->stream);
    thisptr->stream = NULL;
  }
}
