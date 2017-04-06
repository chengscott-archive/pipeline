#pragma once

class buffer {
public:
    buffer() {}
    IR::IR* instr_;
    uint32_t RawInstr_ = 0, ALUOut_ = 0, RegRs_ = 0, RegRt_ = 0, WriteDst_ = 0;
    bool RegWrite_ = false, MemRead_ = false;
};
