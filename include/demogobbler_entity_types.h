#pragma once

#include <stddef.h>

struct demogobbler_sendprop;
struct demogobbler_sendtable;

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
  size_t sendtables_count;
} estate;

struct demogobbler_parser_state {
  void *client_state;
  estate entity_state;
  const char *error_message;
  bool error;
};
