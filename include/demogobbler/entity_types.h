#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "demogobbler/allocator.h"
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
typedef struct dg_float dg_float;

enum dg_proptype { dg_float_bitcoord, dg_float_bitcoordmp, dg_float_bitcellcoord, dg_float_bitnormal, dg_float_noscale, dg_float_bitcoordmplp,
dg_float_bitcoordmpint, dg_float_bitcellcoordlp, dg_float_bitcellcoordint, dg_float_unsigned, dg_int_varuint32, dg_int_unsigned, dg_int_signed };

typedef struct {
  union {
    uint32_t unsigned_val;
    int32_t signed_val;
    float float_val;
    dg_bitcoord bitcoord_val;
    dg_bitcoordmp bitcoordmp_val;
    dg_bitcellcoord bitcellcoord_val;
    dg_bitnormal bitnormal_val;
    struct dg_string_value *str_val; // strings
    // vector and array types, use pointer here to reduce the size of the prop_value struct
    struct dg_array_value *arr_val;
    struct dg_vector2_value *v2_val;
    struct dg_vector3_value *v3_val;
  };
  unsigned type : 8;
  unsigned prop_numbits : 7;
  unsigned array_num_elements : 10;
  unsigned proptype : 6;
} dg_prop_value_inner;

typedef struct {
  dg_prop_value_inner value; // Actual value
  uint32_t prop_index;
  //struct dg_sendprop *prop;  // Pointer to sendprop containing all the metadata
} prop_value;

struct dg_string_value {
  char *str;
  size_t len;
};

struct dg_vector2_value {
  dg_prop_value_inner x, y;
};

enum dg_vector3_sign { dg_vector3_sign_no, dg_vector3_sign_pos, dg_vector3_sign_neg };

struct dg_vector3_value {
  dg_prop_value_inner x, y, z;
  enum dg_vector3_sign _sign;
};

struct dg_array_value {
  dg_prop_value_inner *values;
  size_t array_size;
};

struct dg_ent_update {
  int ent_index;
  int datatable_id;
  int handle;
  size_t update_type;
  prop_value *prop_value_array;
  size_t prop_value_array_size;
  bool new_way;
};

typedef struct dg_ent_update dg_ent_update;

typedef struct {
  dg_ent_update *ent_updates;
  size_t ent_updates_count;
  int *explicit_deletes;
  size_t explicit_deletes_count;
  uint32_t serverclass_bits;
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

struct entity_parse_scrap {
  dg_pes excluded_props;
  dg_hashtable dts_with_excludes;
  dg_hashtable dt_hashtable;
};

typedef struct entity_parse_scrap entity_parse_scrap;

struct estate {
  dg_serverclass_data *class_datas;
  struct dg_sendtable *sendtables;
  struct dg_serverclass *serverclasses;
  dg_edict *edicts;
  uint32_t sendtable_count;
  uint32_t serverclass_count;
  entity_parse_scrap scrap;
  bool should_store_props;
};

typedef struct estate estate;

struct dg_stringtable_data {
  uint32_t max_entries;
  uint32_t user_data_size_bits;
  uint32_t flags;
  bool user_data_fixed_size;
};

typedef struct dg_stringtable_data dg_stringtable_data;

enum { MAX_STRINGTABLES = 18 };

struct dg_parser_state {
  void *client_state;
  estate entity_state;
  dg_stringtable_data stringtables[MAX_STRINGTABLES];
  uint32_t stringtables_count;
  const char *error_message;
  bool error;
};

#ifdef __cplusplus
}
#endif
