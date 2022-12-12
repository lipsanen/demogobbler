#pragma once

// Freddie is a higher level C++ API for demo related stuff

#include "demogobbler.h"
#include "demogobbler/allocator.h"
#include <functional>
#include <memory>
#include <stddef.h>
#include <variant>
#include <vector>

namespace freddie {

  struct mallocator {
  std::vector<void*> pointers;

  mallocator();
  mallocator(const mallocator& rhs) = delete;
  mallocator& operator=(mallocator&& rhs) = default;
  mallocator& operator=(const mallocator& rhs) = delete;
  ~mallocator();
  void* alloc(uint32_t size);
  void attach(void* ptr);
  void release();       // release the memory stored without freeing
  void free(void *ptr); // free specific pointer
  };

  typedef std::variant<packet_parsed, dg_customdata, dg_datatables_parsed,
                      dg_stringtables_parsed, dg_consolecmd, dg_synctick, dg_usercmd,
                      dg_stop>
      packet_variant_t;

  struct demo_packet {
  packet_variant_t packet;
  mallocator memory;
  };

  dg_parse_result splice_demos(const char *output_path, const char **demo_paths, size_t demo_count);

  struct demo_t {
    demo_t();
    ~demo_t();
    mallocator temp_allocator;
    dg_arena arena;
    dg_demver_data demver_data;
    dg_header header;
    std::vector<std::shared_ptr<demo_packet>> packets;
    demo_t(const demo_t &rhs) = delete;
    demo_t &operator=(const demo_t &rhs) = delete;

    template <typename T> std::shared_ptr<demo_packet> copy_packet(T *orig) {
        auto packet = std::make_shared<demo_packet>();
        packet->memory = std::move(temp_allocator);
        packet->packet = *orig;
        temp_allocator.release();

        return packet;
    }

    static dg_parse_result parse_demo(demo_t *output, void *stream, dg_input_interface interface);
    static dg_parse_result parse_demo(demo_t *output, const char *filepath);
    dg_parse_result write_demo(void *stream, dg_output_interface interface);
    dg_parse_result write_demo(const char *filepath);
  };

  dg_parse_result convert_demo(const demo_t *example, demo_t *demo);
  typedef void(*printFunc)(bool first, const char* msg, ...);

  struct compare_props_args {
      const demo_t* first;
      const demo_t* second;
      printFunc func = nullptr;
  };

  dg_parse_result compare_props(const compare_props_args* args);
  typedef std::function<void(const char *error)> error_func;

  struct memory_stream {
    size_t offset = 0;
    size_t buffer_size = 0;
    size_t file_size = 0;
    void *buffer = nullptr;
    bool agrees = true;
    memory_stream *ground_truth = nullptr;
    error_func errfunc = nullptr;

    void *get_ptr();
    size_t get_bytes_left();
    void allocate_space(size_t bytes);
    void fill_with_file(const char *filepath);
    ~memory_stream();
  };

  // Function for interfacing with C
  std::size_t memory_stream_read(void *stream, void *dest, size_t bytes);
  int memory_stream_seek(void *stream, long int offset);
  std::size_t memory_stream_write(void *stream, const void *src, size_t bytes);

  typedef size_t (*dg_input_read)(void *stream, void *dest, size_t bytes);
  typedef int (*dg_input_seek)(void *stream, long int offset);
} // namespace freddie
