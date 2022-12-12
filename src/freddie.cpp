#include "demogobbler/freddie.hpp"
#include "demogobbler/streams.h"
#include <cstdint>
#include <cstring>
#include <iostream>

using namespace freddie;

typedef void *(*func_dg_alloc)(void *allocator, uint32_t size, uint32_t alignment);
typedef void *(*func_dg_realloc)(void *allocator, void *ptr, uint32_t prev_size, uint32_t size,
                                 uint32_t alignment);
typedef void (*func_dg_clear)(void *allocator);
typedef void (*func_dg_free)(void *allocator);
typedef void (*func_dg_attach)(void *allocator, void *ptr, uint32_t size);

static void *mallocator_allocate(void *alloc, uint32_t size, uint32_t alignment) {
  mallocator *arena = (mallocator *)alloc;
  void *ptr = malloc(size);
  arena->pointers.push_back(ptr);
  return ptr;
}

static void *mallocator_reallocate(void *allocator, void *ptr, uint32_t prev_size, uint32_t size,
                                   uint32_t alignment) {
  mallocator *arena = (mallocator *)allocator;
  for (size_t i = 0; i < arena->pointers.size(); ++i) {
    if (ptr == arena->pointers[i]) {
      return arena->pointers[i] = realloc(ptr, size);
    }
  }

  void *rval = malloc(size);
  arena->pointers.push_back(rval);
  return rval;
}

static void mallocator_clear(void *allocator) {
  mallocator *arena = (mallocator *)allocator;
  for (auto ptr : arena->pointers)
    ::free(ptr);
  arena->pointers.clear();
}

static void mallocator_attach(void *allocator, void *ptr, uint32_t size) {
  mallocator *arena = (mallocator *)allocator;
  arena->pointers.push_back(ptr);
}

mallocator::mallocator() {}

mallocator::~mallocator() {
  for (auto ptr : pointers)
    ::free(ptr);
}

void mallocator::release() { pointers.clear(); }

void *mallocator::alloc(uint32_t size) { return malloc(size); }

void mallocator::attach(void *ptr) { pointers.push_back(ptr); }

void mallocator::free(void *ptr) {
  pointers.erase(std::remove(pointers.begin(), pointers.end(), ptr), pointers.end());
  ::free(ptr);
}

demo_t::demo_t() {
  const size_t INITIAL_ARENA_SIZE = 1 << 20;
  arena = dg_arena_create(INITIAL_ARENA_SIZE);
}

demo_t::~demo_t() { dg_arena_free(&arena); }

static void handle_header(parser_state *_state, struct dg_header *header) {
  demo_t *state = (demo_t *)_state->client_state;
  state->header = *header;
}

static void handle_version(parser_state *_state, dg_demver_data data) {
  demo_t *state = (demo_t *)_state->client_state;
  state->demver_data = data;
}

#define HANDLE_PACKET(type)                                                                        \
  static void handle_##type(parser_state *_state, type *packet) {                                  \
    demo_t *state = (demo_t *)_state->client_state;                                                \
    auto copy = state->copy_packet(packet);                                                        \
    state->packets.push_back(copy);                                                                \
  }

HANDLE_PACKET(dg_consolecmd);
HANDLE_PACKET(dg_customdata);
HANDLE_PACKET(dg_datatables_parsed);
HANDLE_PACKET(dg_stop);
HANDLE_PACKET(dg_stringtables_parsed);
HANDLE_PACKET(dg_synctick);
HANDLE_PACKET(dg_usercmd);
HANDLE_PACKET(packet_parsed);

static void noop(void *allocator) {}

dg_parse_result demo_t::parse_demo(demo_t *output, void *stream, dg_input_interface interface) {
  dg_settings settings;
  dg_settings_init(&settings);
  settings.temp_alloc_state.allocator = &output->temp_allocator;
  settings.temp_alloc_state.alloc = mallocator_allocate;
  settings.temp_alloc_state.attach = mallocator_attach;
  settings.temp_alloc_state.clear = mallocator_clear;
  settings.temp_alloc_state.realloc = mallocator_reallocate;
  settings.permanent_alloc_state.allocator = &output->arena;
  settings.permanent_alloc_state.clear = noop;
  settings.client_state = output;
  settings.header_handler = handle_header;
  settings.demo_version_handler = handle_version;
  settings.stop_handler = handle_dg_stop;
  settings.consolecmd_handler = handle_dg_consolecmd;
  settings.customdata_handler = handle_dg_customdata;
  settings.datatables_parsed_handler = handle_dg_datatables_parsed;
  settings.stringtables_parsed_handler = handle_dg_stringtables_parsed;
  settings.packet_parsed_handler = handle_packet_parsed;
  settings.stop_handler = handle_dg_stop;
  settings.synctick_handler = handle_dg_synctick;
  settings.usercmd_handler = handle_dg_usercmd;
  settings.parse_packetentities = true;

  return dg_parse(&settings, stream, interface);
}

dg_parse_result demo_t::parse_demo(demo_t *output, const char *filepath) {
  FILE *file = fopen(filepath, "rb");
  dg_parse_result result;

  if (file == nullptr) {
    result.error = true;
    result.error_message = "unable to open file";
  } else {
    result = parse_demo(output, file, {dg_fstream_read, dg_fstream_seek});
    fclose(file);
  }

  return result;
}

dg_parse_result demo_t::write_demo(void *stream, dg_output_interface interface) {
  dg_parse_result result;
  std::memset(&result, 0, sizeof(result));
  dg_writer writer;
  dg_writer_init(&writer);
  writer.version = demver_data;
  dg_writer_open(&writer, stream, interface);
  dg_write_header(&writer, &header);

  for (size_t i = 0; i < packets.size(); ++i) {
    auto* packet = &packets[i]->packet;
    packet_parsed* packet_ptr = std::get_if<packet_parsed>(packet);
    dg_datatables_parsed *dt_ptr = std::get_if<dg_datatables_parsed>(packet);
    dg_stringtables_parsed *st_ptr = std::get_if<dg_stringtables_parsed>(packet);
    dg_consolecmd *cmd_ptr = std::get_if<dg_consolecmd>(packet);
    dg_usercmd *user_ptr = std::get_if<dg_usercmd>(packet);
    dg_stop *stop_ptr = std::get_if<dg_stop>(packet);
    dg_synctick *sync_ptr = std::get_if<dg_synctick>(packet);
    dg_customdata *custom_ptr = std::get_if<dg_customdata>(packet);


    if (packet_ptr) {
      dg_write_packet_parsed(&writer, packet_ptr);
    } else if (dt_ptr) {
      dg_write_datatables_parsed(&writer, dt_ptr);
    } else if (st_ptr) {
      dg_write_stringtables_parsed(&writer, st_ptr);
    } else if (cmd_ptr) {
      dg_write_consolecmd(&writer, cmd_ptr);
    } else if (user_ptr) {
      dg_write_usercmd(&writer, user_ptr);
    } else if (stop_ptr) {
      dg_write_stop(&writer, stop_ptr);
    } else if (sync_ptr) {
      dg_write_synctick(&writer, sync_ptr);
    } else if (custom_ptr) {
      dg_write_customdata(&writer, custom_ptr);
    } else {
      result.error = true;
      result.error_message = "unknown demo packet";
      break;
    }
  }

  dg_writer_close(&writer);

  return result;
}

dg_parse_result demo_t::write_demo(const char *filepath) {
  FILE *file = fopen(filepath, "wb");
  dg_parse_result result;

  if (file == nullptr) {
    result.error = true;
    result.error_message = "unable to open file";
  } else {
    result = write_demo(file, {dg_fstream_write});
    fclose(file);
  }

  return result;
}

static void fix_svc_serverinfo(const char *gamedir, demo_t *demo) {
  for (size_t i = 0; i < demo->packets.size(); ++i) {
    auto packet = demo->packets[i]->packet;
    if (std::holds_alternative<packet_parsed>(packet)) {
      packet_parsed *ptr = std::get_if<packet_parsed>(&packet);
      for (size_t msg_index = 0; msg_index < ptr->message_count; ++msg_index) {
        packet_net_message *msg = ptr->messages + msg_index;
        if (msg->mtype == svc_serverinfo) {
          size_t len = strlen(gamedir);
          char *dest = (char *)dg_arena_allocate(&demo->arena, len + 1, 1);
          memcpy(dest, gamedir, len + 1);
          msg->message_svc_serverinfo->game_dir = dest;
          return;
        }
      }
    }
  }
}

static dg_bitstream get_start_state(const dg_bitwriter *writer) {
  dg_bitstream stream;
  memset(&stream, 0, sizeof(stream));
  stream.data = writer->ptr;
  stream.bitoffset = writer->bitoffset;
  return stream;
}

static void finalize_stream(dg_bitstream *stream, const dg_bitwriter *writer) {
  stream->bitsize = writer->bitoffset;
}

static void fix_packets(demo_t *demo) {
  for (size_t i = 0; i < demo->packets.size(); ++i) {
    auto* packet = &demo->packets[i]->packet;
    packet_parsed* ptr = std::get_if<packet_parsed>(packet);

    if (ptr) {
      for (size_t msg_index = 0; msg_index < ptr->message_count; ++msg_index) {
        packet_net_message *msg = ptr->messages + msg_index;

        if (msg->mtype == svc_packet_entities) {
          write_packetentities_args args;
          args.is_delta = msg->message_svc_packet_entities.is_delta;
          args.version = &demo->demver_data;
          args.data = &msg->message_svc_packet_entities.parsed->data;
          dg_bitwriter bitwriter;
          dg_bitwriter_init(&bitwriter,
                            dg_bitstream_bits_left(&msg->message_svc_packet_entities.data));
          auto stream = get_start_state(&bitwriter);
#ifdef GROUND_TRUTH_CHECK
          bitwriter.truth_data = msg->message_svc_packet_entities.data.data;
          bitwriter.truth_data_offset = msg->message_svc_packet_entities.data.bitoffset;
          bitwriter.truth_size_bits = msg->message_svc_packet_entities.data.bitsize;
#endif
          dg_bitwriter_write_packetentities(&bitwriter, args);
          finalize_stream(&stream, &bitwriter);
          demo->packets[i]->memory.attach(
              bitwriter.ptr); // transfer the bitwriter memory over to the packet
          msg->message_svc_packet_entities.data = stream;
        }
      }
    }
  }
}

dg_parse_result freddie::convert_demo(const demo_t *example, demo_t *demo) {
  dg_parse_result result;
  memset(&result, 0, sizeof(result));

  demo->demver_data = example->demver_data;
  demo->header.net_protocol = example->header.net_protocol;
  demo->header.demo_protocol = example->header.demo_protocol;
  memcpy(demo->header.game_directory, example->header.game_directory, 260);
  fix_svc_serverinfo(example->header.game_directory, demo);
  fix_packets(demo);

  return result;
}

static dg_datatables_parsed *get_datatables(const demo_t *demo) {
  for (size_t i = 0; i < demo->packets.size(); ++i) {
    auto* packet = &demo->packets[i]->packet;
    dg_datatables_parsed *dt_ptr = std::get_if<dg_datatables_parsed>(packet);
    if (dt_ptr) {
      return dt_ptr;
    }
  }
  return nullptr;
}

static int32_t get_dt(const estate* state, const char* name, int32_t initial_guess) {
  if((int32_t)state->serverclass_count > initial_guess && strcmp(state->class_datas[initial_guess].dt_name, name) == 0) {
    return initial_guess;
  }
  else {
    for(int32_t i=0; i < (int32_t)state->serverclass_count; ++i)
    {
      if(strcmp(name, state->class_datas[i].dt_name) == 0) {
        return i;
      }
    }
    return -1;
  }
}

#define ARGSPRINT(...) if(args->func) args->func(__VA_ARGS__)

static void compare_sendtables(const compare_props_args* args, const estate* lhs, const estate* rhs) {
  for(int32_t i=0; i < (int32_t)lhs->serverclass_count; ++i)
  {
    const char* name = lhs->class_datas[i].dt_name;
    auto dt = get_dt(rhs, name, i);
    if(dt == -1)
    {
      ARGSPRINT(true, "[%d] %s only\n", i, name);
    }
    else if(dt != i)
    {
      ARGSPRINT(true, "[%d] %s index changed -> %d\n", i, name, dt);
    }
    else
    {
      ARGSPRINT(true, "[%d] %s matches\n", i, name);
    }
  }

  for(int32_t i=0; i < (int32_t)rhs->serverclass_count; ++i)
  {
    const char* name = rhs->class_datas[i].dt_name;
    auto dt = get_dt(lhs, name, i);
    if(dt == -1)
    {
      ARGSPRINT(false, "[%d] %s only\n", i, name);
    }
  }
}

dg_parse_result freddie::compare_props(const compare_props_args* args) {
  dg_parse_result result;
  memset(&result, 0, sizeof(result));
  dg_arena arena = dg_arena_create(1 << 10);
  estate estate1;
  memset(&estate1, 0, sizeof(estate1));
  estate estate2;
  memset(&estate2, 0, sizeof(estate2));

  dg_alloc_state allocator = {&arena, (func_dg_alloc)dg_arena_allocate,
                              (func_dg_clear)dg_arena_clear, (func_dg_realloc)dg_arena_reallocate,
                              (func_dg_attach)dg_arena_attach};
  dg_datatables_parsed* datatable1 = get_datatables(args->first);
  dg_datatables_parsed* datatable2 = get_datatables(args->second);

  if(datatable1 == nullptr)
  {
    result.error = true;
    result.error_message = "missing datatable";
    ARGSPRINT(true, "missing datatable");
    goto end;
  }

  if(datatable2 == nullptr)
  {
    result.error = true;
    result.error_message = "missing datatable";
    ARGSPRINT(false, "missing datatable");
    goto end;
  }

  estate_init_args args1;
  estate_init_args args2;
  args2.allocator = args1.allocator = &allocator;
  args2.flatten_datatables = args1.flatten_datatables = true;
  args2.should_store_props = args1.should_store_props = false;
  args1.message = datatable1;
  args1.version_data = &args->first->demver_data;
  args2.message = datatable2;
  args2.version_data = &args->second->demver_data;

  dg_estate_init(&estate1, args1);
  dg_estate_init(&estate2, args2);

  compare_sendtables(args, &estate1, &estate2);

end:

  dg_arena_free(&arena);
  dg_estate_free(&estate1);
  dg_estate_free(&estate2);

  return result;
}

void *memory_stream::get_ptr() {
  std::uint8_t *ptr = (std::uint8_t *)this->buffer;
  return ptr + this->offset;
}

std::size_t memory_stream::get_bytes_left() {
  if (this->offset > this->file_size) {
    return 0;
  } else {
    return this->file_size - this->offset;
  }
}

void memory_stream::allocate_space(size_t bytes) {
  if (bytes + this->offset > this->buffer_size) {
    std::size_t next_buffer_size = std::max(bytes + this->offset, this->buffer_size * 2);
    if (this->buffer) {
      this->buffer = realloc(this->buffer, next_buffer_size);
    } else {
      this->buffer = malloc(next_buffer_size);
    }

    this->buffer_size = next_buffer_size;
  }
}

void memory_stream::fill_with_file(const char *filepath) {
  FILE *stream = fopen(filepath, "rb");
  const int read_size = 4096;
  bool eof = false;

  while (!eof) {
    this->allocate_space(read_size);
    auto ptr = this->get_ptr();
    auto readIt = fread(ptr, 1, read_size, stream);
    eof = readIt != read_size;
    this->offset += readIt;
  }

  this->file_size = offset;
  this->offset = 0; // Set to beginning of file
  fclose(stream);
}

memory_stream::~memory_stream() {
  if (this->buffer) {
    free(this->buffer);
  }
}

std::size_t freddie::memory_stream_read(void *s, void *dest, size_t bytes) {
  memory_stream *stream = reinterpret_cast<memory_stream *>(s);

  if (stream->offset > stream->file_size) {
    return 0;
  } else {
    auto ptr = stream->get_ptr();
    bytes = std::min(stream->get_bytes_left(), bytes);
    memcpy(dest, ptr, bytes);
    stream->offset += bytes;
    return bytes;
  }
}

int freddie::memory_stream_seek(void *s, long int offset) {
  memory_stream *stream = reinterpret_cast<memory_stream *>(s);
  stream->offset += offset;

  return 0;
}

std::size_t freddie::memory_stream_write(void *s, const void *src, size_t bytes) {
  memory_stream *stream = reinterpret_cast<memory_stream *>(s);
  stream->allocate_space(bytes);
  auto dest = stream->get_ptr();
  memcpy(dest, src, bytes);

  if (stream->ground_truth && stream->agrees) {
    if (stream->offset + bytes > stream->ground_truth->file_size) {
      stream->agrees = false;
      if (stream->errfunc)
        stream->errfunc("Write out of bounds for ground truth");
    } else {
      auto comparison_ptr = (uint8_t *)stream->ground_truth->buffer + stream->offset;
      if (memcmp(dest, comparison_ptr, bytes) != 0) {
        stream->agrees = false;
        if (stream->errfunc)
          stream->errfunc("Did not match with ground truth");
      }
    }
  }

  stream->offset += bytes;
  stream->file_size += bytes;

  return bytes;
}
