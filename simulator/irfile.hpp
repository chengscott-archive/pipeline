#pragma once
#include <string>

namespace IR {
    char getType(const uint32_t rhs) {
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

    std::string getFunctName(const uint32_t funct) {
        switch (funct) {
            case 0x20: return "ADD";
            case 0x21: return "ADDU";
            case 0x22: return "SUB";
            case 0x24: return "AND";
            case 0x25: return "OR";
            case 0x26: return "XOR";
            case 0x27: return "NOR";
            case 0x28: return "NAND";
            case 0x2A: return "SLT";
            case 0x00: return "SLL";
            case 0x02: return "SRL";
            case 0x03: return "SRA";
            case 0x08: return "JR";
            case 0x18: return "MULT";
            case 0x19: return "MULTU";
            case 0x10: return "MFHI";
            case 0x12: return "MFLO";
        }
        return "";
    }

    std::string getOpName(const uint32_t instr) {
        if (instr == 0) return "NOP";
        const uint32_t opcode = (instr >> 26) & 0x3f;
        switch (opcode) {
            case 0x0: return getFunctName(instr & 0x3f);
            case 0x2: return "J";
            case 0x3: return "JAL";
            case 0x3f: return "HALT";
            case 0x08: return "ADDI";
            case 0x09: return "ADDIU";
            case 0x23: return "LW";
            case 0x21: return "LH";
            case 0x25: return "LHU";
            case 0x20: return "LB";
            case 0x24: return "LBU";
            case 0x2B: return "SW";
            case 0x29: return "SH";
            case 0x28: return "SB";
            case 0x0F: return "LUI";
            case 0x0C: return "ANDI";
            case 0x0D: return "ORI";
            case 0x0E: return "NORI";
            case 0x0A: return "SLTI";
            case 0x04: return "BEQ";
            case 0x05: return "BNE";
            case 0x07: return "BGTZ";
        }
        return "";
    }
}
