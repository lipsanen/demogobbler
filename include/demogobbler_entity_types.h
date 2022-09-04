#pragma once

#include "demogobbler_floats.h"
#include <stddef.h>

struct demogobbler_sendprop;
struct demogobbler_sendtable;
struct demogobbler_svc_packet_entities;
struct demogobbler_serverclass;

typedef struct {
  const char* str;
  size_t value;
} hashtable_entry;

typedef struct {
  hashtable_entry* arr;
  size_t max_items;
  size_t item_count;
} hashtable;

typedef struct {
  struct demogobbler_sendprop* exclude_prop;
} prop_exclude_entry;

typedef struct {
  prop_exclude_entry* arr;
  size_t max_items;
  size_t item_count;
} prop_exclude_set;

typedef struct {
  int handle;
  int datatable_id;
  bool in_pvs;
  bool exists;
} edict;

typedef struct {
  struct demogobbler_sendprop *props;
  size_t prop_count;
  const char* dt_name;
} serverclass_data;

typedef struct {
  serverclass_data* class_datas;
  struct demogobbler_sendtable* sendtables;
  struct demogobbler_serverclass* serverclasses;
  edict* edicts;
  uint32_t sendtable_count;
  uint32_t serverclass_count;
  prop_exclude_set excluded_props;
  hashtable dts_with_excludes;
  hashtable dt_hashtable;
} estate;

struct demogobbler_parser_state {
  void *client_state;
  estate entity_state;
  const char *error_message;
  bool error;
};

struct array_value;
struct vector2_value;
struct vector3_value;
struct string_value;
typedef struct array_value array_value;
typedef struct vector2_value vector2_value;
typedef struct vector3_value vector3_value;
typedef struct string_value string_value;

typedef struct {
  union {
    uint32_t unsigned_val;
    int32_t signed_val;
    bitcoord bitcoord_val;
    demogobbler_bitcoordmp bitcoordmp_val;
    demogobbler_bitcellcoord bitcellcoord_val;
    demogobbler_bitnormal bitnormal_val;
    float float_val;
    struct string_value* str_val; // strings
    // vector and array types, use pointer here to reduce the size of the prop_value struct
    struct array_value* arr_val;
    struct vector2_value* v2_val;
    struct vector3_value* v3_val;
  };
} prop_value_inner;

typedef struct{
  prop_value_inner value; // Actual value
  struct demogobbler_sendprop* prop; // Pointer to sendprop containing all the metadata
} prop_value;

struct string_value {
  char* str;
  size_t len;
};

struct vector2_value {
  prop_value_inner x, y;
};

struct vector3_value {
  prop_value_inner x, y;
  union {
    bool sign;
    prop_value_inner z;
  };
};

struct array_value {
  prop_value_inner* values;
  size_t array_size;
};

typedef struct {
  int ent_index;
  size_t update_type;
  edict* ent;
  prop_value* prop_value_array;
  size_t prop_value_array_size;
  bool new_way;
} ent_update;

typedef struct {
  ent_update* ent_updates;
  size_t ent_updates_count;
  int* explicit_deletes;
  size_t explicit_deletes_count;
} packetentities_data;

typedef struct {
  packetentities_data data;
  struct demogobbler_svc_packet_entities* orig;
} svc_packetentities_parsed;
