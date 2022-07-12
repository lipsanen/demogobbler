#include "gtest/gtest.h"
#include "demogobbler.h"

TEST(Basic, InitWorks)
{
    demogobbler_parser parser;
    demogobbler_settings settings;
    demogobbler_settings_init(&settings);
    demogobbler_parser_init(&parser, &settings);
}
