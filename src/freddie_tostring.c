#include "freddie.h"
#include <stdarg.h>
#include <stdio.h>
#include <threads.h>

static const char* send_string(freddie_consume_string_func func, const char* fmt, ...) {
  static thread_local char BUFFER[1024];
  va_list args;
  va_start (args, fmt);
  vsnprintf (BUFFER, sizeof(BUFFER), fmt, args);
  func((const char*)BUFFER);
  va_end (args);
}

void freddie_consolecmd_tostring(demogobbler_consolecmd* message, freddie_consume_string_func func) {
  send_string(func, "consolecmd: tick %d, slot %d\n", message->preamble.tick, message->preamble.slot);
  send_string(func, "\tcommand: %s\n", message->data);
}

void freddie_customdata_tostring(demogobbler_customdata* message, freddie_consume_string_func func) {
  send_string(func, "customdata: tick %d, slot %d\n", message->preamble.tick, message->preamble.slot);
  send_string(func, "\tdata size: %d\n", message->size_bytes);
}

void freddie_datatables_tostring(demogobbler_datatables* message, freddie_consume_string_func func) {
  send_string(func, "datatables: tick %d, slot %d\n", message->preamble.tick, message->preamble.slot);
  send_string(func, "\tdata size: %d\n", message->size_bytes);
}

void freddie_netmessage_tostring(packet_net_message* message, freddie_consume_string_func func) {

}

void freddie_packet_tostring(demogobbler_packet* message, freddie_consume_string_func func) {
  if(message->preamble.type == demogobbler_type_signon) {
    send_string(func, "signon: tick %d, slot %d\n", message->preamble.tick, message->preamble.slot);
  }
  else {
    send_string(func, "packet: tick %d, slot %d\n", message->preamble.tick, message->preamble.slot);
  }

  for (int i = 0; i < message->cmdinfo_size; ++i) {
    send_string(func, "\tcmdinfo %d:\n", i);
    send_string(func, "\t\tinterp flags %d\n", message->cmdinfo[i].interp_flags);
    send_string(func, "\t\tlocal viewangles (%f, %f, %f)\n", message->cmdinfo[i].local_viewangles.x,
           message->cmdinfo[i].local_viewangles.y, message->cmdinfo[i].local_viewangles.z);
    send_string(func, "\t\tlocal viewangles 2 (%f, %f, %f)\n", message->cmdinfo[i].local_viewangles2.x,
           message->cmdinfo[i].local_viewangles2.y, message->cmdinfo[i].local_viewangles2.z);
    send_string(func, "\t\tview angles (%f, %f, %f)\n", message->cmdinfo[i].view_angles.x,
           message->cmdinfo[i].view_angles.y, message->cmdinfo[i].view_angles.z);
    send_string(func, "\t\tview angles2 (%f, %f, %f)\n", message->cmdinfo[i].view_angles2.x,
           message->cmdinfo[i].view_angles2.y, message->cmdinfo[i].view_angles2.z);
    send_string(func, "\t\torigin (%f, %f, %f)\n", message->cmdinfo[i].view_origin.x,
           message->cmdinfo[i].view_origin.y, message->cmdinfo[i].view_origin.z);
    send_string(func, "\t\torigin 2 (%f, %f, %f)\n", message->cmdinfo[i].view_origin2.x,
           message->cmdinfo[i].view_origin2.y, message->cmdinfo[i].view_origin2.z);
  }

  send_string(func, "\tin/out sequence: %d/%d\n", message->in_sequence, message->out_sequence);
  send_string(func, "\tdata size: %d\n", message->size_bytes);
  send_string(func, "\tmessages:\n");
}
void freddie_stringtables_tostring(demogobbler_stringtables* message, freddie_consume_string_func func) {
  send_string(func, "stringtables: tick %d, slot %d\n", message->preamble.tick, message->preamble.slot);
  send_string(func, "\tdata size: %d\n", message->size_bytes);
}

void freddie_synctick_tostring(demogobbler_synctick* message, freddie_consume_string_func func) {
  send_string(func, "synctick: tick %d, slot %d\n", message->preamble.tick, message->preamble.slot);
}

void freddie_usercmd_tostring(demogobbler_usercmd* message, freddie_consume_string_func func) {
  send_string(func, "usercmd: tick %d, slot %d\n", message->preamble.tick, message->preamble.slot);
  send_string(func, "\tdata size: %d\n", message->size_bytes);
}
