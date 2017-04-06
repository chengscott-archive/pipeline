#pragma once
#include <cstdint>

namespace IR {
    struct IR {
        char type;
    };

    struct R_type : IR {
        R_type() { type = 'R'; };
        R_type(const R_type& rhs) {
            type = 'R';
            opcode = rhs.opcode;
            rs = rhs.rs;
            rt = rhs.rt;
            rd = rhs.rd;
            shamt = rhs.shamt;
            funct = rhs.funct;
        }
    	uint32_t opcode, rs, rt, rd, shamt, funct;
    };

    struct I_type : IR {
        I_type() { type = 'I'; };
        I_type(const I_type& rhs) {
            type = 'I';
            opcode = rhs.opcode;
            rs = rhs.rs;
            rt = rhs.rt;
            C = rhs.C;
        }
    	uint32_t opcode, rs, rt, C;
    };

    struct J_type : IR {
        J_type() { type = 'J'; };
        J_type(const J_type& rhs) {
            type = 'J';
            opcode = rhs.opcode;
            C = rhs.C;
        }
    	uint32_t opcode, C;
    };

    struct S_type : IR {
        S_type() { type = 'S'; };
        S_type(const S_type& rhs) {
            type = 'S';
            opcode = rhs.opcode;
            C = rhs.C;
        }
    	uint32_t opcode, C;
    };

    char getType(const uint32_t&);
    R_type R_decode(const uint32_t&);
    I_type I_decode(const uint32_t&);
    J_type J_decode(const uint32_t&);
    S_type S_decode(const uint32_t&);
}
