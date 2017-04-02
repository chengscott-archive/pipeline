#pragma once
#include <cstdint>

namespace IR {
    struct R_type {
    	uint32_t opcode, rs, rt, rd, shamt, funct;
    };

    struct I_type {
    	uint32_t opcode, rs, rt, C;
    };

    struct J_type {
    	uint32_t opcode, C;
    };

    struct S_type {
    	uint32_t opcode, C;
    };

    char getType(const uint32_t&);
    R_type R_decode(const uint32_t&);
    I_type I_decode(const uint32_t&);
    J_type J_decode(const uint32_t&);
    S_type S_decode(const uint32_t&);
}
