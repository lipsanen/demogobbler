#pragma once

#include "demogobbler.h"
#include "filereader.h"
#include "stdbool.h"
#include "stdio.h"

typedef struct parser parser;

struct parser
{
    demogobbler_settings m_settings;
    filereader m_reader;
};

void parser_init(parser* thisptr, demogobbler_settings* settings);
void parser_parse(parser* thisptr, const char* filepath);
void _parse_header(parser* thisptr);
void _parser_mainloop(parser* thisptr);
