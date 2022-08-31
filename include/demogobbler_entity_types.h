#pragma once

#include "demogobbler_floats.h"
#include <stddef.h>

struct demogobbler_sendprop;
struct demogobbler_sendtable;
struct demogobbler_svc_packet_entities;

typedef struct {
  int handle;
  int datatable_id;
  bool in_pvs;
  bool exists;
} edict;

typedef struct {
  struct demogobbler_sendprop *props;
  size_t prop_count;
} flattened_props;

typedef struct {
  flattened_props* class_props;
  struct demogobbler_sendtable* sendtables;
  edict* edicts;
  uint32_t sendtable_count;
  uint32_t serverclass_count;
} estate;

struct demogobbler_parser_state {
  void *client_state;
  estate entity_state;
  const char *error_message;
  bool error;
};

struct prop_array_value;
typedef struct prop_array_value prop_array_value;

typedef struct{
  union {
    uint32_t unsigned_val;
    int32_t signed_val;
    bitcoord bitcoord_val;
    demogobbler_bitcoordmp bitcoordmp_val;
    demogobbler_bitcellcoord bitcellcoord_val;
    demogobbler_bitnormal bitnormal_val;
    float float_val;
    char* str_val; // strings
    // vector and array types, use pointer here to reduce the size of the prop_value struct
    struct prop_array_value* arr_val;
  };
  demogobbler_sendprop* prop; // Pointer to sendprop containing all the metadata
} prop_value;

struct prop_array_value {
  prop_value* values;
  size_t array_size;
};

typedef struct {
  size_t ent_index;
  size_t update_type;
  prop_value* prop_value_array;
  size_t prop_value_array_size;
} ent_update;

typedef struct {
  ent_update* ent_updates;
  size_t ent_updates_count;
  int* explicit_deletes;
  size_t explicit_deletes_count;
  struct demogobbler_svc_packet_entities* orig;
} svc_packetentities_parsed;
