#include "parser_packetentities.h"
#include "bitstream.h"
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
  vector_array prop_array;
} prop_parse_state;

static void read_prop(prop_parse_state *state, demogobbler_sendprop *prop);

static void read_int(prop_parse_state *state, demogobbler_sendprop *prop) {
  if (prop->flag_unsigned) {
    demogobbler_bitstream_read_uint(state->stream, prop->prop_numbits);
  } else {
    demogobbler_bitstream_read_sint(state->stream, prop->prop_numbits);
  }
}

static void read_float(prop_parse_state *state, demogobbler_sendprop *prop) {
  bitstream *stream = state->stream;
  if (prop->flag_coord) {
    demogobbler_bitstream_read_bitcoord(stream);
  } else if (prop->flag_coordmp) {
    demogobbler_bitstream_read_bitcoordmp(stream, false, false);
  } else if (prop->flag_coordmplp) {
    demogobbler_bitstream_read_bitcoordmp(stream, false, true);
  } else if (prop->flag_coordmpint) {
    demogobbler_bitstream_read_bitcoordmp(stream, true, false);
  } else if (prop->flag_noscale) {
    demogobbler_bitstream_read_float(stream);
  } else if (prop->flag_normal) {
    demogobbler_bitstream_read_bitnormal(stream);
  } else if (prop->flag_cellcoord) {
    demogobbler_bitstream_read_bitcellcoord(stream, false, false, prop->prop_numbits);
  } else if (prop->flag_cellcoordlp) {
    demogobbler_bitstream_read_bitcellcoord(stream, false, true, prop->prop_numbits);
  } else if (prop->flag_cellcoordint) {
    demogobbler_bitstream_read_bitcellcoord(stream, true, false, prop->prop_numbits);
  } else {
    unsigned int value = demogobbler_bitstream_read_uint(stream, prop->prop_numbits);
    float scale = (float)value / ((1 << prop->prop_numbits) - 1);
    float val = prop->prop_.low_value + (prop->prop_.high_value - prop->prop_.low_value) * scale;
  }
}

static void read_vector3(prop_parse_state *state, demogobbler_sendprop *prop) {
  read_float(state, prop);
  read_float(state, prop);

  if (prop->flag_normal) {
    bool sign = bitstream_read_bit(state->stream);
  } else {
    read_float(state, prop);
  }
}

static void read_vector2(prop_parse_state *state, demogobbler_sendprop *prop) {
  read_float(state, prop);
  read_float(state, prop);
}

static void read_string(prop_parse_state *state, demogobbler_sendprop *prop) {
  const size_t dt_max_string_bits = 9;
  size_t len = bitstream_read_uint(state->stream, dt_max_string_bits);
  bitstream_read_fixed_string(state->stream, NULL, len);
}

static void read_array(prop_parse_state *state, demogobbler_sendprop *prop) {
  unsigned count =
      bitstream_read_uint(state->stream, highest_bit_index(prop->array_num_elements) + 1);

  for (size_t i = 0; i < count; ++i) {
    read_prop(state, prop->array_prop);
  }
}

static void read_prop(prop_parse_state *state, demogobbler_sendprop *prop) {
#ifdef DEBUG_BREAK_PROP
  ++CURRENT_DEBUG_INDEX;
#endif

#ifdef DEBUG_BREAK_PROP
  if (BREAK_INDEX == CURRENT_DEBUG_INDEX) {
    ; // Set a breakpoint here
  }
#endif

  if (prop->proptype == sendproptype_array) {
    read_array(state, prop);
  } else if (prop->proptype == sendproptype_vector3) {
    read_vector3(state, prop);
  } else if (prop->proptype == sendproptype_vector2) {
    read_vector2(state, prop);
  } else if (prop->proptype == sendproptype_float) {
    read_float(state, prop);
  } else if (prop->proptype == sendproptype_string) {
    read_string(state, prop);
  } else if (prop->proptype == sendproptype_int) {
    read_int(state, prop);
  } else {
    state->thisptr->error = true;
    state->thisptr->error_message = "Got an unknown prop type in read_prop";
  }
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

    if (i < -1 || i > (int)datas->prop_count) {
      thisptr->error = true;
      thisptr->error_message = "Invalid prop index encountered";
    }

    if (i == -1 || thisptr->error || stream->overflow)
      break;

    read_prop(state, datas->props + i);
  }
}

static void parse_props_old(prop_parse_state *state) {
  parser *thisptr = state->thisptr;
  edict *ent = state->ent;
  unsigned int datatable_id = state->ent->datatable_id;
  serverclass_data *data = demogobbler_estate_serverclass_data(thisptr, ent->datatable_id);
  int i = -1;
  int old_index;

  while (bitstream_read_bit(state->stream)) {
    old_index = i;
    i += bitstream_read_ubitvar(state->stream) + 1;
    if (i < -1 || i > data->prop_count) {
      state->thisptr->error = true;
      state->thisptr->error_message = "Invalid prop index encountered";
    }

    if (i == -1 || state->thisptr->error || state->stream->overflow)
      break;

    read_prop(state, data->props + i);
  }
}

static void parse_props(prop_parse_state* state, edict *ent) {
  state->ent = ent;
  demo_version_data* demo_version = &state->thisptr->demo_version;
  if (demo_version->demo_protocol == 4) {
    parse_props_prot4(state);
  } else {
    parse_props_old(state);
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

static void read_explicit_deletes(prop_parse_state* state) {
  while(bitstream_read_bit(state->stream)) {
    unsigned delete_index = bitstream_read_uint(state->stream, MAX_EDICT_BITS);
    edict *ent = state->thisptr->state.entity_state.edicts + delete_index;
    memset(ent, 0, sizeof(edict));
  }
}

void demogobbler_parse_packetentities(parser *thisptr,
                                      struct demogobbler_svc_packet_entities *message) {
  bitstream stream = message->data;
  prop_parse_state state;
  state.thisptr = thisptr;
  state.stream = &stream;

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

    if (newI >= MAX_EDICTS || newI < 0) {
      thisptr->error = true;
      thisptr->error_message = "Illegal entity index";
      goto end;
    }

    if (update_type == 0 || update_type == 2) {
      // delta
      edict *ent = thisptr->state.entity_state.edicts + newI;
      if (update_type == 0) { // delta
        parse_props(&state, ent);
      } else {  // enter pvs
        unsigned int handle_serial_number_bits = 10;
        ent->datatable_id = bitstream_read_uint(&stream, bits);
        ent->handle = demogobbler_bitstream_read_uint(&stream, handle_serial_number_bits);
        ent->exists = true;

        if (ent->datatable_id >= thisptr->state.entity_state.serverclass_count) {
          thisptr->error = true;
          thisptr->error_message = "Invalid class ID in svc_packetentities";
          goto end;
        }

        parse_props(&state, ent);
      }
    } else {
      edict *ent = thisptr->state.entity_state.edicts + newI;
      if (update_type == 1) {
        ent->in_pvs = false;           // Leave PVS
      } else { // update_type == 3
        memset(ent, 0, sizeof(edict)); // Delete
      }
    }
  }

  if(message->is_delta && !thisptr->error && !stream.overflow) {
    read_explicit_deletes(&state);
  }

end:;

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
