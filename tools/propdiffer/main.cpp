#include "demogobbler.h"
#include "demogobbler/freddie.hpp"
#include <cstdio>
#include <stdarg.h>
#include <string.h>

const char* firstdemo = nullptr;
const char* seconddemo = nullptr;
using namespace freddie;

typedef void(*printFunc)(bool first, const char* msg, ...);
struct compare_props_args {
    const demo_t* first;
    const demo_t* second;
    printFunc func = nullptr;
};

static void print_msg(bool first, const char* format, ...) {
  va_list args;
  va_start(args, format);
  printf("%s: ", first ? firstdemo : seconddemo);
  vprintf(format, args);
  va_end(args);
}

static int32_t get_dt(const estate *state, const char *name, int32_t initial_guess) {
  if ((int32_t)state->serverclass_count > initial_guess &&
      strcmp(state->class_datas[initial_guess].dt_name, name) == 0) {
    return initial_guess;
  } else {
    for (int32_t i = 0; i < (int32_t)state->serverclass_count; ++i) {
      if (strcmp(name, state->class_datas[i].dt_name) == 0) {
        return i;
      }
    }
    return -1;
  }
}

#define ARGSPRINT(...)                                                                             \
  if (args->func)                                                                                  \
  args->func(__VA_ARGS__)

static bool prop_equal(const dg_sendprop* prop1, const char* prop_name)
{
  char prop1_name[64];
  dg_sendprop_name(prop1_name, sizeof(prop1_name), prop1);

  return strcmp(prop1_name, prop_name) == 0;
}

static bool prop_attributes_equal(const compare_props_args *args, const dg_sendprop* prop1, const dg_sendprop* prop2)
{
  char prop1_name[64];
  dg_sendprop_name(prop1_name, sizeof(prop1_name), prop1);

  bool equal = true;
#define COMPARE_ELEMENT(x) if(prop1->x != prop2->x) { \
    equal = false; \
    args->func(true, "\t\tprop attribute " #x " changed %u -> %u\n", prop1->x, prop2->x); \
  }

  COMPARE_ELEMENT(array_num_elements);
  COMPARE_ELEMENT(flag_cellcoord);
  COMPARE_ELEMENT(flag_cellcoordint);
  COMPARE_ELEMENT(flag_cellcoordlp);
  COMPARE_ELEMENT(flag_changesoften);
  COMPARE_ELEMENT(flag_collapsible);
  COMPARE_ELEMENT(flag_coord);
  COMPARE_ELEMENT(flag_coordmp);
  COMPARE_ELEMENT(flag_coordmplp);
  COMPARE_ELEMENT(flag_coordmpint);
  COMPARE_ELEMENT(flag_coord);
  COMPARE_ELEMENT(flag_insidearray);
  COMPARE_ELEMENT(flag_normal);
  COMPARE_ELEMENT(flag_noscale);
  COMPARE_ELEMENT(flag_proxyalwaysyes);
  COMPARE_ELEMENT(flag_rounddown);
  COMPARE_ELEMENT(flag_roundup);
  COMPARE_ELEMENT(flag_unsigned);
  COMPARE_ELEMENT(flag_xyze);

  return equal;
}

static dg_sendprop* find_prop(const dg_serverclass_data *data, const char* name, size_t initial_index)
{
  if(initial_index < data->prop_count)
  {
    if(prop_equal(data->props + initial_index, name))
    {
      return data->props + initial_index;
    }
  }

  for(size_t i=0; i < data->prop_count; ++i)
  {
    if(prop_equal(data->props + i, name))
      return data->props + i;
  }

  return nullptr;
}

static void compare_sendtable_props(const compare_props_args *args,
                                    const dg_serverclass_data *data1,
                                    const dg_serverclass_data *data2) {
  char buffer[64];
  for(size_t i=0; i < data1->prop_count; ++i)
  {
    dg_sendprop* first_prop = data1->props + i;
    dg_sendprop_name(buffer, sizeof(buffer), first_prop);
    dg_sendprop* prop = find_prop(data2, buffer, i);
    if(!prop)
    {
      args->func(true, "\tonly has %s at %lu\n", buffer, i);
      continue;
    }
  
    size_t index = prop - data2->props;
    if(index != i)
    {
      args->func(true, "\tprop %s index changed %lu -> %lu\n", buffer, i, index);
    }

    prop_attributes_equal(args, first_prop, prop);
  }

  for(size_t i=0; i < data2->prop_count; ++i)
  {
    dg_sendprop* first_prop = data2->props + i;
    dg_sendprop_name(buffer, sizeof(buffer), first_prop);
    dg_sendprop* prop = find_prop(data1, buffer, i);
    if(!prop)
    {
      args->func(false, "\tonly has %s at %lu\n", buffer, i);
      continue;
    }
  }
}

static void compare_sendtables(const compare_props_args *args, const estate *lhs,
                               const estate *rhs) {
  for (int32_t i = 0; i < (int32_t)lhs->serverclass_count; ++i) {
    const char *name = lhs->class_datas[i].dt_name;
    auto dt = get_dt(rhs, name, i);
    if (dt == -1) {
      ARGSPRINT(true, "[%d] %s only\n", i, name);
    } else if (dt != i) {
      ARGSPRINT(true, "[%d] %s index changed -> %d\n", i, name, dt);
      compare_sendtable_props(args, lhs->class_datas + i, rhs->class_datas + dt);
    } else {
      ARGSPRINT(true, "[%d] %s matches\n", i, name);
      compare_sendtable_props(args, lhs->class_datas + i, rhs->class_datas + dt);
    }
  }

  for (int32_t i = 0; i < (int32_t)rhs->serverclass_count; ++i) {
    const char *name = rhs->class_datas[i].dt_name;
    auto dt = get_dt(lhs, name, i);
    if (dt == -1) {
      ARGSPRINT(false, "[%d] %s only\n", i, name);
    }
  }
}

int main(int argc, char **argv) {
  if (argc <= 2) {
    printf("Usage: ddiff <example file> <input file>\n");
    return 0;
  }

  freddie::demo_t lhs;
  freddie::demo_t rhs;
  firstdemo = argv[1];
  seconddemo = argv[2];
  auto result = freddie::demo_t::parse_demo(&lhs, argv[1]);
  auto allocator = dg_arena_create_allocator(&lhs.arena);
  freddie::datatable_change_info change_info(allocator);

  if(result.error)
  {
    std::printf("error parsing 1st demo: %s\n", result.error_message);
    return 1;
  }

  result = freddie::demo_t::parse_demo(&rhs, argv[2]);

  if(result.error)
  {
    std::printf("error parsing 2nd demo: %s\n", result.error_message);
    return 1;
  }

  compare_props_args args;
  args.first = &lhs;
  args.second = &rhs;
  args.func = print_msg;
  //compare_props(&args);

  result = change_info.init(&lhs, &rhs);

  if(result.error)
  {
    std::printf("error creating change info %s\n", result.error_message);
    return 1;
  }

  change_info.print(true);

  return 0;
}