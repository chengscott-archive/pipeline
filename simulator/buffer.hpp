#pragma once

struct IFID_Buffer {
    uint32_t instr = 0, rs, rt;
};

struct IDEX_Buffer {
    char type = 'R';
    uint32_t instr = 0, opcode, rs, rt, rd, shamt, funct, C, rs_data, rt_data;
};

struct EXMEM_Buffer {
    uint32_t instr = 0, opcode = 0, ALU_Result, WriteDest;
    bool MemWrite = false, MemRead = false, RegWrite = false;
};

struct MEMWB_Buffer {
    MEMWB_Buffer() = default;
    MEMWB_Buffer(const MEMWB_Buffer& rhs) : instr(rhs.instr), rt_data(rhs.rt_data),
        WriteDest(rhs.WriteDest), RegWrite(rhs.RegWrite) {};
    uint32_t instr = 0, rt_data, WriteDest;
    bool RegWrite = false, RegPrint = false;
};
