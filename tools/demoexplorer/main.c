#include "freddie.h"
#include <ncurses.h>
#include <string.h>

static int lines = 0;
static int line_index = 0;
static int scroll_index = 0;
static WINDOW *pad = NULL;

struct message_view_state {
  uint32_t line;
  bool expanded : 1;
  bool internal_expanded : 1;
};

typedef struct message_view_state message_view_state;

static message_view_state *messages_state = NULL;
static freddie_demo demo;
static bool header_expanded = false;

static void count_lines(const char *fmt, ...) { ++lines; }

static void print_line(const char *fmt, ...) {
  char BUFFER[1024];
  va_list args;
  va_start(args, fmt);
  vsnprintf(BUFFER, sizeof(BUFFER), fmt, args);
  mvwprintw(pad, line_index, 0, (const char *)BUFFER);
  va_end(args);
  ++line_index;
}

typedef void (*consume_string_func)(const char *str, ...);

static void header_title(demogobbler_header *header, consume_string_func func) {
  if (header_expanded)
    func("- Header");
  else
    func("+ Header");
}

void header_internals(demogobbler_header *header, consume_string_func func) {
  func("  ID: %s", header->ID);
  func("  Demo protocol: %d", header->demo_protocol);
  func("  Net protocol: %d", header->net_protocol);
  func("  Server name: %s", header->server_name);
  func("  Client name: %s", header->client_name);
  func("  Map name: %s", header->map_name);
  func("  Game directory: %s", header->game_directory);
  func("  Seconds: %f", header->seconds);
  func("  Tick: %d", header->tick_count);
  func("  Frames: %d", header->frame_count);
  func("  Signon length: %d", header->frame_count);
}

void message_title(size_t i, consume_string_func func) {
  freddie_demo_message *message = demo.messages + i;
  bool expanded = messages_state[i].expanded;
  char c = expanded ? '-' : '+';
  switch (message->mtype) {
  case demogobbler_type_consolecmd:
    func("%c Consolecmd tick %d, slot %d", c, message->consolecmd.preamble.tick,
         message->consolecmd.preamble.slot);
    break;
  case demogobbler_type_customdata:
    func("%c Customdata tick %d, slot %d", c, message->customdata.preamble.tick,
         message->customdata.preamble.slot);
    break;
  case demogobbler_type_datatables:
    func("%c Datatables tick %d, slot %d", c, message->datatables.preamble.tick,
         message->customdata.preamble.slot);
    break;
  case demogobbler_type_packet:
    func("%c Packet tick %d, slot %d", c, message->packet.preamble.tick,
         message->packet.preamble.slot);
    break;
  case demogobbler_type_signon:
    func("%c Signon tick %d, slot %d", c, message->packet.preamble.tick,
         message->packet.preamble.slot);
    break;
  case demogobbler_type_stop:
    func("Stop");
    break;
  case demogobbler_type_stringtables:
    func("%c Stringtables tick %d, slot %d", c, message->stringtables.preamble.tick,
         message->stringtables.preamble.slot);
    break;
  case demogobbler_type_synctick:
    func("%c Synctick tick %d, slot %d", c, message->synctick.preamble.tick,
         message->synctick.preamble.slot);
    break;
  case demogobbler_type_usercmd:
    func("%c Usercmd tick %d, slot %d", c, message->usercmd.preamble.tick,
         message->usercmd.preamble.slot);
    break;
  default:
    break;
  }
}

static void netmessage_print(packet_net_message *message, consume_string_func func) {
  switch(message->mtype) {
    case net_nop:
      func("    net_nop");
      break;
    case net_disconnect:
      func("    net_disconnect");
      func("      reason: %s", message->message_net_disconnect.text);
      break;
    case net_file:
      func("    net_file");
      func("      filename: %s", message->message_net_file.filename);
      func("      file requested: %d", message->message_net_file.file_requested);
      func("      transfer id: %d", message->message_net_file.transfer_id);
      break;
    case net_tick:
      func("    net_tick");
      func("      tick: %d", message->message_net_tick.tick);
      func("      frametime: %d", message->message_net_tick.host_frame_time);
      func("      frametime dev %d", message->message_net_tick.host_frame_time_std_dev);
      break;
    case net_stringcmd:
      func("    net_stringcmd");
      func("      command: %s", message->message_net_stringcmd.command);
      break;
    case net_setconvar:
      func("    net_setconvar");
      func("      convars set: %d", message->message_net_setconvar.count);
      break;
    case net_signonstate:
      func("    net_signonstate");
      func("      signon state: %d", message->message_net_signonstate.signon_state);
      func("      spawn count: %d", message->message_net_signonstate.spawn_count);
      break;
    default:
      func("    message %d", message->mtype);
      func("      dump not implemented");
      break;
  }
}

void message_internals(size_t i, consume_string_func func) {
  freddie_demo_message *message = demo.messages + i;
  switch (message->mtype) {
  case demogobbler_type_consolecmd:
    func("  Cmd: %s", message->consolecmd.data);
    break;
  case demogobbler_type_customdata:
    func("  Bytes: %d", message->customdata.size_bytes);
    break;
  case demogobbler_type_datatables:
    func("  Bytes: %d", message->datatables.size_bytes);
    break;
  case demogobbler_type_packet:
  case demogobbler_type_signon:
    for (size_t i = 0; i < message->packet.cmdinfo_size; ++i) {
      func("  CmdInfo[%lu]", i);
      func("    Interp flags: (%d)", message->packet.cmdinfo[i].interp_flags);
      func("    Vieworigin: (%f, %f, %f)", message->packet.cmdinfo[i].view_origin.x,
           message->packet.cmdinfo[i].view_origin.y, message->packet.cmdinfo[i].view_origin.z);
      func("    Viewangles: (%f, %f, %f)", message->packet.cmdinfo[i].view_angles.x,
           message->packet.cmdinfo[i].view_angles.y, message->packet.cmdinfo[i].view_angles.z);
      func("    Local viewangles: (%f, %f, %f)", message->packet.cmdinfo[i].local_viewangles.x,
           message->packet.cmdinfo[i].local_viewangles.y, message->packet.cmdinfo[i].local_viewangles.z);
      func("    Vieworigin 2: (%f, %f, %f)", message->packet.cmdinfo[i].view_origin2.x,
           message->packet.cmdinfo[i].view_origin2.y, message->packet.cmdinfo[i].view_origin2.z);
      func("    Viewangles 2: (%f, %f, %f)", message->packet.cmdinfo[i].view_angles2.x,
           message->packet.cmdinfo[i].view_angles2.y, message->packet.cmdinfo[i].view_angles2.z);
      func("    Local viewangles 2: (%f, %f, %f)", message->packet.cmdinfo[i].local_viewangles2.x,
           message->packet.cmdinfo[i].local_viewangles2.y, message->packet.cmdinfo[i].local_viewangles2.z);
    }

    func("  Net messages:");
    for(size_t i=0; i < message->packet.net_messages_count; ++i) {
      netmessage_print(message->packet.net_messages + i, func);
    }

    break;
  case demogobbler_type_stop:
    func("Stop");
    break;
  case demogobbler_type_stringtables:
    func("  Bytes: %d", message->stringtables.size_bytes);
    break;
  case demogobbler_type_synctick:
    break;
  case demogobbler_type_usercmd:
    func("  Cmd: %d", message->usercmd.cmd);
    func("  Bytes: %d", message->usercmd.size_bytes);
    break;
  default:
    break;
  }
}

static void do_printing(freddie_demo *demo, consume_string_func func) {
  header_title(&demo->header, func);
  if (header_expanded) {
    header_internals(&demo->header, func);
  }

  for (size_t i = 0; i < demo->message_count; ++i) {
    messages_state[i].line = line_index;
    message_title(i, func);
    if (messages_state[i].expanded) {
      message_internals(i, func);
    }
  }
}

static void update_view() {
  lines = 0;
  do_printing(&demo, count_lines);
  wresize(pad, lines, COLS);
  werase(pad);
  keypad(pad, TRUE);
  line_index = 0;
  do_printing(&demo, print_line);
  prefresh(pad, scroll_index, 0, 0, 0, LINES - 1, COLS);
}

static void handle_mouse1(MEVENT event) {
  bool changed = false;
  int line_clicked = scroll_index + event.y;
  if (line_clicked == 0) {
    header_expanded = !header_expanded;
    changed = true;
  } else {
    // The line is at minimum pointing to message line - 1
    // This is the case if none of the messages are expanded
    // Do simple linear search to find the index that matches
    for (int i = line_clicked - 1; i >= 0; --i) {
      if (messages_state[i].line == line_clicked) {
        messages_state[i].expanded = !messages_state[i].expanded;
        changed = true;
        break;
      }
      else if(messages_state[i].line < line_clicked) {
        break;
      }
    }
  }
  if (changed)
    update_view();
}

int main(int argc, char **argv) {
  initscr();
  clear();
  noecho();
  cbreak(); // Line buffering disabled. pass on everything
  keypad(stdscr, TRUE);
  curs_set(0);
  demogobbler_parse_result result = freddie_parse_file(argv[1], &demo);

  if (result.error) {
    printf("Error parsing demo: %s\n", result.error_message);
    return 1;
  }

  messages_state = malloc(demo.message_count * sizeof(message_view_state));
  memset(messages_state, 0, demo.message_count * sizeof(message_view_state));

  pad = newpad(LINES, COLS);
  update_view();

  MEVENT event;
  mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
  mmask_t mouse1_mask =
      BUTTON1_CLICKED | BUTTON1_PRESSED | BUTTON1_DOUBLE_CLICKED | BUTTON1_TRIPLE_CLICKED;
  mmask_t mwheelup_mask = BUTTON4_PRESSED;
  mmask_t mwheeldown_mask = BUTTON5_PRESSED;

  while (1) {
    wgetch(pad);

    if (getmouse(&event) == OK) {
      if (event.bstate & (mouse1_mask)) {
        handle_mouse1(event);
      } else if (event.bstate & mwheelup_mask) {
        scroll_index -= 1;
        scroll_index = scroll_index < 0 ? 0 : scroll_index;
        prefresh(pad, scroll_index, 0, 0, 0, LINES - 1, COLS);
      } else if (event.bstate & mwheeldown_mask) {
        scroll_index += 1;
        scroll_index = scroll_index >= lines ? lines : scroll_index;
        prefresh(pad, scroll_index, 0, 0, 0, LINES - 1, COLS);
      }
    }
  }

  delwin(pad);
  endwin();
  return 0;
}
