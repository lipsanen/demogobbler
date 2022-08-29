#include "parser_packetentities.h"
#include "utils.h"
#include "vector_array.h"
#include <signal.h>
#include <string.h>

#ifdef DEBUG_BREAK_PROP
int CURRENT_DEBUG_INDEX = 0;
static int BREAK_INDEX = 1;
#endif

enum UpdateTypeFlags { flags_delta = 0, flags_leavepvs = 1, flags_enterpvs = 2, flags_delete = 3 };

enum UpdateType {
  EnterPVS = 0, // Entity came back into pvs, create new entity if one doesn't exist
  LeavePVS,     // Entity left pvs
  DeltaEnt,     // There is a delta for this entity.
  PreserveEnt,  // Entity stays alive but no delta ( could be LOD, or just unchanged )
  Finished,     // finished parsing entities successfully
  Failed,       // parsing error occured while reading entities
};

typedef struct {
  int m_nOldEntity;
  int m_nNewEntity;
  int m_nHeaderBase;
  int m_nHeaderCount;
  int m_UpdateFlags;
  bool m_bAsDelta;
  enum UpdateType m_UpdateType;
  bool is_entity;
} centityinfo;

typedef struct {
  parser *thisptr;
  bitstream *stream;
  edict *ent;
  vector_array prop_array;
  centityinfo entity_read_info;
  int server_class_bits;
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
      bitstream_read_uint(state->stream, Q_log2(prop->array_num_elements) + 1);

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
    int brk = 0; // Set a breakpoint here
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
  flattened_props *props = thisptr->state.entity_state.class_props + ent->datatable_id;
  int i = -1;

  bool new_way = thisptr->demo_version.game != l4d && bitstream_read_bit(state->stream);

  while (true) {
    i = bitstream_read_field_index(state->stream, i, new_way);

    if (i < -1 || i > (int)props->prop_count) {
      thisptr->error = true;
      thisptr->error_message = "Invalid prop index encountered";
    }

    if (i == -1 || thisptr->error || stream->overflow)
      break;

    read_prop(state, props->props + i);
  }
}

static void parse_props_old(prop_parse_state *state) {
  unsigned int datatable_id = state->ent->datatable_id;
  flattened_props *props = state->thisptr->state.entity_state.class_props + datatable_id;
  int i = -1;
  int old_index;

  while (bitstream_read_bit(state->stream)) {
    old_index = i;
    i += bitstream_read_ubitvar(state->stream) + 1;
    if (i < -1 || i > props->prop_count) {
      state->thisptr->error = true;
      state->thisptr->error_message = "Invalid prop index encountered";
    }

    if (i == -1 || state->thisptr->error || state->stream->overflow)
      break;

    read_prop(state, props->props + i);
  }
}

static void parse_props(prop_parse_state *state, edict *ent) {
  state->ent = ent;
  demo_version_data *demo_version = &state->thisptr->demo_version;
  if (demo_version->demo_protocol == 4) {
    parse_props_prot4(state);
  } else {
    parse_props_old(state);
  }
}

static void next_old_entity(prop_parse_state *state) {
  // Grab next old entity
  centityinfo *u = &state->entity_read_info;
  edict *edicts = state->thisptr->state.entity_state.edicts;
  if (u->m_bAsDelta) {
    u->m_nOldEntity += 1;
    while (u->m_nOldEntity < MAX_EDICTS && edicts[u->m_nOldEntity].exists) {
      u->m_nOldEntity += 1;
    }

    if (u->m_nOldEntity < 0) {
      u->m_nOldEntity = ENTITY_SENTINEL;
    }
  } else {
    u->m_nOldEntity = ENTITY_SENTINEL;
  }
}

static void cl_parsedeltaheader(prop_parse_state *state) {
  centityinfo *u = &state->entity_read_info;

  u->m_nNewEntity = u->m_nHeaderBase + 1 + bitstream_read_ubitvar(state->stream);
  u->m_nHeaderBase = u->m_nNewEntity;
  u->m_UpdateFlags = bitstream_read_uint(state->stream, 2);
  // 0 = delta
  // 1 = leave pvs
  // 2 = enter pvs
  // 3 = delete
}

static bool cl_determineupdatetype(prop_parse_state *state) {
  centityinfo *u = &state->entity_read_info;

  if (!u->is_entity) {
    // attempt at simplifying some parsing stuff, skips some extra steps
    u->m_UpdateType = Finished;
    return false;
  } else if (u->m_nNewEntity > u->m_nOldEntity) {
    u->m_UpdateType = PreserveEnt;
  } else {
    if (u->m_UpdateFlags == 2) {
      u->m_UpdateType = EnterPVS;
    } else if (u->m_UpdateFlags == 1 || u->m_UpdateFlags == 3) {
      u->m_UpdateType = LeavePVS;
    } else {
      u->m_UpdateType = DeltaEnt;
    }
  }

  return true;
}

#define SET_ERROR(msg)                                                                             \
  state->thisptr->error = true;                                                                    \
  state->thisptr->error_message = msg;                                                             \
  u->m_UpdateType = Failed;                                                                        \
  goto end;

static void read_enter_pvs(prop_parse_state *state) {
  centityinfo *u = &state->entity_read_info;
  const int networked_ehandle_bits = 10;
  edict *ent = state->thisptr->state.entity_state.edicts + state->entity_read_info.m_nNewEntity;

  ent->datatable_id = bitstream_read_uint(state->stream, state->server_class_bits);
  ent->handle = bitstream_read_uint(state->stream, networked_ehandle_bits);
  ent->exists = true;
  ent->in_pvs = true;

  parse_props(state, ent);

  if (u->m_nNewEntity == u->m_nOldEntity) {
    next_old_entity(state);
  }
}

static void read_leave_pvs(prop_parse_state *state) {
  centityinfo *u = &state->entity_read_info;
  edict *edict = state->thisptr->state.entity_state.edicts + u->m_nOldEntity;

  if (!u->m_bAsDelta) {
    SET_ERROR("LeavePVS on full update");
  }

  edict->in_pvs = false;

  if (u->m_UpdateFlags == flags_delete) {
    memset(edict, 0, sizeof(*edict));
  }

  next_old_entity(state);
end:;
}

static void read_delta_ent(prop_parse_state *state) {
  centityinfo *u = &state->entity_read_info;
  if(!state->entity_read_info.m_bAsDelta) {
    SET_ERROR("Delta entity in non-delta packet");
  }
  edict *ent = state->thisptr->state.entity_state.edicts + state->entity_read_info.m_nNewEntity;
  parse_props(state, ent);
  next_old_entity(state);
end:;
}

static void read_preserve_ent(prop_parse_state *state) {
  centityinfo *u = &state->entity_read_info;

  if (!u->m_bAsDelta) {
    SET_ERROR("PreserveEnt on full update");
  }

  if (u->m_nNewEntity >= MAX_EDICTS) {
    SET_ERROR("u.m_nNewEntity == MAX_EDICTS");
  }

  next_old_entity(state);
end:;
}

static void read_deletions(prop_parse_state *state) {
  edict *edicts = state->thisptr->state.entity_state.edicts;
  while (bitstream_read_bit(state->stream)) {
    int ent = bitstream_read_uint(state->stream, MAX_EDICT_BITS);

    if (ent >= MAX_EDICTS && ent < 0) {
      // Given that we only read MAX_EDICT_BITS this shouldnt happen ever?
      state->thisptr->error = true;
      state->thisptr->error_message = "Illegal entity index in explicit delete\n";
      return;
    }

    edict *edict = &edicts[ent];
    memset(edict, 0, sizeof(*edict));
  }
}

#undef SET_ERROR
#define SET_ERROR(msg)                                                                             \
  state.thisptr->error = true;                                                                     \
  state.thisptr->error_message = msg;                                                              \
  u->m_UpdateType = Failed;                                                                        \
  goto end;

void demogobbler_parse_packetentities(parser *thisptr,
                                      struct demogobbler_svc_packet_entities *message) {
  bitstream stream = message->data;
  prop_parse_state state;
  prop_value props_array[64];
  memset(&state, 0, sizeof(prop_parse_state));

  state.thisptr = thisptr;
  state.stream = &stream;
  state.entity_read_info.m_nHeaderBase = -1;
  state.entity_read_info.m_nHeaderCount = message->updated_entries;
  state.entity_read_info.m_nNewEntity = -1;
  state.entity_read_info.m_nOldEntity = -1;
  state.entity_read_info.m_UpdateType = PreserveEnt;
  state.entity_read_info.m_bAsDelta = message->is_delta;
  state.server_class_bits = Q_log2(thisptr->state.entity_state.serverclass_count) + 1;
  state.prop_array = demogobbler_va_create(props_array);

  next_old_entity(&state);
  centityinfo *u = &state.entity_read_info;

  while (u->m_UpdateType < Finished) {
    --u->m_nHeaderCount;
    u->is_entity = (u->m_nHeaderCount >= 0) ? true : false;

    if (u->is_entity) {
      cl_parsedeltaheader(&state);
    }

    if (u->m_nNewEntity >= MAX_EDICTS || u->m_nNewEntity < 0) {
      SET_ERROR("Illegal entity index");
    }

    u->m_UpdateType = PreserveEnt;

    while (u->m_UpdateType == PreserveEnt) {
      if (cl_determineupdatetype(&state)) {
        switch (u->m_UpdateType) {
        case EnterPVS:
          read_enter_pvs(&state);
          break;
        case LeavePVS:
          read_leave_pvs(&state);
          break;
        case DeltaEnt:
          read_delta_ent(&state);
          break;
        case PreserveEnt:
          read_preserve_ent(&state);
          break;
        default:
          state.thisptr->error = true;
          state.thisptr->error_message = "Invalid update type";
          break;
        }
      }
    }
  }

  if (message->is_delta && state.entity_read_info.m_UpdateType == Finished) {
    read_deletions(&state);
  }

end:
  if (stream.overflow && !thisptr->error) {
    thisptr->error = true;
    thisptr->error_message = "Stream overflowed in svc_packetentities\n";
  }

  if (!thisptr->error && u->m_nHeaderCount > 0) {
    thisptr->error = true;
    thisptr->error_message = "Read the wrong number of entries in svc_packetentities\n";
  }

  if (!thisptr->error && demogobbler_bitstream_bits_left(state.stream) > 0) {
    thisptr->error = true;
    thisptr->error_message = "Not all bits read from svc_packetentities";
  }

#ifdef DEBUG_BREAK_PROP
  if (thisptr->error) {
    fprintf(stderr, "Failed at %d\n", CURRENT_DEBUG_INDEX);
  }
#endif
}
