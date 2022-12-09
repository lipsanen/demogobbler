#pragma once

// Freddie is a higher level C++ API for demo related stuff

#include "arena.h"
#include "demogobbler.h"
#include <stddef.h>
#include <variant>
#include <vector>

namespace freddie
{
    dg_parse_result splice_demos(const char *output_path, const char **demo_paths,
                                        size_t demo_count);

    typedef std::variant<packet_parsed*, dg_customdata*, dg_datatables_parsed*, dg_stringtables_parsed*, dg_consolecmd*, dg_synctick*, dg_usercmd*, dg_stop*> demo_packet;

    struct demo
    {
        demo();
        ~demo();
        dg_arena arena;
        dg_demver_data demver_data;
        dg_header header;
        std::vector<demo_packet> packets;

        template<typename T>
        T* copy_packet(T* orig)
        {
            T* packet = (T*)dg_arena_allocate(&arena, sizeof(T), alignof(T));
            *packet = *orig;
            return packet;
        }

        static dg_parse_result parse_demo(demo* output, void* stream, dg_input_interface interface);
        dg_parse_result write_demo(void* stream, dg_output_interface interface);
    };
}
