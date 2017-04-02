#include "irfile.hpp"

char IR::getType(const uint32_t& rhs) {
    uint32_t opcode = (rhs >> 26) & 0x3f;
    switch (opcode) {
        case 0x0:
            return 'R';
        case 0x2: case 0x3:
            return 'J';
        case 0x3f:
            return 'S';
        case 0x08: case 0x09: case 0x23: case 0x21: case 0x25: case 0x20:
        case 0x24: case 0x2B: case 0x29: case 0x28: case 0x0F: case 0x0C:
        case 0x0D: case 0x0E: case 0x0A: case 0x04: case 0x05: case 0x07:
            return 'I';
    }
    return 'F';
}

IR::R_type IR::R_decode(const uint32_t& rhs) {
    R_type ret;
    ret.opcode = (rhs >> 26) & 0x3f;
    ret.rs = (rhs >> 21) & 0x1f;
    ret.rt = (rhs >> 16) & 0x1f;
    ret.rd = (rhs >> 11) & 0x1f;
    ret.shamt = (rhs >> 6) & 0x1f;
    ret.funct = rhs & 0x3f;
    return ret;
}

IR::I_type IR::I_decode(const uint32_t& rhs) {
    I_type ret;
    ret.opcode = (rhs >> 26) & 0x3f;
    ret.rs = (rhs >> 21) & 0x1f;
    ret.rt = (rhs >> 16) & 0x1f;
    ret.C = rhs & 0xffff;
    return ret;
}

IR::J_type IR::J_decode(const uint32_t& rhs) {
    J_type ret;
    ret.opcode = (rhs >> 26) & 0x3f;
    ret.C = rhs & 0x3ffffff;
    return ret;
}

IR::S_type IR::S_decode(const uint32_t& rhs) {
    S_type ret;
    ret.opcode = (rhs >> 26) & 0x3f;
    ret.C = rhs & 0x3ffffff;
    return ret;
}
