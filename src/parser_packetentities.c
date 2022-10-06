#include "parser_packetentities.h"
#include "arena.h"
#include "demogobbler/bitstream.h"
#include "demogobbler/bitwriter.h"
#include "demogobbler/entity_types.h"
#include "parser_entity_state.h"
#include "utils.h"
#include "vector_array.h"
#include <string.h>

#ifdef DEBUG_BREAK_PROP
int CURRENT_DEBUG_INDEX = 0;
static int BREAK_INDEX = 20;
#endif

typedef struct {
  parser *thisptr;
  bitstream *stream;
  arena *a;
  ent_update* update;
  vector_array prop_array;
  packetentities_data output;
  bool new_way;
} prop_parse_state;

static prop_value read_prop(prop_parse_state *state, demogobbler_sendprop *prop);
static void write_prop(bitwriter* writer, prop_value value);

static void write_int(bitwriter* thisptr, prop_value value) {
  if(value.prop->flag_unsigned) {
    bitwriter_write_uint(thisptr, value.value.unsigned_val, value.prop->prop_numbits);
  } else {
    bitwriter_write_sint(thisptr, value.value.signed_val, value.prop->prop_numbits);
  }
}

static void read_int(prop_parse_state *state, demogobbler_sendprop *prop, prop_value_inner *value) {
  if (prop->flag_unsigned) {
    value->unsigned_val = demogobbler_bitstream_read_uint(state->stream, prop->prop_numbits);
  } else {
    value->signed_val = demogobbler_bitstream_read_sint(state->stream, prop->prop_numbits);
  }
}

static void write_float(bitwriter* thisptr, demogobbler_sendprop *prop, prop_value_inner value) {
  if (prop->flag_coord) {
    bitwriter_write_bitcoord(thisptr, value.bitcoord_val);
  } else if (prop->flag_coordmp) {
    demogobbler_bitwriter_write_bitcoordmp(thisptr, value.bitcoordmp_val, false, false);
  } else if (prop->flag_coordmplp) {
    demogobbler_bitwriter_write_bitcoordmp(thisptr, value.bitcoordmp_val, false, true);
  } else if (prop->flag_coordmpint) {
    demogobbler_bitwriter_write_bitcoordmp(thisptr, value.bitcoordmp_val, true, false);
  } else if (prop->flag_noscale) {
    bitwriter_write_float(thisptr, value.float_val);
  } else if (prop->flag_normal) {
    demogobbler_bitwriter_write_bitnormal(thisptr, value.bitnormal_val);
  } else if (prop->flag_cellcoord) {
    demogobbler_bitwriter_write_bitcellcoord(thisptr, value.bitcellcoord_val, false, false, prop->prop_numbits);
  } else if (prop->flag_cellcoordlp) {
    demogobbler_bitwriter_write_bitcellcoord(thisptr, value.bitcellcoord_val, false, true, prop->prop_numbits);
  } else if (prop->flag_cellcoordint) {
    demogobbler_bitwriter_write_bitcellcoord(thisptr, value.bitcellcoord_val, true, false, prop->prop_numbits);
  } else {
    bitwriter_write_uint(thisptr, value.unsigned_val, prop->prop_numbits);
  }
}

static void read_float(prop_parse_state *state, demogobbler_sendprop *prop,
                       prop_value_inner *value) {
  bitstream *stream = state->stream;
  if (prop->flag_coord) {
    value->bitcoord_val = demogobbler_bitstream_read_bitcoord(stream);
  } else if (prop->flag_coordmp) {
    value->bitcoordmp_val = demogobbler_bitstream_read_bitcoordmp(stream, false, false);
  } else if (prop->flag_coordmplp) {
    value->bitcoordmp_val = demogobbler_bitstream_read_bitcoordmp(stream, false, true);
  } else if (prop->flag_coordmpint) {
    value->bitcoordmp_val = demogobbler_bitstream_read_bitcoordmp(stream, true, false);
  } else if (prop->flag_noscale) {
    value->float_val = demogobbler_bitstream_read_float(stream);
  } else if (prop->flag_normal) {
    value->bitnormal_val = demogobbler_bitstream_read_bitnormal(stream);
  } else if (prop->flag_cellcoord) {
    value->bitcellcoord_val =
        demogobbler_bitstream_read_bitcellcoord(stream, false, false, prop->prop_numbits);
  } else if (prop->flag_cellcoordlp) {
    value->bitcellcoord_val =
        demogobbler_bitstream_read_bitcellcoord(stream, false, true, prop->prop_numbits);
  } else if (prop->flag_cellcoordint) {
    value->bitcellcoord_val =
        demogobbler_bitstream_read_bitcellcoord(stream, true, false, prop->prop_numbits);
  } else {
    value->unsigned_val = demogobbler_bitstream_read_uint(stream, prop->prop_numbits);
  }
}

static void write_vector3(bitwriter* thisptr, prop_value value) {
  write_float(thisptr, value.prop, value.value.v3_val->x);
  write_float(thisptr, value.prop, value.value.v3_val->y);

  if(value.prop->flag_normal) {
    bitwriter_write_bit(thisptr, value.value.v3_val->sign);
  } else {
    write_float(thisptr, value.prop, value.value.v3_val->z);
  }
}

static void read_vector3(prop_parse_state *state, demogobbler_sendprop *prop,
                         prop_value_inner *value) {
  value->v3_val =
      demogobbler_arena_allocate(state->a, sizeof(vector3_value), alignof(vector3_value));
  memset(value->v3_val, 0, sizeof(vector3_value));

  read_float(state, prop, &value->v3_val->x);
  read_float(state, prop, &value->v3_val->y);

  if (prop->flag_normal) {
    value->v3_val->sign = bitstream_read_bit(state->stream);
  } else {
    read_float(state, prop, &value->v3_val->z);
  }
}

static void write_vector2(bitwriter* thisptr, prop_value value) {
  write_float(thisptr, value.prop, value.value.v2_val->x);
  write_float(thisptr, value.prop, value.value.v2_val->y);
}

static void read_vector2(prop_parse_state *state, demogobbler_sendprop *prop,
                         prop_value_inner *value) {
  value->v2_val =
      demogobbler_arena_allocate(state->a, sizeof(vector2_value), alignof(vector2_value));
  memset(value->v2_val, 0, sizeof(vector2_value));

  read_float(state, prop, &value->v2_val->x);
  read_float(state, prop, &value->v2_val->y);
}

static const size_t dt_max_string_bits = 9;

static void write_string(bitwriter* thisptr, prop_value value) {
  bitwriter_write_uint(thisptr, value.value.str_val->len, dt_max_string_bits);
  bitwriter_write_bits(thisptr, value.value.str_val->str, 8 * value.value.str_val->len);
}

static void read_string(prop_parse_state *state, demogobbler_sendprop *prop,
                        prop_value_inner *value) {
  value->str_val =
      demogobbler_arena_allocate(state->a, sizeof(string_value), alignof(string_value));
  size_t len = value->str_val->len = bitstream_read_uint(state->stream, dt_max_string_bits);
  value->str_val->str = demogobbler_arena_allocate(state->a, len + 1, 1);
  bitstream_read_fixed_string(state->stream, value->str_val->str, len);
  value->str_val->str[len] = '\0'; // make sure we have zero terminated string
}

static void write_array(bitwriter* thisptr, prop_value value) {
  struct array_value* arr = value.value.arr_val;
  unsigned int bits = highest_bit_index(value.prop->array_num_elements) + 1;
  bitwriter_write_uint(thisptr, arr->array_size, bits);
  prop_value temp;
  memset(&temp, 0, sizeof(temp));
  temp.prop = value.prop->array_prop;

  for (size_t i=0; i < arr->array_size; ++i) {
    temp.value = arr->values[i];
    write_prop(thisptr, temp);
  }
}

static void read_array(prop_parse_state *state, demogobbler_sendprop *prop,
                       prop_value_inner *value) {
  value->arr_val = demogobbler_arena_allocate(state->a, sizeof(array_value), alignof(array_value));
  value->arr_val->array_size =
      bitstream_read_uint(state->stream, highest_bit_index(prop->array_num_elements) + 1);
  value->arr_val->values = demogobbler_arena_allocate(state->a, sizeof(prop_value_inner) * value->arr_val->array_size,
                                                      alignof(prop_value_inner));

  for (size_t i = 0; i < value->arr_val->array_size; ++i) {
    prop_value temp = read_prop(state, prop->array_prop);
    value->arr_val->values[i] = temp.value;
  }
}

static void write_prop(bitwriter* thisptr, prop_value value) {
  demogobbler_sendproptype type = value.prop->proptype;
  switch(type) {
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
      write_float(thisptr, value.prop, value.value);
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

static prop_value read_prop(prop_parse_state *state, demogobbler_sendprop *prop) {
#ifdef DEBUG_BREAK_PROP
  ++CURRENT_DEBUG_INDEX;
#endif

#ifdef DEBUG_BREAK_PROP
  if (BREAK_INDEX == CURRENT_DEBUG_INDEX) {
    ; // Set a breakpoint here
  }
#endif

  prop_value value;
  memset(&value, 0, sizeof(value));
  value.prop = prop;
  if (prop->proptype == sendproptype_array) {
    read_array(state, prop, &value.value);
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

static void write_props_prot4(bitwriter* thisptr, struct write_packetentities_args args, const ent_update* update) {
  const serverclass_data* data = args.entity_state->class_datas + update->datatable_id;

  if(args.version->game != l4d) {
    bitwriter_write_bit(thisptr, update->new_way);
  }

  int last_prop_index = -1;
  for(size_t i=0; i < update->prop_value_array_size; ++i) {
    int prop_index = update->prop_value_array[i].prop - data->props;
    demogobbler_bitwriter_write_field_index(thisptr, prop_index, last_prop_index, update->new_way);
    last_prop_index = prop_index;
    write_prop(thisptr, update->prop_value_array[i]);
  }

  demogobbler_bitwriter_write_field_index(thisptr, -1, last_prop_index, update->new_way);
}

static void parse_props_prot4(prop_parse_state *state) {
  parser *thisptr = state->thisptr;
  bitstream *stream = state->stream;
  serverclass_data *datas = demogobbler_estate_serverclass_data(thisptr, state->update->datatable_id);
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

    prop_value value = read_prop(state, datas->props + i);
    demogobbler_va_push_back(&state->prop_array, &value);
  }
}

static void write_props_old(bitwriter* thisptr, struct write_packetentities_args args, const ent_update* update) {
  const serverclass_data* data = args.entity_state->class_datas + update->datatable_id;
  int old_prop_index = -1;
  for(size_t i=0; i < update->prop_value_array_size; ++i) {
    bitwriter_write_bit(thisptr, true);
    int prop_index = update->prop_value_array[i].prop - data->props;
    uint32_t diffy = prop_index - (old_prop_index + 1);
    old_prop_index = prop_index;
    demogobbler_bitwriter_write_ubitvar(thisptr, diffy);
    write_prop(thisptr, update->prop_value_array[i]);
  }
  bitwriter_write_bit(thisptr, false);
}

static void parse_props_old(prop_parse_state *state) {
  parser *thisptr = state->thisptr;
  serverclass_data *data = demogobbler_estate_serverclass_data(thisptr, state->update->datatable_id);
  int i = -1;

  while (bitstream_read_bit(state->stream)) {
    i += bitstream_read_ubitvar(state->stream) + 1;
    if (i < -1 || i >= data->prop_count) {
      state->thisptr->error = true;
      state->thisptr->error_message = "Invalid prop index encountered";
    }

    if (i == -1 || state->thisptr->error || state->stream->overflow)
      break;

    prop_value value = read_prop(state, data->props + i);
    demogobbler_va_push_back(&state->prop_array, &value);
  }
}

static void write_props(bitwriter* thisptr, struct write_packetentities_args args, const ent_update* update) {
  if(args.version->demo_protocol == 4) {
    write_props_prot4(thisptr, args, update);
  } else {
    write_props_old(thisptr, args, update);
  }
}

static void parse_props(prop_parse_state *state, size_t index) {
  state->update = state->output.ent_updates + index;
  demogobbler_va_clear(&state->prop_array);
  demo_version_data *demo_version = &state->thisptr->demo_version;
  if (demo_version->demo_protocol == 4) {
    parse_props_prot4(state);
    state->update->new_way = state->new_way;
  } else {
    parse_props_old(state);
  }

  if (state->prop_array.count_elements > 0) {
    state->update->prop_value_array_size = state->prop_array.count_elements;
    size_t bytes = sizeof(prop_value) * state->prop_array.count_elements;
    state->update->prop_value_array = demogobbler_arena_allocate(state->a, bytes, alignof(prop_value));
    memcpy(state->update->prop_value_array, state->prop_array.ptr, bytes);
  }
}

static void read_explicit_deletes(prop_parse_state *state, bool new_logic) {
  if(new_logic) {
    int32_t nbase = -1;
    uint32_t ncount = bitstream_read_ubitint(state->stream);
    uint32_t bytes = sizeof(int32_t) * ncount;
    state->output.explicit_deletes = demogobbler_arena_allocate(state->a, bytes, alignof(int));
    state->output.explicit_deletes_count = ncount;

    for(uint32_t i=0; i < ncount; ++i) {
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
      state->output.explicit_deletes = demogobbler_arena_allocate(state->a, bytes, alignof(int));
      memset(state->output.explicit_deletes, 0, bytes);
    }

    while (bitstream_read_bit(state->stream)) {
      unsigned delete_index = bitstream_read_uint(state->stream, MAX_EDICT_BITS);
      edict *ent = state->thisptr->state.entity_state.edicts + delete_index;
      memset(ent, 0, sizeof(edict));

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

static bool game_has_new_deletes(const demo_version_data* version) {
  return version->game == l4d2 && version->l4d2_version >= 2091;
}

void demogobbler_bitwriter_write_packetentities(bitwriter* thisptr, struct write_packetentities_args args) {
  unsigned bits = Q_log2(args.entity_state->serverclass_count) + 1;
  int index = -1;
  size_t i;
  for (i = 0; i < args.data.ent_updates_count && !thisptr->error; ++i) {
    const ent_update* update = args.data.ent_updates  + i;
    uint32_t diffy = update->ent_index - (index + 1);
    if(args.version->demo_protocol >= 4) {
      demogobbler_bitwriter_write_ubitint(thisptr, diffy);
    } else {
      demogobbler_bitwriter_write_ubitvar(thisptr, diffy);
    }
    index = update->ent_index;
    bitwriter_write_uint(thisptr, update->update_type, 2);

    if(update->update_type == 0) {
      write_props(thisptr, args, update);
    } else if (update->update_type == 2) {
      bitwriter_write_uint(thisptr, update->datatable_id, bits);
      bitwriter_write_uint(thisptr, update->handle, HANDLE_BITS);
      write_props(thisptr, args, update);
    }
  }

  if(args.is_delta && !thisptr->error) {
    if(game_has_new_deletes(args.version)) {
      demogobbler_bitwriter_write_ubitint(thisptr, args.data.explicit_deletes_count);
      int32_t nbase = -1;
      for(size_t i=0; i < args.data.explicit_deletes_count; ++i) {
        int32_t delta = args.data.explicit_deletes[i] - nbase;
        demogobbler_bitwriter_write_ubitint(thisptr, delta);
        nbase = args.data.explicit_deletes[i];
      }
    } else {
      for(size_t i=0; i < args.data.explicit_deletes_count; ++i) {
        bitwriter_write_bit(thisptr, true);
        bitwriter_write_uint(thisptr, args.data.explicit_deletes[i], MAX_EDICT_BITS);
      }
      bitwriter_write_bit(thisptr, false);
    }
  }
}

void demogobbler_apply_entupdate(estate* entity_state, packetentities_data data) {
  for(size_t i=0; i < data.ent_updates_count; ++i) {
    ent_update* update = data.ent_updates + i;
    edict* ent = entity_state->edicts + update->ent_index;
    if(update->update_type == 2) {
      // Enter pvs
      ent->exists = true;
      ent->datatable_id = update->datatable_id;
      ent->handle = update->handle;
      ent->in_pvs = true;
    } else if (update->update_type == 1) {
      // Leave PVS
      ent->in_pvs = false;
    } else if (update->update_type == 3) {
      memset(ent, 0, sizeof(edict));
    }
  }

  for(size_t i=0; i < data.explicit_deletes_count; ++i) {
    edict* ent = entity_state->edicts + data.explicit_deletes[i];
    memset(ent, 0, sizeof(edict));
  }
}

void demogobbler_parse_packetentities(parser *thisptr,
                                      struct demogobbler_svc_packet_entities *message) {
  if(!thisptr->state.entity_state.edicts) {
    thisptr->error = true;
    thisptr->error_message = "Tried to parse packetentities without datatables";
    return;
  }
  
  bitstream stream = message->data;
  prop_parse_state state;
  memset(&state, 0, sizeof(state));
  state.thisptr = thisptr;
  state.a = &thisptr->temp_arena;
  state.stream = &stream;

  state.output.ent_updates_count = message->updated_entries;
  size_t ent_update_bytes = sizeof(ent_update) * state.output.ent_updates_count;
  state.output.ent_updates =
      demogobbler_arena_allocate(state.a, ent_update_bytes, alignof(ent_update));
  memset(state.output.ent_updates, 0, ent_update_bytes);

  prop_value props_array[64];
  state.prop_array = demogobbler_va_create(props_array, prop_value);

  unsigned bits = Q_log2(thisptr->state.entity_state.serverclass_count) + 1;

  if (!thisptr->state.entity_state.class_datas) {
    thisptr->error = true;
    thisptr->error_message = "Tried to parse packet entities with no flattened props";
  }

  int newI = -1;
  size_t i;

  for (i = 0; i < message->updated_entries && !thisptr->error && !stream.overflow; ++i) {
    ent_update* update = state.output.ent_updates + i;
    newI += 1;
    if (thisptr->demo_version.demo_protocol >= 4) {
      newI += bitstream_read_ubitint(&stream);
    } else {
      newI += bitstream_read_ubitvar(&stream);
    }

    unsigned update_type = bitstream_read_uint(&stream, 2);

    update->update_type = update_type;
    update->ent_index = newI;
    const edict* ent = thisptr->state.entity_state.edicts + newI;

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
      update->datatable_id = bitstream_read_uint(&stream, bits);
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
    bool new_deletes = thisptr->demo_version.game == l4d2 && thisptr->demo_version.l4d2_version >= 2091;
    read_explicit_deletes(&state, new_deletes);
  }

end:;
#ifdef DEMOGOBBLER_UNSAFE
  if (thisptr->m_settings.packetentities_parsed_handler) {
#else
  if (!thisptr->error && !stream.overflow && thisptr->m_settings.packetentities_parsed_handler) {
#endif
    svc_packetentities_parsed parsed;
    memset(&parsed, 0, sizeof(parsed));
    parsed.data = state.output;
    parsed.orig = message;
    thisptr->m_settings.packetentities_parsed_handler(&thisptr->state, &parsed);
  }

  demogobbler_apply_entupdate(&thisptr->state.entity_state, state.output);

  demogobbler_va_free(&state.prop_array);

  if(stream.bitsize > stream.bitoffset) {
    thisptr->error = true;
    thisptr->error_message = "Did not read all bits from svc_packetentities\n";
  }

  if (stream.overflow && !thisptr->error) {
    thisptr->error = true;
    thisptr->error_message = "Stream overflowed in svc_packetentities\n";
  }

  if (!thisptr->error && i != message->updated_entries) {
    thisptr->error = true;
    thisptr->error_message = "Read the wrong number of entries in svc_packetentities\n";
  }

  if (thisptr->error) {
#ifdef DEBUG_BREAK_PROP
    printf("Failed at %d, %u / %u bits parsed, error %s\n", CURRENT_DEBUG_INDEX, stream.bitoffset - message->data.bitoffset, message->data_length, thisptr->error_message);
#endif
  }
}
