#include "parser_packetentities.h"
#include "arena.h"
#include "bitstream.h"
#include "demogobbler_entity_types.h"
#include "parser_entity_state.h"
#include "utils.h"
#include "vector_array.h"
#include <signal.h>
#include <string.h>

#ifdef DEBUG_BREAK_PROP
int CURRENT_DEBUG_INDEX = 0;
static int BREAK_INDEX = 20;
#endif

typedef struct {
  parser *thisptr;
  bitstream *stream;
  edict *ent;
  arena *a;
  vector_array prop_array;
  packetentities_data output;
} prop_parse_state;

static prop_value read_prop(prop_parse_state *state, demogobbler_sendprop *prop);

static void read_int(prop_parse_state *state, demogobbler_sendprop *prop, prop_value_inner *value) {
  if (prop->flag_unsigned) {
    value->unsigned_val = demogobbler_bitstream_read_uint(state->stream, prop->prop_numbits);
  } else {
    value->signed_val = demogobbler_bitstream_read_sint(state->stream, prop->prop_numbits);
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

static void read_vector2(prop_parse_state *state, demogobbler_sendprop *prop,
                         prop_value_inner *value) {
  value->v2_val =
      demogobbler_arena_allocate(state->a, sizeof(vector2_value), alignof(vector2_value));
  memset(value->v2_val, 0, sizeof(vector2_value));

  read_float(state, prop, &value->v2_val->x);
  read_float(state, prop, &value->v2_val->y);
}

static void read_string(prop_parse_state *state, demogobbler_sendprop *prop,
                        prop_value_inner *value) {
  const size_t dt_max_string_bits = 9;
  size_t len = bitstream_read_uint(state->stream, dt_max_string_bits);
  value->str_val = demogobbler_arena_allocate(state->a, len + 1, 1);
  bitstream_read_fixed_string(state->stream, value->str_val, len);
  value->str_val[len] = '\0';
}

static void read_array(prop_parse_state *state, demogobbler_sendprop *prop,
                       prop_value_inner *value) {
  value->arr_val = demogobbler_arena_allocate(state->a, sizeof(array_value), alignof(array_value));
  unsigned count =
      bitstream_read_uint(state->stream, highest_bit_index(prop->array_num_elements) + 1);
  value->arr_val->values = demogobbler_arena_allocate(state->a, sizeof(prop_value_inner) * count,
                                                      alignof(prop_value_inner));

  for (size_t i = 0; i < count; ++i) {
    prop_value temp = read_prop(state, prop->array_prop);
    value->arr_val->values[i] = temp.value;
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

static void parse_props_prot4(prop_parse_state *state) {
  parser *thisptr = state->thisptr;
  bitstream *stream = state->stream;
  edict *ent = state->ent;
  serverclass_data *datas = demogobbler_estate_serverclass_data(thisptr, ent->datatable_id);
  int i = -1;
  bool new_way = thisptr->demo_version.game != l4d && bitstream_read_bit(state->stream);

  while (true) {
    i = bitstream_read_field_index(state->stream, i, new_way);

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

static void parse_props_old(prop_parse_state *state) {
  parser *thisptr = state->thisptr;
  edict *ent = state->ent;
  serverclass_data *data = demogobbler_estate_serverclass_data(thisptr, ent->datatable_id);
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

static void parse_props(prop_parse_state *state, edict *ent, size_t index) {
  state->ent = ent;
  ent_update *update = state->output.ent_updates + index;
  demogobbler_va_clear(&state->prop_array);
  demo_version_data *demo_version = &state->thisptr->demo_version;
  if (demo_version->demo_protocol == 4) {
    parse_props_prot4(state);
  } else {
    parse_props_old(state);
  }

  if (state->prop_array.count_elements > 0) {
    update->prop_value_array_size = state->prop_array.count_elements;
    size_t bytes = sizeof(prop_value) * state->prop_array.count_elements;
    update->prop_value_array = demogobbler_arena_allocate(state->a, bytes, alignof(prop_value));
    memcpy(update->prop_value_array, state->prop_array.ptr, bytes);
  }
}

static int update_old_index(parser *thisptr, int oldI) {
  edict *ent = thisptr->state.entity_state.edicts + oldI;
  while (oldI <= MAX_EDICTS && !ent->exists) {
    ++oldI;
    ent = thisptr->state.entity_state.edicts + oldI;
  }

  return oldI;
}

static void read_explicit_deletes(prop_parse_state *state) {
  int max_updates = (state->stream->bitsize - state->stream->bitoffset) / (1 + MAX_EDICT_BITS);

  if (max_updates >= 0) {
    size_t bytes = sizeof(int) * max_updates;
    state->output.explicit_deletes = demogobbler_arena_allocate(state->a, bytes, alignof(int));
    memset(state->output.explicit_deletes, 0, bytes);
  }

  int index = 0;

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
    ++index;
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
  state.prop_array = demogobbler_va_create(props_array);

  unsigned bits = Q_log2(thisptr->state.entity_state.serverclass_count) + 1;

  if (!thisptr->state.entity_state.class_datas) {
    thisptr->error = true;
    thisptr->error_message = "Tried to parse packet entities with no flattened props";
  }

  int newI = -1;
  size_t i;

  for (i = 0; i < message->updated_entries && !thisptr->error && !stream.overflow; ++i) {
    newI += 1;
    if (thisptr->demo_version.demo_protocol >= 4) {
      newI += bitstream_read_ubitint(&stream);
    } else {
      newI += bitstream_read_ubitvar(&stream);
    }

    unsigned update_type = bitstream_read_uint(&stream, 2);

    state.output.ent_updates[i].update_type = update_type;
    state.output.ent_updates[i].ent_index = newI;

    if (newI >= MAX_EDICTS || newI < 0) {
      thisptr->error = true;
      thisptr->error_message = "Illegal entity index";
      goto end;
    }

    edict *ent = thisptr->state.entity_state.edicts + newI;
    state.output.ent_updates[i].ent = ent;

    if (update_type == 0) {
      // delta
      parse_props(&state, ent, i); 
    } else if (update_type == 2) {
      // enter pvs
      unsigned int handle_serial_number_bits = 10;
      ent->datatable_id = bitstream_read_uint(&stream, bits);
      ent->handle = demogobbler_bitstream_read_uint(&stream, handle_serial_number_bits);
      ent->exists = true;

      if (ent->datatable_id >= thisptr->state.entity_state.serverclass_count) {
        thisptr->error = true;
        thisptr->error_message = "Invalid class ID in svc_packetentities";
        goto end;
      }

      parse_props(&state, ent, i);
    } else if (update_type == 1) {
      // Leave PVS
      ent->in_pvs = false;
    } else {
      // Delete
      memset(ent, 0, sizeof(edict));
    }
  }

  if (message->is_delta && !thisptr->error && !stream.overflow) {
    read_explicit_deletes(&state);
  }

  if (!thisptr->error && !stream.overflow && thisptr->m_settings.packetentities_parsed_handler) {
    svc_packetentities_parsed parsed;
    memset(&parsed, 0, sizeof(parsed));
    parsed.data = state.output;
    parsed.orig = message;
    thisptr->m_settings.packetentities_parsed_handler(&thisptr->state, &parsed);
  }

end:;

  demogobbler_va_free(&state.prop_array);

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
    fprintf(stderr, "Failed at %d\n", CURRENT_DEBUG_INDEX);
#endif
  }
}
