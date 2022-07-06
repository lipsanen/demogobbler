#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct demogoblin_header demogoblin_header;

struct demogoblin_header
{
    int empty;
};

typedef void(*func_demogoblin_handle_header)(demogoblin_header* header);

#ifdef __cplusplus
}
#endif
