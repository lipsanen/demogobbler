#include "demogobbler.h"
#include "freddie.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

struct demogobbler_splicer_state {
  writer writer;
  size_t demo_index;
  size_t demo_count;
  bool has_seen_positive_tick;
  size_t base_tick;
  size_t demo_highest_tick;
};

typedef struct demogobbler_splicer_state splicer_state;

static void get_tick(splicer_state* state, demogobbler_message_preamble* preamble) {
  if(state->demo_index != 0) {
    preamble->tick = MAX(state->demo_highest_tick, preamble->tick + state->base_tick);
    state->demo_highest_tick = MAX(state->demo_highest_tick, preamble->tick);
  }
}

static void handle_consolecmd(parser_state *ptr, demogobbler_consolecmd *message) {
  splicer_state *state = ptr->client_state;
  get_tick(state, &message->preamble);
  demogobbler_write_consolecmd(&state->writer, message);
}

void handle_customdata(parser_state *ptr, demogobbler_customdata *message) {
  splicer_state *state = ptr->client_state;
  get_tick(state, &message->preamble);
  demogobbler_write_customdata(&state->writer, message);
}

void handle_datatables(parser_state *ptr, demogobbler_datatables *message) {
  splicer_state *state = ptr->client_state;
  get_tick(state, &message->preamble);
  demogobbler_write_datatables(&state->writer, message);
}

void handle_demoversion(parser_state *ptr, demo_version_data message) {
  splicer_state *state = ptr->client_state;
  // We only check the demo version data on the first demo
  if (state->demo_index == 0) {
    state->writer.version = message;
  }
}

void handle_header(parser_state *ptr, demogobbler_header *message) {
  splicer_state *state = ptr->client_state;
  if (state->demo_index == 0) {
    demogobbler_write_header(&state->writer, message);
  }
}

void handle_packet(parser_state *ptr, demogobbler_packet *message) {
  splicer_state *state = ptr->client_state;

  if(message->preamble.tick >= 0) {
    state->has_seen_positive_tick = true;
  }

  // We toss any negative packets from the end
  if(!state->has_seen_positive_tick || message->preamble.tick >= 0) {
    get_tick(state, &message->preamble);
    size_t tick = MIN(0, state->base_tick + message->preamble.tick);
    state->demo_highest_tick = MAX(state->demo_highest_tick, tick);
    demogobbler_write_packet(&state->writer, message);
  }
}

void handle_stringtables(parser_state *ptr, demogobbler_stringtables *message) {
  splicer_state *state = ptr->client_state;
  if(state->demo_index == 0) {
    get_tick(state, &message->preamble);
    demogobbler_write_stringtables(&state->writer, message);
  }
}

void handle_stop(parser_state *ptr, demogobbler_stop *message) {
  splicer_state *state = ptr->client_state;
  if (state->demo_index == state->demo_count - 1) {
    demogobbler_write_stop(&state->writer, message);
  }
}

void handle_usercmd(parser_state *ptr, demogobbler_usercmd *message) {
  splicer_state *state = ptr->client_state;
  get_tick(state, &message->preamble);
  demogobbler_write_usercmd(&state->writer, message);
}

freddie_result demogobbler_freddie_splice_demos(const char *output_path, const char **demo_paths,
                                                size_t demo_count) {
  freddie_result result;
  splicer_state state;
  memset(&result, 0, sizeof(result));
  memset(&state, 0, sizeof(state));
  demogobbler_writer_open_file(&state.writer, output_path);

  if (state.writer.error)
    goto end;

  state.demo_index = 0;
  state.demo_count = demo_count;

  demogobbler_settings settings;
  demogobbler_settings_init(&settings);

  // Synctick is useless and makes demos do a weird skip in the beginning, so it is not written
  // Also it breaks spliced demos
  settings.client_state = &state;
  settings.consolecmd_handler = handle_consolecmd;
  settings.customdata_handler = handle_customdata;
  settings.datatables_handler = handle_datatables;
  settings.demo_version_handler = handle_demoversion;
  settings.header_handler = handle_header;
  settings.packet_handler = handle_packet;
  settings.stop_handler = handle_stop;
  settings.stringtables_handler = handle_stringtables;
  settings.usercmd_handler = handle_usercmd;

  for (state.demo_index = 0; state.demo_index < state.demo_count; ++state.demo_index) {
    state.has_seen_positive_tick = false;
    state.base_tick = state.demo_highest_tick;
    state.demo_highest_tick = 0;
    demogobbler_parse_result rval = demogobbler_parse_file(&settings, demo_paths[state.demo_index]);
    printf("Wrote %s\n", demo_paths[state.demo_index]);
    if (rval.error) {
      result.error = true;
      result.error_message = rval.error_message;
      goto end;
    }
  }

end:
  if (state.writer.error) {
    result.error = true;
    result.error_message = state.writer.error_message;
  }

  demogobbler_writer_free(&state.writer);

  return result;
}
