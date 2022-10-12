#pragma once

#include "demogobbler/arena.h"
#include "demogobbler/floats.h"
#include <stddef.h>

struct dg_sendprop;
struct dg_sendtable;
struct dg_svc_packet_entities;
struct dg_serverclass;

typedef struct {
  const char *str;
  size_t value;
} dg_hashtable_entry;

typedef struct {
  dg_hashtable_entry *arr;
  size_t max_items;
  size_t item_count;
} dg_hashtable;

typedef struct {
  struct dg_sendprop *exclude_prop;
} dg_pes_entry;

typedef struct {
  dg_pes_entry *arr;
  size_t max_items;
  size_t item_count;
} dg_pes;

struct dg_array_value;
struct dg_vector2_value;
struct dg_vector3_value;
struct dg_string_value;
typedef struct dg_array_value dg_array_value;
typedef struct dg_vector2_value dg_vector2_value;
typedef struct dg_vector3_value dg_vector3_value;
typedef struct dg_string_value dg_string_value;

typedef struct {
  union {
    uint32_t unsigned_val;
    int32_t signed_val;
    dg_bitcoord bitcoord_val;
    dg_bitcoordmp bitcoordmp_val;
    dg_bitcellcoord bitcellcoord_val;
    dg_bitnormal bitnormal_val;
    float float_val;
    struct dg_string_value *str_val; // strings
    // vector and array types, use pointer here to reduce the size of the prop_value struct
    struct dg_array_value *arr_val;
    struct dg_vector2_value *v2_val;
    struct dg_vector3_value *v3_val;
  };
} dg_prop_value_inner;

typedef struct {
  dg_prop_value_inner value; // Actual value
  struct dg_sendprop *prop;  // Pointer to sendprop containing all the metadata
} prop_value;

struct dg_string_value {
  char *str;
  size_t len;
};

struct dg_vector2_value {
  dg_prop_value_inner x, y;
};

struct dg_vector3_value {
  dg_prop_value_inner x, y;
  union {
    bool sign;
    dg_prop_value_inner z;
  };
};

struct dg_array_value {
  dg_prop_value_inner *values;
  size_t array_size;
};

typedef struct {
  int ent_index;
  int datatable_id;
  int handle;
  size_t update_type;
  prop_value *prop_value_array;
  size_t prop_value_array_size;
  bool new_way;
} dg_ent_update;

typedef struct {
  dg_ent_update *ent_updates;
  size_t ent_updates_count;
  int *explicit_deletes;
  size_t explicit_deletes_count;
} dg_packetentities_data;

typedef struct {
  dg_packetentities_data data;
  struct dg_svc_packet_entities *orig;
} dg_svc_packetentities_parsed;

struct dg_epropnode;
typedef struct dg_epropnode dg_epropnode;

struct dg_epropnode {
  dg_epropnode *next;
  dg_prop_value_inner value;
  int16_t index;
};

typedef struct {
  dg_epropnode *head;
} dg_eproplist;

typedef struct {
  dg_prop_value_inner *values;
  uint16_t *next_prop_indices;
  uint16_t prop_count;
} dg_eproparr;

typedef struct {
  int handle;
  int datatable_id;
  bool in_pvs;
  bool exists;
  bool explicitly_deleted;
#ifdef DEMOGOBBLER_USE_LINKED_LIST_PROPS
  dg_eproplist props;
#else
  dg_eproparr props;
#endif
} dg_edict;

typedef struct {
  struct dg_sendprop *props;
  size_t prop_count;
  const char *dt_name;
} dg_serverclass_data;

typedef struct {
  dg_serverclass_data *class_datas;
  struct dg_sendtable *sendtables;
  struct dg_serverclass *serverclasses;
  dg_edict *edicts;
  uint32_t sendtable_count;
  uint32_t serverclass_count;
  dg_arena memory_arena;
  bool should_store_props;
} estate;

struct dg_parser_state {
  void *client_state;
  estate entity_state;
  const char *error_message;
  bool error;
};
