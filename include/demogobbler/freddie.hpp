#pragma once

// Freddie is a higher level C++ API for demo related stuff

#include "demogobbler.h"
#include "demogobbler/allocator.h"
#include <functional>
#include <memory>
#include <stddef.h>
#include <unordered_map>
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
    dg_parse_result write_demo(void *stream, dg_output_interface interface, bool expect_equal=false);
    dg_parse_result write_demo(const char *filepath);
    dg_datatables_parsed *get_datatables() const;
  };

  dg_parse_result convert_demo(const demo_t *example, demo_t *demo);

  struct prop_status {
    dg_sendprop* target = nullptr;
    uint32_t index = 0;
    bool exists = true;
    bool flags_changed = false;
  };

  struct datatable_status {
    uint32_t index = 0;
    bool exists = true;
    bool props_changed = false;
  };

  struct datatable_change_info {
    datatable_change_info(dg_alloc_state allocator);
    ~datatable_change_info();
    datatable_change_info(const datatable_change_info& lhs) = delete;
    datatable_change_info& operator=(const datatable_change_info& lhs) = delete;

    dg_parse_result init(freddie::demo_t *input, const freddie::demo_t *target);
    void add_datatable(uint32_t new_index, bool changed, bool exists);
    void add_prop(dg_sendprop* prop, prop_status status);
    void print(bool print_props);
    void print_props(uint32_t datatable_id);
  
    prop_status get_prop_status(dg_sendprop* prop);
    datatable_status get_datatable_status(uint32_t index);
    dg_parse_result convert_updates(dg_packetentities_data* data);
    dg_parse_result convert_instancebaselines(dg_sentry* stringtable, dg_bitstream* data);
    dg_parse_result convert_props(dg_ent_update* update, uint32_t new_datatable_id);
    dg_parse_result convert_demo(freddie::demo_t* input);

    dg_alloc_state allocator;
    estate input_estate;
    estate target_estate;
    std::unordered_map<dg_sendprop*, prop_status> prop_map;
    std::vector<datatable_status> datatables;
    dg_ent_update* baselines;
    uint32_t baselines_count;
    dg_datatables_parsed target_datatable;
    dg_demver_data target_demver;
  };

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

  dg_bitstream get_start_state(const dg_bitwriter *writer);
  void finalize_stream(dg_bitstream *stream, const dg_bitwriter *writer);

  // Function for interfacing with C
  std::size_t memory_stream_read(void *stream, void *dest, size_t bytes);
  int memory_stream_seek(void *stream, long int offset);
  std::size_t memory_stream_write(void *stream, const void *src, size_t bytes);

  typedef size_t (*dg_input_read)(void *stream, void *dest, size_t bytes);
  typedef int (*dg_input_seek)(void *stream, long int offset);
} // namespace freddie
