#include "parser_datatables.h"
#include "alignof_wrapper.h"
#include "demogobbler/allocator.h"
#include "demogobbler.h"
#include "demogobbler/bitstream.h"
#include "demogobbler/bitwriter.h"
#include "demogobbler/datatable_types.h"
#include "parser_entity_state.h"
#include "utils.h"
#include <string.h>

typedef dg_datatables_parsed datatables;

typedef struct {
  dg_demver_data *demo_version;
  bool error;
  const char *error_message;
} datatable_parser;

static_assert(sizeof(dg_sendprop) <= 32,
              "Sendprops are larger than they should be, weird bitfield behavior?");

static const dg_sendproptype old_props[] = {sendproptype_int,     sendproptype_float,
                                            sendproptype_vector3, sendproptype_string,
                                            sendproptype_array,   sendproptype_datatable};

static const dg_sendproptype new_props[] = {
    sendproptype_int,    sendproptype_float, sendproptype_vector3,  sendproptype_vector2,
    sendproptype_string, sendproptype_array, sendproptype_datatable};

static unsigned sendproptype_to_raw(writer *thisptr, dg_sendproptype proptype) {
  size_t size;
  const dg_sendproptype *props;
  dg_demver_data *version = &thisptr->version;

  if (version->network_protocol <= 14) {
    props = old_props;
    size = ARRAYSIZE(old_props);
  } else {
    props = new_props;
    size = ARRAYSIZE(new_props);
  }

  for (size_t i = 0; i < size; ++i) {
    if (props[i] == proptype) {
      return i;
    }
  }

  thisptr->error = true;
  if (proptype == sendproptype_vector2) {
    thisptr->error_message = "Unable to convert sendprop vector2";
  } else {
    thisptr->error_message = "Unable to convert sendprop type";
  }

  return 0;
}

static dg_sendproptype raw_to_sendproptype(datatable_parser *thisptr, unsigned int value) {
  dg_demver_data *data = thisptr->demo_version;
  if (data->network_protocol <= 14) {
    if (value < ARRAYSIZE(old_props)) {
      return old_props[value];
    }
  } else {
    if (value < ARRAYSIZE(new_props)) {
      return new_props[value];
    }
  }

  return sendproptype_invalid;
}

static char *parse_cstring(dg_alloc_state *a, dg_bitstream *stream) {
  char STRINGBUF[1024];
  size_t size = bitstream_read_cstring(stream, STRINGBUF, sizeof(STRINGBUF));
  char *rval = NULL;

  if (size == 0) {
    stream->overflow = true;
  } else if (!stream->overflow) {
    rval = dg_alloc_allocate(a, size, 1);
    memcpy(rval, STRINGBUF, size);
  }

  return rval;
}

static unsigned get_flags_from_sendprop(writer *thisptr, dg_sendprop *prop) {
  unsigned flags = 0;
  // hopefully the compiler wizards can generate branchless code from this
  // otherwise there's like 100 branches in this function
#define GET_BIT(member, index)                                                                     \
  flags |= (prop->member && (index < thisptr->version.sendprop_flag_bits)) ? (1 << index) : 0;
  // Uses & instead of && in the hope that it avoids a branch

  GET_BIT(flag_unsigned, 0);
  GET_BIT(flag_coord, 1);
  GET_BIT(flag_noscale, 2);
  GET_BIT(flag_rounddown, 3);
  GET_BIT(flag_roundup, 4);
  GET_BIT(flag_normal, 5);
  GET_BIT(flag_exclude, 6);
  GET_BIT(flag_xyze, 7);
  GET_BIT(flag_insidearray, 8);
  GET_BIT(flag_proxyalwaysyes, 9);

  if (thisptr->version.sendprop_flag_bits <= 16) {
    GET_BIT(flag_changesoften, 10);
    GET_BIT(flag_isvectorelem, 11);
    GET_BIT(flag_collapsible, 12);
    GET_BIT(flag_coordmp, 13);
    GET_BIT(flag_coordmplp, 14);
    GET_BIT(flag_coordmpint, 15);
  } else {
    GET_BIT(flag_isvectorelem, 10);
    GET_BIT(flag_collapsible, 11);
    GET_BIT(flag_coordmp, 12);
    GET_BIT(flag_coordmplp, 13);
    GET_BIT(flag_coordmpint, 14);
    GET_BIT(flag_cellcoord, 15);
    GET_BIT(flag_cellcoordlp, 16);
    GET_BIT(flag_cellcoordint, 17);
    GET_BIT(flag_changesoften, 18);
  }
#undef GET_BIT

  return flags;
}

static void get_sendprop_flags(datatable_parser *thisptr, dg_sendprop *prop, unsigned int flags) {
#define GET_BIT(member, index) prop->member = (flags & (1 << index)) ? 1 : 0;

  GET_BIT(flag_unsigned, 0);
  GET_BIT(flag_coord, 1);
  GET_BIT(flag_noscale, 2);
  GET_BIT(flag_rounddown, 3);
  GET_BIT(flag_roundup, 4);
  GET_BIT(flag_normal, 5);
  GET_BIT(flag_exclude, 6);
  GET_BIT(flag_xyze, 7);
  GET_BIT(flag_insidearray, 8);
  GET_BIT(flag_proxyalwaysyes, 9);

  if (thisptr->demo_version->sendprop_flag_bits <= 16) {
    GET_BIT(flag_changesoften, 10);
    GET_BIT(flag_isvectorelem, 11);
    GET_BIT(flag_collapsible, 12);
    GET_BIT(flag_coordmp, 13);
    GET_BIT(flag_coordmplp, 14);
    GET_BIT(flag_coordmpint, 15);
  } else {
    GET_BIT(flag_isvectorelem, 10);
    GET_BIT(flag_collapsible, 11);
    GET_BIT(flag_coordmp, 12);
    GET_BIT(flag_coordmplp, 13);
    GET_BIT(flag_coordmpint, 14);
    GET_BIT(flag_cellcoord, 15);
    GET_BIT(flag_cellcoordlp, 16);
    GET_BIT(flag_cellcoordint, 17);
    GET_BIT(flag_changesoften, 18);
  }

#undef GET_BIT
}

static void write_sendprop(writer *thisptr, bitwriter *writer, dg_sendprop *prop) {
  unsigned int raw_value = sendproptype_to_raw(thisptr, prop->proptype);
  if (thisptr->error)
    return;

  bitwriter_write_uint(writer, raw_value, 5);
  bitwriter_write_cstring(writer, prop->name);
  unsigned int flags = get_flags_from_sendprop(thisptr, prop);
  bitwriter_write_uint(writer, flags, thisptr->version.sendprop_flag_bits);

  if (thisptr->version.demo_protocol >= 4 && thisptr->version.game != l4d) {
    bitwriter_write_uint(writer, prop->priority, 8);
  }

  if (prop->proptype == sendproptype_datatable) {
    bitwriter_write_cstring(writer, prop->dtname);
  } else if (prop->flag_exclude) {
    bitwriter_write_cstring(writer, prop->exclude_name);
  } else if (prop->proptype == sendproptype_array) {
    bitwriter_write_uint(writer, prop->array_num_elements, 10);
  } else {
    bitwriter_write_float(writer, prop->prop_.low_value);
    bitwriter_write_float(writer, prop->prop_.high_value);
    bitwriter_write_uint(writer, prop->prop_numbits, thisptr->version.sendprop_numbits_for_numbits);
  }
}

static void parse_sendprop(datatable_parser *thisptr, dg_alloc_state *a, dg_bitstream *stream,
                           dg_sendtable *table, dg_sendprop *prop, size_t index) {
  memset(prop, 0, sizeof(dg_sendprop));
  unsigned int raw_value = bitstream_read_uint(stream, 5);
  prop->proptype = raw_to_sendproptype(thisptr, raw_value);

  if (prop->proptype == sendproptype_invalid) {
    thisptr->error = true;
    thisptr->error_message = "Invalid sendprop type in datatable";
    return;
  }

  prop->name = parse_cstring(a, stream);
  unsigned flags = bitstream_read_uint(stream, thisptr->demo_version->sendprop_flag_bits);
  get_sendprop_flags(thisptr, prop, flags);

  if (thisptr->demo_version->demo_protocol >= 4 && thisptr->demo_version->game != l4d) {
    prop->priority = bitstream_read_uint(stream, 8);
  }

  if (prop->proptype == sendproptype_datatable) {
    prop->dtname = parse_cstring(a, stream);
  } else if (prop->flag_exclude) {
    prop->exclude_name = parse_cstring(a, stream);
  } else if (prop->proptype == sendproptype_array) {
    prop->baseclass = table;
    prop->array_num_elements = bitstream_read_uint(stream, 10);
    prop->array_prop = (prop - 1); // The insidearray prop should be in the previous element

    if (index == 0 || !prop->array_prop->flag_insidearray) {
      thisptr->error = true;
      thisptr->error_message = "Array prop not preceded by insidearray prop";
    }
  } else {
    prop->baseclass = table;
    prop->prop_.low_value = bitstream_read_float(stream);
    prop->prop_.high_value = bitstream_read_float(stream);
    prop->prop_numbits =
        bitstream_read_uint(stream, thisptr->demo_version->sendprop_numbits_for_numbits);
  }
}

static void write_sendtable(writer *thisptr, bitwriter *writer, dg_sendtable *ptable) {
  bitwriter_write_bit(writer, ptable->needs_decoder);
  bitwriter_write_cstring(writer, ptable->name);
  bitwriter_write_uint(writer, ptable->prop_count, thisptr->version.datatable_propcount_bits);

  for (size_t i = 0; i < ptable->prop_count; ++i) {
    write_sendprop(thisptr, writer, ptable->props + i);
  }
}

#define ERROR_SET (stream->overflow || thisptr->error)

static void parse_sendtable(datatable_parser *thisptr, dg_alloc_state *a, dg_bitstream *stream,
                            dg_sendtable *ptable) {
  memset(ptable, 0, sizeof(dg_sendtable));
  ptable->needs_decoder = bitstream_read_bit(stream);
  ptable->name = parse_cstring(a, stream);

  if (ptable->name == NULL) {
    thisptr->error = true;
    thisptr->error_message = "Sendtable name was null";
    return;
  }

  ptable->prop_count = bitstream_read_uint(stream, thisptr->demo_version->datatable_propcount_bits);
  ptable->props =
      dg_alloc_allocate(a, ptable->prop_count * sizeof(dg_sendprop), alignof(dg_sendprop));

  for (size_t i = 0; i < ptable->prop_count && !ERROR_SET; ++i) {
    parse_sendprop(thisptr, a, stream, ptable, ptable->props + i, i);
  }
}

static void write_serverclass(bitwriter *writer, dg_serverclass *pclass) {
  bitwriter_write_uint(writer, pclass->serverclass_id, 16);
  bitwriter_write_cstring(writer, pclass->serverclass_name);
  bitwriter_write_cstring(writer, pclass->datatable_name);
}

static void parse_serverclass(datatable_parser *thisptr, dg_alloc_state *a, dg_bitstream *stream,
                              dg_serverclass *pclass) {
  memset(pclass, 0, sizeof(dg_serverclass));
  pclass->serverclass_id = bitstream_read_uint(stream, 16);
  pclass->serverclass_name = parse_cstring(a, stream);
  pclass->datatable_name = parse_cstring(a, stream);
}

#undef ERROR_SET
#define ERROR_SET (stream.overflow || thisptr->error)

void dg_write_datatables_parsed(writer *thisptr, dg_datatables_parsed *datatables) {
  bitwriter writer;
  bitwriter_init(&writer, datatables->_raw_buffer_bytes * 8);

  for (size_t i = 0; i < datatables->sendtable_count; ++i) {
    bitwriter_write_bit(&writer, true);
    write_sendtable(thisptr, &writer, datatables->sendtables + i);
  }
  bitwriter_write_bit(&writer, false);
  bitwriter_write_uint(&writer, datatables->serverclass_count, 16);

  for (size_t i = 0; i < datatables->serverclass_count; ++i) {
    write_serverclass(&writer, datatables->serverclasses + i);
  }

  size_t bytes = writer.bitoffset / 8;
  size_t bit_in_byte_offset = writer.bitoffset & 0x7;

  if (bit_in_byte_offset != 0) {
    bytes += 1;
    if (datatables->_raw_buffer && bytes == datatables->_raw_buffer_bytes) {
      // If we are exactly at the same number of bytes, copy the last bits out of the buffer to
      // ensure bit for bit equality
      dg_bitstream stream =
          bitstream_create(datatables->_raw_buffer, datatables->_raw_buffer_bytes * 8);
      bitstream_advance(&stream, writer.bitoffset);
      unsigned int bits_left = stream.bitsize - stream.bitoffset;
      unsigned int value = bitstream_read_uint(&stream, bits_left);
      bitwriter_write_uint(&writer, value, bits_left);
    }
  }

  if (writer.error && !thisptr->error) {
    thisptr->error = writer.error;
    thisptr->error_message = writer.error_message;
  }

  if (!thisptr->error) {
    dg_write_preamble(thisptr, datatables->preamble);
    thisptr->output_funcs.write(thisptr->_stream, &bytes, 4);
    thisptr->output_funcs.write(thisptr->_stream, writer.ptr, bytes);
  }

  bitwriter_free(&writer);
}

dg_datatables_parsed_rval dg_parse_datatables(dg_demver_data *version_data,
                                              dg_alloc_state *allocator, dg_datatables *input) {
  datatable_parser dparser;
  datatables output;
  memset(&dparser, 0, sizeof(dparser));
  memset(&output, 0, sizeof(output));

  dparser.demo_version = version_data;
  output.preamble = input->preamble;
  output._raw_buffer = input->data;
  output._raw_buffer_bytes = input->size_bytes;

  dg_bitstream stream = bitstream_create(input->data, input->size_bytes * 8);

  size_t array_size = 1024; // a guess at what the array size could be
  output.sendtables = malloc(array_size * sizeof(dg_sendtable));

  while (bitstream_read_bit(&stream)) {
    if (output.sendtable_count >= array_size) {
      array_size <<= 1;
      output.sendtables = realloc(output.sendtables, array_size * sizeof(dg_sendtable));
    }
    parse_sendtable(&dparser, allocator, &stream, output.sendtables + output.sendtable_count);
    ++output.sendtable_count;
  }
  //printf("sendtable count %lu\n", output.sendtable_count);
  dg_alloc_attach(allocator, output.sendtables, array_size * sizeof(dg_sendtable));

  output.serverclass_count = bitstream_read_uint(&stream, 16);
  output.serverclasses = dg_alloc_allocate(
      allocator, output.serverclass_count * sizeof(dg_serverclass), alignof(dg_serverclass));

  for (size_t i = 0; i < output.serverclass_count && !dparser.error; ++i) {
    parse_serverclass(&dparser, allocator, &stream, output.serverclasses + i);
  }

  unsigned bits_left = dg_bitstream_bits_left(&stream);

  if (stream.overflow && !dparser.error) {
    dparser.error = true;
    dparser.error_message = "Bitstream overflowed while parsing datatables";
  } else if(bits_left >= 8) {
    dparser.error = true;
    dparser.error_message = "Did not read all bits out of datatables";
  }

  dg_datatables_parsed_rval rval;
  rval.error = dparser.error;
  rval.error_message = dparser.error_message;
  rval.output = output;

  return rval;
}

void parse_datatables(dg_parser *thisptr, dg_datatables *input) {
  dg_alloc_state* allocator;
  bool init_entity_state;
  if (thisptr->state.entity_state.edicts == NULL && (thisptr->m_settings.parse_packetentities || thisptr->m_settings.flattened_props_handler)) {
    allocator = dg_parser_perm_allocator(thisptr);
    init_entity_state = false;
  } else {
    allocator = dg_parser_temp_allocator(thisptr);
    init_entity_state = true;
  }

  dg_datatables_parsed_rval value =
      dg_parse_datatables(&thisptr->demo_version, allocator, input);

  if (!value.error) {
    if (thisptr->m_settings.datatables_parsed_handler)
      thisptr->m_settings.datatables_parsed_handler(&thisptr->state, &value.output);
    if (!init_entity_state) {
      dg_parser_init_estate(thisptr, &value.output);
    }
  } else {
    thisptr->error = value.error;
    thisptr->error_message = value.error_message;
  }
}
