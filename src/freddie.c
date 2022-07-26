#include "freddie.h"
#include "streams.h"
#include "utils.h"
#include <string.h>

struct demogobbler_wip_freddy {
  dynamic_array consolecmd_array;
  dynamic_array cmdinfo_array;
  demogobbler_freddie output;
};

typedef struct demogobbler_wip_freddy wip_freddie;

static void wip_freddie_init(wip_freddie* thisptr) {
  const size_t MIN_ALLOCATION = 4096;

  memset(&thisptr->output, 0, sizeof(demogobbler_freddie));
  dynamic_array_init(&thisptr->cmdinfo_array, MIN_ALLOCATION, sizeof(demogobbler_cmdinfo));
  dynamic_array_init(&thisptr->consolecmd_array, MIN_ALLOCATION, sizeof(freddie_consolecmd));
  dynamic_array_init(&thisptr->output._memory_chunk, MIN_ALLOCATION, 1);
}

static void header_handler(parser_state* _state, demogobbler_header *header) {
  wip_freddie* wip = (wip_freddie*)_state->client_state;
  wip->output.header = *header;
}

static void packet_handler(parser_state* _state, demogobbler_packet *message) {
  wip_freddie* wip = (wip_freddie*)_state->client_state;
  demogobbler_cmdinfo* insert_point = dynamic_array_add(&wip->cmdinfo_array, 1);
  *insert_point = message->cmdinfo[0];
}

static void consolecmd_handler(parser_state* _state, demogobbler_consolecmd *message) {
  wip_freddie* wip = (wip_freddie*)_state->client_state;
  freddie_consolecmd* insert_point = dynamic_array_add(&wip->consolecmd_array, 1);
  insert_point->tick = message->preamble.tick;
  size_t length = message->size_bytes;
  char* dest = dynamic_array_add(&wip->output._memory_chunk, length);
  memcpy(dest, message->data, length);

  // DANGER, the cmd field is now pointing into the bermuda triangle
  insert_point->__offset = dynamic_array_offset(&wip->output._memory_chunk, dest);
}

void demogobbler_freddie_parse_internal(wip_freddie *wip, void *stream, input_interface interface) {
  demogobbler_settings settings;
  demogobbler_settings_init(&settings);
  settings.consolecmd_handler = consolecmd_handler;
  settings.packet_handler = packet_handler;
  settings.header_handler = header_handler;
  settings.client_state = wip;

  demogobbler_parse_result out = demogobbler_parse(&settings, stream, interface);

  wip->output.error = out.error;
  wip->output.error_message = out.error_message;

  // Copy the arrays to the final output
  wip->output.cmdinfo = wip->cmdinfo_array.ptr;
  wip->output.cmdinfo_count = wip->cmdinfo_array.count;
  wip->output.consolecmd = wip->consolecmd_array.ptr;
  wip->output.consolecmd_count = wip->consolecmd_array.count;

  // Fix up the pointers to the common memory chunk
  for(size_t i=0; i < wip->output.consolecmd_count; ++i) {
    const char* str = (const char*)wip->output._memory_chunk.ptr + wip->output.consolecmd[i].__offset;
    freddie_consolecmd* freddie_cmd = &wip->output.consolecmd[i];
    freddie_cmd ->cmd = str;
  }
}

demogobbler_freddie demogobbler_freddie_file(const char *filepath) {
  wip_freddie wip;
  wip_freddie_init(&wip);
  FILE *stream = fopen(filepath, "rb");

  if (stream) {
    demogobbler_freddie_parse_internal(&wip, stream,
                                       (input_interface){fstream_read, fstream_seek});
    fclose(stream);
  } else {
    wip.output.error = true;
    wip.output.error_message = "Unable to open file";
  }

  return wip.output;
}

demogobbler_freddie demogobbler_freddie_buffer(void *buffer, size_t buffer_size) {
  wip_freddie wip;
  wip_freddie_init(&wip);

  if (buffer) {
    buffer_stream stream = {buffer, buffer_size};
    demogobbler_freddie_parse_internal(&wip, &stream,
                                       (input_interface){buffer_stream_read, buffer_stream_seek});
  }

  return wip.output;
}

void demogobbler_freddie_free(demogobbler_freddie *freddie) {
  if (freddie->cmdinfo) {
    free(freddie->cmdinfo);
    freddie->cmdinfo = NULL;
    freddie->cmdinfo_count = 0;
  }

  if (freddie->consolecmd) {
    free(freddie->consolecmd);
    freddie->consolecmd_count = 0;
  }

  dynamic_array_free(&freddie->_memory_chunk);
}
