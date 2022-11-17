#include "parser_usercmd.h"
#include <string.h>

#define READ_UINT(member, bits) out->member ## _exists = bitstream_read_bit(&stream); \
    if(out->member ## _exists) out->member = bitstream_read_uint(&stream, bits);

#define WRITE_UINT(member, bits) bitwriter_write_bit(thisptr, input->member ## _exists ); \
    if(input->member ## _exists ) bitwriter_write_uint(thisptr, input->member, bits);

#define READ_FLOAT(member) out->member ## _exists = bitstream_read_bit(&stream); \
    if(out->member ## _exists) out->member = bitstream_read_float(&stream);

#define WRITE_FLOAT(member) bitwriter_write_bit(thisptr, input->member ## _exists ); \
    if(input->member ## _exists ) bitwriter_write_float(thisptr, input->member);

dg_parse_result dg_parser_parse_usercmd(const dg_demver_data* version_data, const dg_usercmd *input, dg_usercmd_parsed* out) {
    dg_parse_result result;
    memset(&result, 0, sizeof(result));
    dg_bitstream stream = bitstream_create(input->data, input->size_bytes * 8);
    memset(out, 0, sizeof(*out));
    out->orig = input;

    if(input->size_bytes < 0) {
        result.error = true;
        result.error_message = "Usercmd length was negative";
        return result;
    }

    READ_UINT(command_number, 32);
    READ_UINT(tick_count, 32);

    READ_FLOAT(viewangle.x);
    READ_FLOAT(viewangle.y);
    READ_FLOAT(viewangle.z);
    READ_FLOAT(movement.side);
    READ_FLOAT(movement.forward);
    READ_FLOAT(movement.up);

    out->buttons_exists = bitstream_read_bit(&stream);
    if(out->buttons_exists) {
        out->buttons = bitstream_read_sint32(&stream);
    }

    READ_UINT(impulse, 8);
    READ_UINT(weapon_select, 11);

    if(out->weapon_select_exists) {
        READ_UINT(weapon_subtype, 6);
    }

    READ_UINT(mouse_dx, 16);
    READ_UINT(mouse_dy, 16);

    if(stream.overflow) {
        result.error = true;
        result.error_message = "Bitstream overflowed during usercmd parsing";
    } else if(stream.bitoffset != stream.bitsize) {
        unsigned int left = stream.bitsize - stream.bitoffset;
        out->left_overbits = bitstream_fork_and_advance(&stream, left);
    }

    return result;
}


void dg_bitwriter_write_usercmd(bitwriter* thisptr, dg_usercmd_parsed* input) {
    WRITE_UINT(command_number, 32);
    WRITE_UINT(tick_count, 32);

    WRITE_FLOAT(viewangle.x);
    WRITE_FLOAT(viewangle.y);
    WRITE_FLOAT(viewangle.z);
    WRITE_FLOAT(movement.side);
    WRITE_FLOAT(movement.forward);
    WRITE_FLOAT(movement.up);

    bitwriter_write_bit(thisptr, input->buttons_exists);
    if(input->buttons_exists) {
        bitwriter_write_sint32(thisptr, input->buttons);
    }

    WRITE_UINT(impulse, 8);
    WRITE_UINT(weapon_select, 11);

    if(input->weapon_select_exists) {
        WRITE_UINT(weapon_subtype, 6);
    }

    WRITE_UINT(mouse_dx, 16);
    WRITE_UINT(mouse_dy, 16);

    bitwriter_write_bitstream(thisptr, &input->left_overbits);
}
