#include "demogoblin.h"
#include "stddef.h"

bool demogoblin_parse(const char* filepath, demogoblin_settings settings)
{
    return false;
}

demogoblin_settings demogoblin_init_settings()
{
    demogoblin_settings settings;
    settings.header_handler = NULL;
    return settings;
}
