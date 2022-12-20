#include "parser_stringtables.h"
#include "alignof_wrapper.h"
#include "demogobbler.h"
#include "demogobbler/allocator.h"
#include "demogobbler/bitstream.h"
#include "demogobbler/streams.h"
#include "utils.h"
#include "writer.h"
#include <string.h>

static void write_stringtable_entry(bitwriter *writer, dg_stringtable_entry *entry) {
  bitwriter_write_cstring(writer, entry->name);
  bitwriter_write_bit(writer, entry->has_data);
  if (entry->has_data) {
    bitwriter_write_uint(writer, entry->size, 16);
    bitwriter_write_bitstream(writer, &entry->data);
  }
}

void dg_write_stringtables_parsed(writer *thisptr, dg_stringtables_parsed *message) {
  dg_write_byte(thisptr, message->orig.preamble.type);
  dg_write_int32(thisptr, message->orig.preamble.tick);
  if (thisptr->version.has_slot_in_preamble) {
    dg_write_byte(thisptr, message->orig.preamble.slot);
  }

  thisptr->bitwriter.bitoffset = 0;
  bitwriter_write_uint(&thisptr->bitwriter, message->tables_count, 8);
  for (uint8_t i = 0; i < message->tables_count; ++i) {
    dg_stringtable *table = message->tables + i;
    bitwriter_write_cstring(&thisptr->bitwriter, table->table_name);
    bitwriter_write_uint(&thisptr->bitwriter, table->entries_count, 16);

    for (uint16_t u = 0; u < table->entries_count; ++u) {
      write_stringtable_entry(&thisptr->bitwriter, table->entries + u);
    }

    bitwriter_write_bit(&thisptr->bitwriter, table->has_classes);

    if (table->has_classes) {
      bitwriter_write_uint(&thisptr->bitwriter, table->classes_count, 16);
      for (uint16_t u = 0; u < table->classes_count; ++u) {
        write_stringtable_entry(&thisptr->bitwriter, table->classes + u);
      }
    }
  }

  if (thisptr->bitwriter.bitoffset % 8 != 0) {
    if (thisptr->bitwriter.bitoffset == message->leftover_bits.bitoffset) {
      bitwriter_write_bitstream(&thisptr->bitwriter, &message->leftover_bits);
    }
  }

  uint32_t bytes = thisptr->bitwriter.bitoffset / 8;
  dg_write_int32(thisptr, thisptr->bitwriter.bitoffset / 8);
  dg_write_data(thisptr, thisptr->bitwriter.ptr, bytes);
}

static const char *read_string(dg_bitstream *stream, dg_alloc_state *allocator) {
  char BUFFER[1024];
  uint32_t bytes = bitstream_read_cstring(stream, BUFFER, 1024);
  char *dest = dg_alloc_allocate(allocator, bytes, 1);
  memcpy(dest, BUFFER, bytes);
  return dest;
}

static void read_stringtable_entry(dg_stringtable_entry *entry, dg_bitstream *stream,
                                   stringtable_parse_args args) {
  entry->name = read_string(stream, args.allocator);
  entry->has_data = bitstream_read_bit(stream);
  if (entry->has_data) {
    entry->size = bitstream_read_uint(stream, 16);
    entry->data = bitstream_fork_and_advance(stream, entry->size * 8);
  }
}

dg_parse_result dg_parse_stringtables(dg_stringtables_parsed *message,
                                      stringtable_parse_args args) {
  dg_parse_result result;
  memset(message, 0, sizeof(*message));
  memset(&result, 0, sizeof(result));
  message->orig = *args.message;

  dg_bitstream stream = bitstream_create(args.message->data, args.message->size_bytes * 8);
  message->tables_count = bitstream_read_uint(&stream, 8);
  message->tables = dg_alloc_allocate(
      args.allocator, message->tables_count * sizeof(dg_stringtable), alignof(dg_stringtable));
  memset(message->tables, 0, message->tables_count * sizeof(dg_stringtable));

  for (uint8_t i = 0; i < message->tables_count; ++i) {
    dg_stringtable *table = message->tables + i;
    table->table_name = read_string(&stream, args.allocator);
    table->entries_count = bitstream_read_uint(&stream, 16);
    table->entries =
        dg_alloc_allocate(args.allocator, table->entries_count * sizeof(dg_stringtable_entry),
                          alignof(dg_stringtable_entry));
    if(table->entries_count > 0) {
      memset(table->entries, 0, table->entries_count * sizeof(dg_stringtable_entry));
      for (uint16_t u = 0; u < table->entries_count; ++u) {
        read_stringtable_entry(table->entries + u, &stream, args);
      }
    }

    table->has_classes = bitstream_read_bit(&stream);
    if (table->has_classes) {
      table->classes_count = bitstream_read_uint(&stream, 16);
      table->classes =
          dg_alloc_allocate(args.allocator, table->classes_count * sizeof(dg_stringtable_entry),
                            alignof(dg_stringtable_entry));
      for (uint16_t u = 0; u < table->classes_count; ++u) {
        read_stringtable_entry(table->classes + u, &stream, args);
      }
    }
  }

  unsigned bits_left = dg_bitstream_bits_left(&stream);

  if (bits_left != 0) {
    message->leftover_bits = bitstream_fork_and_advance(&stream, bits_left);
  }

  if (stream.overflow) {
    result.error = true;
    result.error_message = "Overflowed during dg_stringtable parsing";
  }

  return result;
}

void dg_parser_parse_stringtables(dg_parser *thisptr, dg_stringtables *input) {
  dg_stringtables_parsed out;
  stringtable_parse_args args;
  memset(&out, 0, sizeof(out));
  memset(&args, 0, sizeof(args));

  args.allocator = dg_parser_temp_allocator(thisptr);
  args.message = input;

  dg_parse_result result = dg_parse_stringtables(&out, args);

  if (!result.error || thisptr->m_settings.stringtables_parsed_handler) {
    thisptr->m_settings.stringtables_parsed_handler(&thisptr->state, &out);
  } else if (result.error) {
    thisptr->error = true;
    thisptr->error_message = result.error_message;
  }
}


static void parse_sentry(dg_sentry_parse_args *args, uint32_t entry_bits, int32_t* entry_index, dg_sentry_value* value) {
  char entry_string[1024];
  value->entry_bit = bitstream_read_bit(&args->stream);
  if (!value->entry_bit) {
    value->entry_index = bitstream_read_uint(&args->stream, entry_bits);
    *entry_index = value->entry_index;
  } else {
    value->entry_index = *entry_index;
  }

  value->has_name = bitstream_read_bit(&args->stream);
  if(value->has_name) {
    value->reuse_previous_value = bitstream_read_bit(&args->stream);
    if(value->reuse_previous_value) {
      value->reuse_str_index = bitstream_read_uint(&args->stream, 5);
      value->reuse_length = bitstream_read_uint(&args->stream, 5);
    }
    size_t size = dg_bitstream_read_cstring(&args->stream, entry_string, sizeof(entry_string));
    value->stored_string = dg_alloc_allocate(args->allocator, size, 1);
    memcpy(value->stored_string, entry_string, size);
  }

  value->has_user_data = bitstream_read_bit(&args->stream);

  if(value->has_user_data) {
    uint32_t bits;
    if(args->user_data_fixed_size) {
      bits = args->user_data_size_bits;
    } else {
      value->userdata_length = bitstream_read_uint(&args->stream, 14);
      bits = value->userdata_length * 8;
    }
    value->userdata = bitstream_fork_and_advance(&args->stream, bits);
  }
}

void dg_write_sentry_value(dg_sentry_write_args *args, const dg_sentry_value *_value, uint32_t entry_bits, bool fixed_size, uint32_t user_data_bits) {
  dg_sentry_value value = *_value;
  bitwriter_write_bit(args->writer, value.entry_bit);
  if (!value.entry_bit) {
     bitwriter_write_uint(args->writer, value.entry_index, entry_bits);
  }

  bitwriter_write_bit(args->writer, value.has_name);
  if(value.has_name) {
    bitwriter_write_bit(args->writer, value.reuse_previous_value);
    if(value.reuse_previous_value) {
      bitwriter_write_uint(args->writer, value.reuse_str_index, 5);
      bitwriter_write_uint(args->writer, value.reuse_length, 5);
    }
    bitwriter_write_cstring(args->writer, value.stored_string);
  }

  bitwriter_write_bit(args->writer, value.has_user_data);

  if(value.has_user_data) {
    if(!fixed_size) {
      bitwriter_write_uint(args->writer, value.userdata_length, 14);
    }
    bitwriter_write_bitstream(args->writer, &value.userdata);
  }
}

dg_parse_result dg_write_stringtable_entry(dg_sentry_write_args *args) {
  dg_parse_result result;
  const dg_sentry* input = args->input;
  memset(&result, 0, sizeof(result));
  uint32_t entry_bits = Q_log2(input->max_entries);

  for (size_t i = 0; i < input->values_length; ++i) {
    dg_write_sentry_value(args, input->values + i, entry_bits, input->user_data_fixed_size, input->user_data_size_bits);
  }

  return result;
}


dg_parse_result dg_parse_stringtable_entry(dg_sentry_parse_args *args, dg_sentry *out) {
  dg_parse_result result;
  memset(&result, 0, sizeof(result));
  memset(out, 0, sizeof(dg_stringtable));

  if (args->flags & 1) {
    //int32_t uncompressed_size = bitstream_read_sint32(&args->stream);
    //int32_t compressed_size = bitstream_read_sint32(&args->stream);

    // TODO: implement compression

    result.error = true;
    result.error_message = "compressed stringtable parsing not implemented";
    goto end;
  }

  if (args->demver_data->demo_protocol == 4) {
    out->dictionary_enabled = bitstream_read_bit(&args->stream);

    // TODO: implement dictionary

    result.error = true;
    result.error_message = "dictionary decoding not implemented";
    goto end;
  }

  int32_t entry_index = -1;
  const uint32_t array_bytes = args->num_updated_entries * sizeof(dg_sentry_value);
  out->values = dg_alloc_allocate(args->allocator, array_bytes, alignof(dg_sentry_value));
  out->values_length = args->num_updated_entries;
  out->flags = args->flags;
  out->max_entries = args->max_entries;
  out->user_data_fixed_size = args->user_data_fixed_size;
  out->user_data_size_bits = args->user_data_size_bits;

  if(out->values)
    memset(out->values, 0, array_bytes);
  uint32_t entry_bits = Q_log2(args->max_entries);

  for (size_t i = 0; i < args->num_updated_entries && !args->stream.overflow; ++i) {
    ++entry_index;
    dg_sentry_value *value = out->values + i;
    parse_sentry(args, entry_bits, &entry_index, value);
  }


  if(args->stream.overflow || dg_bitstream_bits_left(&args->stream) > 0) {
    result.error = true;
    result.error_message = "stringtable parsing had bits left";
  }

end:
  return result;
}
