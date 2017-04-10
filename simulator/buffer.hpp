#pragma once

struct IFID_Buffer {
    uint32_t instr;
};

struct IDEX_Buffer {
    char type;
    uint32_t instr, opcode, rs, rt, rd, shamt, funct, C;
    bool RegDst, ALUOp[2], ALUSrc, RegWrite, MemWrite;
};

struct EXMEM_Buffer {
    uint32_t ALU_Result, WriteDest; // rt_data
    bool ALU_Zero, MemWrite, MemRead, MemtoReg, RegWrite;
};

struct MEMWB_Buffer {
    uint32_t rt_data, WriteDest;
    bool MemtoReg, RegWrite;
};
