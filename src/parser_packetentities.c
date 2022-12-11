#include "parser_packetentities.h"
#include "demogobbler.h"
#include "demogobbler/allocator.h"
#include "demogobbler/bitstream.h"
#include "demogobbler/bitwriter.h"
#include "demogobbler/entity_types.h"
#include "parser_entity_state.h"
#include "utils.h"
#include "vector_array.h"
#include <string.h>

#ifdef DEBUG_BREAK_PROP
int DG_CURRENT_DEBUG_INDEX = 0;
static int BREAK_INDEX = 20;
#endif

typedef struct {
  dg_parser *thisptr;
  dg_bitstream *stream;
  dg_alloc_state *a;
  dg_alloc_state *permanent_arena;
  dg_ent_update *update;
  dg_vector_array prop_array;
  dg_packetentities_data output;
  bool new_way;
} prop_parse_state;

static prop_value read_prop(prop_parse_state *state, dg_sendprop *props, dg_sendprop* prop);
static void write_prop(bitwriter *writer, dg_prop_value_inner value);

static void write_int(bitwriter *thisptr, dg_prop_value_inner value) {
  if (value.type == dg_int_varuint32) {
    bitwriter_write_varuint32(thisptr, value.unsigned_val);
  } else if (value.type == dg_int_unsigned) {
    bitwriter_write_uint(thisptr, value.unsigned_val, value.prop_numbits);
  } else {
    bitwriter_write_sint(thisptr, value.signed_val, value.prop_numbits);
  }
}

static void read_int(prop_parse_state *state, dg_sendprop *prop, dg_prop_value_inner *value) {
  value->prop_numbits = prop->prop_numbits;
  if (prop->flag_normal) {
    value->type = dg_int_varuint32;
    value->unsigned_val = dg_bitstream_read_varuint32(state->stream);
  } else if (prop->flag_unsigned) {
    value->type = dg_int_unsigned;
    value->unsigned_val = dg_bitstream_read_uint(state->stream, prop->prop_numbits);
  } else {
    value->type = dg_int_signed;
    value->signed_val = dg_bitstream_read_sint(state->stream, prop->prop_numbits);
  }
}

static void write_float(bitwriter *thisptr, dg_prop_value_inner value) {
  switch(value.type)
  {
    case dg_float_bitcoord:
      bitwriter_write_bitcoord(thisptr, value.bitcoord_val);
      break;
    case dg_float_bitcoordmp:
      dg_bitwriter_write_bitcoordmp(thisptr, value.bitcoordmp_val, false, false);
      break;
    case dg_float_bitcoordmplp:
      dg_bitwriter_write_bitcoordmp(thisptr, value.bitcoordmp_val, false, true);
      break;
    case dg_float_bitcoordmpint:
      dg_bitwriter_write_bitcoordmp(thisptr, value.bitcoordmp_val, true, false);
      break;
    case dg_float_noscale:
      bitwriter_write_float(thisptr, value.float_val);
      break;
    case dg_float_bitnormal:
      dg_bitwriter_write_bitnormal(thisptr, value.bitnormal_val);
      break;
    case dg_float_bitcellcoord:
      dg_bitwriter_write_bitcellcoord(thisptr, value.bitcellcoord_val, false, false,
                                      value.prop_numbits);
      break;
    case dg_float_bitcellcoordlp:
      dg_bitwriter_write_bitcellcoord(thisptr, value.bitcellcoord_val, false, true,
                                    value.prop_numbits);
      break;
    case dg_float_bitcellcoordint:
      dg_bitwriter_write_bitcellcoord(thisptr, value.bitcellcoord_val, true, false,
                                    value.prop_numbits);
      break;
    case dg_float_unsigned:
      bitwriter_write_uint(thisptr, value.unsigned_val, value.prop_numbits);
      break;

  }
}

static void read_float(prop_parse_state *state, dg_sendprop *prop, dg_prop_value_inner *value) {
  dg_bitstream *stream = state->stream;
  value->prop_numbits = prop->prop_numbits;
  if (prop->flag_coord) {
    value->type = dg_float_bitcoord;
    value->bitcoord_val = dg_bitstream_read_bitcoord(stream);
  } else if (prop->flag_coordmp) {
    value->type = dg_float_bitcoordmp;
    value->bitcoordmp_val = dg_bitstream_read_bitcoordmp(stream, false, false);
  } else if (prop->flag_coordmplp) {
    value->type = dg_float_bitcoordmplp;
    value->bitcoordmp_val = dg_bitstream_read_bitcoordmp(stream, false, true);
  } else if (prop->flag_coordmpint) {
    value->type = dg_float_bitcoordmpint;
    value->bitcoordmp_val = dg_bitstream_read_bitcoordmp(stream, true, false);
  } else if (prop->flag_noscale) {
    value->type = dg_float_noscale;
    value->float_val = dg_bitstream_read_float(stream);
  } else if (prop->flag_normal) {
    value->type = dg_float_bitnormal;
    value->bitnormal_val = dg_bitstream_read_bitnormal(stream);
  } else if (prop->flag_cellcoord) {
    value->type = dg_float_bitcellcoord;
    value->bitcellcoord_val =
        dg_bitstream_read_bitcellcoord(stream, false, false, prop->prop_numbits);
  } else if (prop->flag_cellcoordlp) {
    value->type = dg_float_bitcellcoordlp;
    value->bitcellcoord_val =
        dg_bitstream_read_bitcellcoord(stream, false, true, prop->prop_numbits);
  } else if (prop->flag_cellcoordint) {
    value->type = dg_float_bitcellcoordint;
    value->bitcellcoord_val =
        dg_bitstream_read_bitcellcoord(stream, true, false, prop->prop_numbits);
  } else {
    value->type = dg_float_unsigned;
    value->unsigned_val = dg_bitstream_read_uint(stream, prop->prop_numbits);
  }
}

static void write_vector3(bitwriter *thisptr, dg_prop_value_inner value) {
  write_float(thisptr, value.v3_val->x);
  write_float(thisptr, value.v3_val->y);

  if (value.v3_val->_sign != dg_vector3_sign_no) {
    bool sign = value.v3_val->_sign == dg_vector3_sign_pos;
    bitwriter_write_bit(thisptr, sign);
  } else {
    write_float(thisptr, value.v3_val->z);
  }
}

static void read_vector3(prop_parse_state *state, dg_sendprop *prop, dg_prop_value_inner *value) {
  value->v3_val = dg_alloc_allocate(state->a, sizeof(dg_vector3_value), alignof(dg_vector3_value));
  memset(value->v3_val, 0, sizeof(dg_vector3_value));

  read_float(state, prop, &value->v3_val->x);
  read_float(state, prop, &value->v3_val->y);

  if (prop->flag_normal) {
    value->v3_val->_sign = bitstream_read_bit(state->stream) ? dg_vector3_sign_pos : dg_vector3_sign_neg;
  } else {
    read_float(state, prop, &value->v3_val->z);
    value->v3_val->_sign = dg_vector3_sign_no;
  }
}

static void write_vector2(bitwriter *thisptr, dg_prop_value_inner value) {
  write_float(thisptr, value.v2_val->x);
  write_float(thisptr, value.v2_val->y);
}

static void read_vector2(prop_parse_state *state, dg_sendprop *prop, dg_prop_value_inner *value) {
  value->v2_val = dg_alloc_allocate(state->a, sizeof(dg_vector2_value), alignof(dg_vector2_value));
  memset(value->v2_val, 0, sizeof(dg_vector2_value));

  read_float(state, prop, &value->v2_val->x);
  read_float(state, prop, &value->v2_val->y);
}

static const size_t dt_max_string_bits = 9;

static void write_string(bitwriter *thisptr, dg_prop_value_inner value) {
  bitwriter_write_uint(thisptr, value.str_val->len, dt_max_string_bits);
  bitwriter_write_bits(thisptr, value.str_val->str, 8 * value.str_val->len);
}

static void read_string(prop_parse_state *state, dg_sendprop *prop, dg_prop_value_inner *value) {
  value->str_val = dg_alloc_allocate(state->a, sizeof(dg_string_value), alignof(dg_string_value));
  size_t len = value->str_val->len = bitstream_read_uint(state->stream, dt_max_string_bits);
  value->str_val->str = dg_alloc_allocate(state->a, len + 1, 1);
  bitstream_read_fixed_string(state->stream, value->str_val->str, len);
  value->str_val->str[len] = '\0'; // make sure we have zero terminated string
}

static void write_array(bitwriter *thisptr, dg_prop_value_inner value) {
  struct dg_array_value *arr = value.arr_val;
  unsigned int bits = highest_bit_index(value.array_num_elements ) + 1;
  bitwriter_write_uint(thisptr, arr->array_size, bits);

  for (size_t i = 0; i < arr->array_size; ++i) {
    write_prop(thisptr, arr->values[i]);
  }
}

static void read_array(prop_parse_state *state, dg_sendprop *props, uint32_t prop_index, dg_prop_value_inner *value) {
  dg_sendprop* prop = props + prop_index;
  value->array_num_elements = prop->array_num_elements;
  value->arr_val = dg_alloc_allocate(state->a, sizeof(dg_array_value), alignof(dg_array_value));
  value->arr_val->array_size =
      bitstream_read_uint(state->stream, highest_bit_index(prop->array_num_elements) + 1);
  value->arr_val->values =
      dg_alloc_allocate(state->a, sizeof(dg_prop_value_inner) * value->arr_val->array_size,
                        alignof(dg_prop_value_inner));

  for (size_t i = 0; i < value->arr_val->array_size; ++i) {
    prop_value temp = read_prop(state, props, prop->array_prop);
    value->arr_val->values[i] = temp.value;
  }
}

static void write_prop(bitwriter *thisptr, dg_prop_value_inner value) {
  dg_sendproptype type = value.proptype;
  switch (type) {
  case sendproptype_array:
    write_array(thisptr, value);
    break;
  case sendproptype_vector3:
    write_vector3(thisptr, value);
    break;
  case sendproptype_vector2:
    write_vector2(thisptr, value);
    break;
  case sendproptype_float:
    write_float(thisptr, value);
    break;
  case sendproptype_string:
    write_string(thisptr, value);
    break;
  case sendproptype_int:
    write_int(thisptr, value);
    break;
  default:
    thisptr->error = true;
    thisptr->error_message = "Unknown sendproptype for write_prop";
    break;
  }
}

static prop_value read_prop(prop_parse_state *state, dg_sendprop *props, dg_sendprop *prop) {
#ifdef DEBUG_BREAK_PROP
  ++DG_CURRENT_DEBUG_INDEX;
#endif

#ifdef DEBUG_BREAK_PROP
  if (BREAK_INDEX == DG_CURRENT_DEBUG_INDEX) {
    ; // Set a breakpoint here
  }
#endif

  prop_value value;
  memset(&value, 0, sizeof(value));
  value.prop_index = prop - props;
  value.value.proptype = prop->proptype;
  if (prop->proptype == sendproptype_array) {
    read_array(state, props, value.prop_index, &value.value);
  } else if (prop->proptype == sendproptype_vector3) {
    read_vector3(state, prop, &value.value);
  } else if (prop->proptype == sendproptype_vector2) {
    read_vector2(state, prop, &value.value);
  } else if (prop->proptype == sendproptype_float) {
    read_float(state, prop, &value.value);
  } else if (prop->proptype == sendproptype_string) {
    read_string(state, prop, &value.value);
  } else if (prop->proptype == sendproptype_int) {
    read_int(state, prop, &value.value);
  } else {
    state->thisptr->error = true;
    state->thisptr->error_message = "Got an unknown prop type in read_prop";
  }

  return value;
}

static void write_props_prot4(bitwriter *thisptr, struct write_packetentities_args args,
                              const dg_ent_update *update) {
  if (args.version->game != l4d) {
    bitwriter_write_bit(thisptr, update->new_way);
  }

  int last_prop_index = -1;
  for (size_t i = 0; i < update->prop_value_array_size; ++i) {
    int prop_index = update->prop_value_array[i].prop_index;
    dg_bitwriter_write_field_index(thisptr, prop_index, last_prop_index, update->new_way);
    last_prop_index = prop_index;
    write_prop(thisptr, update->prop_value_array[i].value);
  }

  dg_bitwriter_write_field_index(thisptr, -1, last_prop_index, update->new_way);
}

static void parse_props_prot4(prop_parse_state *state) {
  dg_parser *thisptr = state->thisptr;
  dg_bitstream *stream = state->stream;
  dg_serverclass_data *datas = dg_estate_serverclass_data(thisptr, state->update->datatable_id);
  int i = -1;
  bool new_way = thisptr->demo_version.game != l4d && bitstream_read_bit(state->stream); // 1560
  state->new_way = new_way;

  while (true) {
    i = bitstream_read_field_index(state->stream, i, new_way); // 1576

    if (i < -1 || i >= (int)datas->prop_count) {
      thisptr->error = true;
      thisptr->error_message = "Invalid prop index encountered";
    }

    if (i == -1 || thisptr->error || stream->overflow)
      break;

    prop_value value = read_prop(state, datas->props, datas->props + i);
    dg_va_push_back(&state->prop_array, &value);
  }
}

static void write_props_old(bitwriter *thisptr, struct write_packetentities_args args,
                            const dg_ent_update *update) {
  int old_prop_index = -1;
  for (size_t i = 0; i < update->prop_value_array_size; ++i) {
    bitwriter_write_bit(thisptr, true);
    int prop_index = update->prop_value_array[i].prop_index;
    uint32_t diffy = prop_index - (old_prop_index + 1);
    old_prop_index = prop_index;
    dg_bitwriter_write_ubitvar(thisptr, diffy);
    write_prop(thisptr, update->prop_value_array[i].value);
  }
  bitwriter_write_bit(thisptr, false);
}

static void parse_props_old(prop_parse_state *state) {
  dg_parser *thisptr = state->thisptr;
  dg_serverclass_data *data = dg_estate_serverclass_data(thisptr, state->update->datatable_id);
  int i = -1;

  while (bitstream_read_bit(state->stream)) {
    i += bitstream_read_ubitvar(state->stream) + 1;
    if (i < -1 || i >= data->prop_count) {
      state->thisptr->error = true;
      state->thisptr->error_message = "Invalid prop index encountered";
    }

    if (i == -1 || state->thisptr->error || state->stream->overflow)
      break;

    prop_value value = read_prop(state, data->props, data->props + i);
    dg_va_push_back(&state->prop_array, &value);
  }
}

static void write_props(bitwriter *thisptr, struct write_packetentities_args args,
                        const dg_ent_update *update) {
  if (args.version->demo_protocol == 4) {
    write_props_prot4(thisptr, args, update);
  } else {
    write_props_old(thisptr, args, update);
  }
}

static void parse_props(prop_parse_state *state, size_t index) {
  state->update = state->output.ent_updates + index;
  dg_va_clear(&state->prop_array);
  dg_demver_data *demo_version = &state->thisptr->demo_version;
  if (demo_version->demo_protocol == 4) {
    parse_props_prot4(state);
    state->update->new_way = state->new_way;
  } else {
    parse_props_old(state);
  }

  if (state->prop_array.count_elements > 0) {
    state->update->prop_value_array_size = state->prop_array.count_elements;
    size_t bytes = sizeof(prop_value) * state->prop_array.count_elements;
    state->update->prop_value_array = dg_alloc_allocate(state->a, bytes, alignof(prop_value));
    memcpy(state->update->prop_value_array, state->prop_array.ptr, bytes);
  }
}

static void read_explicit_deletes(prop_parse_state *state, bool new_logic) {
  if (new_logic) {
    int32_t nbase = -1;
    uint32_t ncount = bitstream_read_ubitint(state->stream);
    uint32_t bytes = sizeof(int32_t) * ncount;
    state->output.explicit_deletes = dg_alloc_allocate(state->a, bytes, alignof(int));
    state->output.explicit_deletes_count = ncount;

    for (uint32_t i = 0; i < ncount; ++i) {
      int32_t ndelta = bitstream_read_ubitint(state->stream);
      int32_t nslot = nbase + ndelta;
      state->output.explicit_deletes[i] = nslot;
      nbase = nslot;
    }
  } else {
    int index = 0;
    int max_updates = (state->stream->bitsize - state->stream->bitoffset) / (1 + MAX_EDICT_BITS);

    if (max_updates >= 0) {
      size_t bytes = sizeof(int) * max_updates;
      state->output.explicit_deletes = dg_alloc_allocate(state->a, bytes, alignof(int));
      memset(state->output.explicit_deletes, 0, bytes);
    }

    while (bitstream_read_bit(state->stream)) {
      unsigned delete_index = bitstream_read_uint(state->stream, MAX_EDICT_BITS);

      if (index >= max_updates) {
        state->thisptr->error = true;
        state->thisptr->error_message = "Had more explicit deletes than expected";
        break;
      }

      state->output.explicit_deletes[index] = delete_index;
      ++state->output.explicit_deletes_count;
      ++index;
    }
  }
}

static bool game_has_new_deletes(const dg_demver_data *version) {
  return version->game == l4d2 && version->l4d2_version >= 2091;
}

void dg_bitwriter_write_packetentities(bitwriter *thisptr, struct write_packetentities_args args) {
  unsigned bits = args.data.serverclass_bits;
  int index = -1;
  size_t i;
  for (i = 0; i < args.data.ent_updates_count && !thisptr->error; ++i) {
    const dg_ent_update *update = args.data.ent_updates + i;
    uint32_t diffy = update->ent_index - (index + 1);
    if (args.version->demo_protocol >= 4) {
      dg_bitwriter_write_ubitint(thisptr, diffy);
    } else {
      dg_bitwriter_write_ubitvar(thisptr, diffy);
    }
    index = update->ent_index;
    bitwriter_write_uint(thisptr, update->update_type, 2);

    if (update->update_type == 0) {
      write_props(thisptr, args, update);
    } else if (update->update_type == 2) {
      bitwriter_write_uint(thisptr, update->datatable_id, bits);
      bitwriter_write_uint(thisptr, update->handle, HANDLE_BITS);
      write_props(thisptr, args, update);
    }
  }

  if (args.is_delta && !thisptr->error) {
    if (game_has_new_deletes(args.version)) {
      dg_bitwriter_write_ubitint(thisptr, args.data.explicit_deletes_count);
      int32_t nbase = -1;
      for (size_t i = 0; i < args.data.explicit_deletes_count; ++i) {
        int32_t delta = args.data.explicit_deletes[i] - nbase;
        dg_bitwriter_write_ubitint(thisptr, delta);
        nbase = args.data.explicit_deletes[i];
      }
    } else {
      for (size_t i = 0; i < args.data.explicit_deletes_count; ++i) {
        bitwriter_write_bit(thisptr, true);
        bitwriter_write_uint(thisptr, args.data.explicit_deletes[i], MAX_EDICT_BITS);
      }
      bitwriter_write_bit(thisptr, false);
    }
  }
}

void dg_parse_packetentities(dg_parser *thisptr, struct dg_svc_packet_entities *message) {
  if (!thisptr->state.entity_state.edicts) {
    thisptr->error = true;
    thisptr->error_message = "Tried to parse packetentities without datatables";
    return;
  }

  dg_bitstream stream = message->data;
  prop_parse_state state;
  memset(&state, 0, sizeof(state));
  state.thisptr = thisptr;
  state.a = dg_parser_temp_allocator(thisptr);
  state.permanent_arena = dg_parser_perm_allocator(thisptr);
  state.stream = &stream;

  state.output.ent_updates_count = message->updated_entries;
  size_t ent_update_bytes = sizeof(dg_ent_update) * state.output.ent_updates_count;
  state.output.ent_updates = dg_alloc_allocate(state.a, ent_update_bytes, alignof(dg_ent_update));
  memset(state.output.ent_updates, 0, ent_update_bytes);

  prop_value props_array[64];
  state.prop_array = dg_va_create(props_array, prop_value);

  state.output.serverclass_bits = Q_log2(thisptr->state.entity_state.serverclass_count) + 1;

  if (!thisptr->state.entity_state.class_datas) {
    thisptr->error = true;
    thisptr->error_message = "Tried to parse packet entities with no flattened props";
  }

  int newI = -1;
  size_t i;

  for (i = 0; i < message->updated_entries && !thisptr->error && !stream.overflow; ++i) {
    dg_ent_update *update = state.output.ent_updates + i;
    newI += 1;
    if (thisptr->demo_version.demo_protocol >= 4) {
      newI += bitstream_read_ubitint(&stream);
    } else {
      newI += bitstream_read_ubitvar(&stream);
    }

    unsigned update_type = bitstream_read_uint(&stream, 2);

    update->update_type = update_type;
    update->ent_index = newI;
    const dg_edict *ent = thisptr->state.entity_state.edicts + newI;

    if (newI >= MAX_EDICTS || newI < 0) {
      thisptr->error = true;
      thisptr->error_message = "Illegal entity index";
      goto end;
    }
    if (update_type == 0) {
      // delta
      update->datatable_id = ent->datatable_id;
      parse_props(&state, i);
    } else if (update_type == 2) {
      // enter pvs
      update->datatable_id = bitstream_read_uint(&stream, state.output.serverclass_bits);
      update->handle = bitstream_read_uint(&stream, HANDLE_BITS);

      if (update->datatable_id >= thisptr->state.entity_state.serverclass_count) {
        thisptr->error = true;
        thisptr->error_message = "Invalid class ID in svc_packetentities";
        goto end;
      }

      parse_props(&state, i);
    }
  }

  if (message->is_delta && !thisptr->error && !stream.overflow) {
    bool new_deletes =
        thisptr->demo_version.game == l4d2 && thisptr->demo_version.l4d2_version >= 2091;
    read_explicit_deletes(&state, new_deletes);
  }

  if (stream.overflow && !thisptr->error) {
    thisptr->error = true;
    thisptr->error_message = "Stream overflowed in svc_packetentities\n";
  }

  if (stream.bitsize > stream.bitoffset) {
    thisptr->error = true;
    thisptr->error_message = "Did not read all bits from svc_packetentities\n";
  }

  if (!thisptr->error && i != message->updated_entries) {
    thisptr->error = true;
    thisptr->error_message = "Read the wrong number of entries in svc_packetentities\n";
  }

end:;
#ifdef DEMOGOBBLER_UNSAFE
  if (true) {
#else
  if (!thisptr->error) {
#endif
    dg_svc_packetentities_parsed* parsed_ptr;
    message->parsed = parsed_ptr = dg_alloc_allocate(state.a, sizeof(dg_svc_packetentities_parsed), alignof(dg_svc_packetentities_parsed)); 
    memset(parsed_ptr, 0, sizeof(*parsed_ptr));
    if (thisptr->m_settings.packetentities_parsed_handler) {
      parsed_ptr->data = state.output;
      parsed_ptr->orig = message;
      thisptr->m_settings.packetentities_parsed_handler(&thisptr->state, parsed_ptr);
    }

    dg_estate_update(&thisptr->state.entity_state, &state.output);
  }

  dg_va_free(&state.prop_array);

  if (thisptr->error) {
#ifdef DEBUG_BREAK_PROP
    printf("Failed at %d, %u / %u bits parsed, error %s\n", DG_CURRENT_DEBUG_INDEX,
           stream.bitoffset - message->data.bitoffset, message->data_length,
           thisptr->error_message);
#endif
  }
}
