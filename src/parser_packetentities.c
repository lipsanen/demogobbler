#include "parser_packetentities.h"
#include "utils.h"
#include <signal.h>
#include <string.h>

#ifdef DEBUG_BREAK_PROP
static int CURRENT_DEBUG_INDEX = 0;
static int BREAK_INDEX = 20;
#endif

static void read_prop(parser *thisptr, bitstream *stream, edict *ent, demogobbler_sendprop *prop);

static void read_int(bitstream *stream, edict *ent, demogobbler_sendprop *prop) {
  if (prop->flag_unsigned) {
    demogobbler_bitstream_read_uint(stream, prop->prop_numbits);
  } else {
    demogobbler_bitstream_read_sint(stream, prop->prop_numbits);
  }
}

static void read_float(bitstream *stream, edict *ent, demogobbler_sendprop *prop) {
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

static void read_vector3(bitstream *stream, edict *ent, demogobbler_sendprop *prop) {
  read_float(stream, ent, prop);
  read_float(stream, ent, prop);

  if (prop->flag_normal) {
    bool sign = bitstream_read_bit(stream);
  } else {
    read_float(stream, ent, prop);
  }
}

static void read_vector2(bitstream *stream, edict *ent, demogobbler_sendprop *prop) {
  read_float(stream, ent, prop);
  read_float(stream, ent, prop);
}

static void read_string(bitstream *stream, edict *ent, demogobbler_sendprop *prop) {
  const size_t dt_max_string_bits = 9;
  size_t len = bitstream_read_uint(stream, dt_max_string_bits);
  bitstream_read_fixed_string(stream, NULL, len);
}

static void read_array(parser *thisptr, bitstream *stream, edict *ent, demogobbler_sendprop *prop) {
  unsigned count = bitstream_read_uint(stream, highest_bit_index(prop->array_num_elements) + 1);

  for (size_t i = 0; i < count; ++i) {
    read_prop(thisptr, stream, ent, prop->array_prop);
  }
}

static void read_prop(parser *thisptr, bitstream *stream, edict *ent, demogobbler_sendprop *prop) {
#ifdef DEBUG_BREAK_PROP
    ++CURRENT_DEBUG_INDEX;
#endif

#ifdef DEBUG_BREAK_PROP
  if (BREAK_INDEX == CURRENT_DEBUG_INDEX) {
        int brk = 0; // Set a breakpoint here
      }
#endif

    if (prop->proptype == sendproptype_array) {
      read_array(thisptr, stream, ent, prop);
    } else if (prop->proptype == sendproptype_vector3) {
      read_vector3(stream, ent, prop);
    } else if (prop->proptype == sendproptype_vector2) {
      read_vector2(stream, ent, prop);
    } else if (prop->proptype == sendproptype_float) {
      read_float(stream, ent, prop);
    } else if (prop->proptype == sendproptype_string) {
      read_string(stream, ent, prop);
    } else if (prop->proptype == sendproptype_int) {
      read_int(stream, ent, prop);
    } else {
      thisptr->error = true;
      thisptr->error_message = "Got an unknown prop type in read_prop";
    }
}

static void parse_props_prot4(parser *thisptr, bitstream *stream, edict *ent) {
  flattened_props *props = thisptr->state.entity_state.class_props + ent->datatable_id;
  int i = -1;
  int old_index;

  bool new_way = thisptr->demo_version.game != l4d && bitstream_read_bit(stream);

  while (true) {
    old_index = i;
    i = bitstream_read_field_index(stream, i, new_way);

    if (i < -1 || i > (int)props->prop_count) {
      thisptr->error = true;
      thisptr->error_message = "Invalid prop index encountered";
    }

    if (i == -1 || thisptr->error || stream->overflow)
      break;

    read_prop(thisptr, stream, ent, props->props + i);
  }
}

static void parse_props_old(parser *thisptr, bitstream *stream, edict *ent) {
  unsigned int datatable_id = ent->datatable_id;
  flattened_props *props = thisptr->state.entity_state.class_props + datatable_id;
  int i = -1;
  int old_index;

  while (bitstream_read_bit(stream)) {
    old_index = i;
    i += bitstream_read_ubitvar(stream) + 1;
    if (i < -1 || i > props->prop_count) {
      thisptr->error = true;
      thisptr->error_message = "Invalid prop index encountered";
    }

    if (i == -1 || thisptr->error || stream->overflow)
      break;

    read_prop(thisptr, stream, ent, props->props + i);
  }
}

static void parse_props(parser *thisptr, bitstream *stream, edict *ent) {
  if (thisptr->demo_version.demo_protocol == 4) {
    parse_props_prot4(thisptr, stream, ent);
  } else {
    parse_props_old(thisptr, stream, ent);
  }
}

void demogobbler_parse_packetentities(parser *thisptr,
                                      struct demogobbler_svc_packet_entities *message) {
  bitstream stream = message->data;
  unsigned bits = highest_bit_index(thisptr->state.entity_state.sendtables_count);

  if (!thisptr->state.entity_state.class_props) {
    thisptr->error = true;
    thisptr->error_message = "Tried to parse packet entities with no flattened props";
  }

  int oldI = -1, newI = -1;
  size_t i;

  for (i = 0; i < message->updated_entries && !thisptr->error && !stream.overflow; ++i) {
    newI += 1;
    if (thisptr->demo_version.demo_protocol >= 4) {
      newI += bitstream_read_ubitint(&stream);
    } else {
      newI += bitstream_read_ubitvar(&stream);
    }

    if (newI >= MAX_EDICTS || newI < 0) {
      thisptr->error = true;
      thisptr->error_message = "Illegal entity index";
      goto end;
    }

    unsigned update_type = bitstream_read_uint(&stream, 2);
    edict *ent = thisptr->state.entity_state.edicts + newI;

    if (update_type == 0) {
      // delta
      parse_props(thisptr, &stream, ent);
    } else if (update_type == 2) {
      // enter PVS
      unsigned int handle_serial_number_bits = 10;
      ent->datatable_id = bitstream_read_uint(&stream, bits);
      ent->handle = demogobbler_bitstream_read_uint(&stream, handle_serial_number_bits);

      if (ent->datatable_id >= thisptr->state.entity_state.sendtables_count) {
        thisptr->error = true;
        thisptr->error_message = "Invalid class ID in svc_packetentities";
        goto end;
      }

      parse_props(thisptr, &stream, ent);

    } else if (update_type == 1) {
      ent->in_pvs = false; // Leave PVS
    } else {
      memset(ent, 0, sizeof(edict)); // Delete
    }
  }
end:;

  if(stream.overflow && !thisptr->error) {
    thisptr->error = true;
    thisptr->error_message = "Stream overflowed in svc_packetentities\n";
  }

  if(!thisptr->error && i != message->updated_entries) {
    thisptr->error = true;
    thisptr->error_message = "Read the wrong number of entries in svc_packetentities\n";
  }

  if(thisptr->error) {
#ifdef DEBUG_BREAK_PROP
      fprintf(stderr, "Failed at %d\n", CURRENT_DEBUG_INDEX);
#endif
  }
}
