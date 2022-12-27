#include "demogobbler.h"
#include "demogobbler/freddie.hpp"
#include "demogobbler/utils.h"
#include <stdio.h>
#include <string.h>

using namespace freddie;

struct dg_splicer_state {
  writer _writer;
  size_t demo_index;
  size_t demo_count;
  bool has_seen_positive_tick;
  size_t base_tick;
  size_t demo_highest_tick;
};

typedef struct dg_splicer_state splicer_state;

static void get_tick(splicer_state *state, dg_message_preamble *preamble) {
  if (state->demo_index != 0) {
    preamble->tick = MAX(state->demo_highest_tick, preamble->tick + state->base_tick);
    state->demo_highest_tick = MAX(state->demo_highest_tick, (size_t)preamble->tick);
  }
}

static void handle_consolecmd(parser_state *ptr, dg_consolecmd *message) {
  splicer_state *state = (splicer_state*)ptr->client_state;
  get_tick(state, &message->preamble);
  dg_write_consolecmd(&state->_writer, message);
}

void handle_customdata(parser_state *ptr, dg_customdata *message) {
  splicer_state *state = (splicer_state*)ptr->client_state;
  get_tick(state, &message->preamble);
  dg_write_customdata(&state->_writer, message);
}

void handle_datatables(parser_state *ptr, dg_datatables *message) {
  splicer_state *state = (splicer_state*)ptr->client_state;
  get_tick(state, &message->preamble);
  dg_write_datatables(&state->_writer, message);
}

void handle_demoversion(parser_state *ptr, dg_demver_data message) {
  splicer_state *state = (splicer_state*)ptr->client_state;
  // We only check the demo version data on the first demo
  if (state->demo_index == 0) {
    state->_writer.version = message;
  }
}

void handle_header(parser_state *ptr, dg_header *message) {
  splicer_state *state = (splicer_state*)ptr->client_state;
  if (state->demo_index == 0) {
    dg_write_header(&state->_writer, message);
  }
}

void handle_packet(parser_state *ptr, dg_packet *message) {
  splicer_state *state = (splicer_state*)ptr->client_state;

  if (message->preamble.tick >= 0) {
    state->has_seen_positive_tick = true;
  }

  // We toss any negative packets from the end
  if (!state->has_seen_positive_tick || message->preamble.tick >= 0) {
    get_tick(state, &message->preamble);
    size_t tick = MIN(0, state->base_tick + message->preamble.tick);
    state->demo_highest_tick = MAX(state->demo_highest_tick, tick);
    dg_write_packet(&state->_writer, message);
  }
}

void handle_stringtables(parser_state *ptr, dg_stringtables *message) {
  splicer_state *state = (splicer_state*)ptr->client_state;
  if (state->demo_index == 0) {
    get_tick(state, &message->preamble);
    dg_write_stringtables(&state->_writer, message);
  }
}

void handle_stop(parser_state *ptr, dg_stop *message) {
  splicer_state *state = (splicer_state*)ptr->client_state;
  if (state->demo_index == state->demo_count - 1) {
    dg_write_stop(&state->_writer, message);
  }
}

void handle_usercmd(parser_state *ptr, dg_usercmd *message) {
  splicer_state *state = (splicer_state*)ptr->client_state;
  get_tick(state, &message->preamble);
  dg_write_usercmd(&state->_writer, message);
}

dg_parse_result freddie::splice_demos(const char *output_path, const char **demo_paths,
                                     size_t demo_count) {
  dg_parse_result result;
  splicer_state state;
  memset(&result, 0, sizeof(result));
  memset(&state, 0, sizeof(state));
  dg_writer_open_file(&state._writer, output_path);

  if (state._writer.error)
    goto end;

  state.demo_index = 0;
  state.demo_count = demo_count;

  dg_settings settings;
  dg_settings_init(&settings);

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
    dg_parse_result rval = dg_parse_file(&settings, demo_paths[state.demo_index]);
    printf("Wrote %s\n", demo_paths[state.demo_index]);
    if (rval.error) {
      result.error = true;
      result.error_message = rval.error_message;
      goto end;
    }
  }

end:
  if (state._writer.error) {
    result.error = true;
    result.error_message = state._writer.error_message;
  }

  dg_writer_free(&state._writer);

  return result;
}
