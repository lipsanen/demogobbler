#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "header.h"
#include <stdbool.h>

struct demogoblin_settings
{
    func_demogoblin_handle_header header_handler;
};

typedef struct demogoblin_settings demogoblin_settings;


demogoblin_settings demogoblin_init_settings();
bool demogoblin_parse(const char* filepath, demogoblin_settings settings);

#ifdef __cplusplus
}
#endif