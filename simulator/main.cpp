#include "main.hpp"

int main() {
    snapshot = fopen("snapshot.rpt", "w");
    error_dump = fopen("error_dump.rpt", "w");
    mem.LoadInstr();
    uint32_t SP = mem.LoadData();
    reg.setReg(29, SP);
    for (size_t cycle = 0; cycle <= 500000; ++cycle) {
        fprintf(snapshot, "cycle %zu\n", cycle);
        if (cycle == 0) {
            for (int i = 0; i < 32; ++i) {
                fprintf(snapshot, "$%02d: 0x%08X\n", i, 0);
            }
            fprintf(snapshot, "$HI: 0x%08X\n$LO: 0x%08X\n", 0, 0);
        }
        if (MEM_WB_t.RegPrint) {
            fprintf(snapshot, "$%02d: 0x%08X\n", MEM_WB_t.WriteDest, MEM_WB_t.rt_data);
        }
        fprintf(snapshot, "PC: 0x%08X\n", mem.getPC());
        try {
            WB();
            MEM();
            EX();
            ID();
            fprintf(snapshot, "IF: 0x%08X", mem.getInstr());
            IF();
        } catch (uint32_t err) {
            dump_error(err, cycle);
            // if (err & HALT) break;
        }
        fprintf(snapshot, "%s\nID: %s\nEX: %s\nDM: %s\nWB: %s\n\n\n",
            stages[0].c_str(), stages[1].c_str(), stages[2].c_str(),
            stages[3].c_str(), stages[4].c_str());
        if (IR::getOpName(IF_ID.instr) == "HALT" && stages[1] == "HALT" &&
            stages[2] == "HALT" && stages[3] == "HALT" && stages[4] == "HALT") break;
    }
    fclose(snapshot);
    fclose(error_dump);
    return 0;
}

void dump_error(const uint32_t ex, const size_t cycle) {
    if (ex & ERR_WRITE_REG_ZERO) {
        fprintf(error_dump, "In cycle %zu: Write $0 Error\n", cycle);
    }
    if (ex & ERR_NUMBER_OVERFLOW) {
        fprintf(error_dump, "In cycle %zu: Number Overflow\n", cycle);
    }
    if (ex & ERR_OVERWRTIE_REG_HI_LO) {
        fprintf(error_dump, "In cycle %zu: Overwrite HI-LO registers\n", cycle);
    }
    if (ex & ERR_ADDRESS_OVERFLOW) {
        fprintf(error_dump, "In cycle %zu: Address Overflow\n", cycle);
    }
    if (ex & ERR_MISALIGNMENT) {
        fprintf(error_dump, "In cycle %zu: Misalignment Error\n", cycle);
    }
    if (ex & ERR_ILLEGAL) {
        printf("illegal instruction found at 0x%08X\n", mem.getPC());
    }
}

/**
* Five Stages
*/

void WB() {
    stages[4] = IR::getOpName(MEM_WB.instr);
    MEM_WB_t = MEM_WB;
    const uint32_t& dest = MEM_WB.WriteDest, data = MEM_WB.rt_data;
    if (MEM_WB.RegWrite) {
        MEM_WB_t.RegPrint = reg.getReg(dest) != data; // change
        if (dest == 0) throw ERR_WRITE_REG_ZERO;
        else reg.setReg(dest, data);
    }
}

void MEM() {
    stages[3] = IR::getOpName(EX_MEM.instr);
    MEM_WB.instr = EX_MEM.instr;
    MEM_WB.rt_data = EX_MEM.ALU_Result;
    MEM_WB.RegWrite = EX_MEM.RegWrite;
    MEM_WB.WriteDest = EX_MEM.WriteDest;
    const uint32_t& opcode = EX_MEM.opcode,
            MemWrite = EX_MEM.MemWrite,
            MemRead = EX_MEM.MemRead,
            ALU_Result = EX_MEM.ALU_Result;
    uint32_t err = 0;
    // TODO: add overflow: sw, sh, lw, lh, lhu
    if (opcode == 0x2B && MemWrite) { // sw
        err |= (ALU_Result >= 1024 || ALU_Result + 1 >= 1024 ||
            ALU_Result + 2 >= 1024 || ALU_Result + 3 >= 1024 ? ERR_ADDRESS_OVERFLOW : 0);
        err |= (ALU_Result % 4 != 0 ? ERR_MISALIGNMENT : 0);
        if (err & HALT) throw err;
        mem.saveWord(EX_MEM.WriteDest, ALU_Result);
    } else if (opcode == 0x29 && MemWrite) { // sh
        err |= (ALU_Result >= 1024 || ALU_Result + 1 >= 1024 ? ERR_ADDRESS_OVERFLOW : 0);
        err |= (ALU_Result % 2 != 0 ? ERR_MISALIGNMENT : 0);
        mem.saveHalfWord(EX_MEM.WriteDest, ALU_Result);
    } else if (opcode == 0x28 && MemWrite) { // sb
        err |= (ALU_Result >= 1024 ? ERR_ADDRESS_OVERFLOW : 0);
        mem.saveByte(EX_MEM.WriteDest, ALU_Result);
    } else if (opcode == 0x23 && MemRead) { // lw
        err |= (ALU_Result >= 1024 || ALU_Result + 1 >= 1024 ||
            ALU_Result + 2 >= 1024 || ALU_Result + 3 >= 1024 ? ERR_ADDRESS_OVERFLOW : 0);
        err |= (ALU_Result % 4 != 0 ? ERR_MISALIGNMENT : 0);
        MEM_WB.rt_data = mem.loadWord(ALU_Result);
    } else if (opcode == 0x21 && MemRead) { // lh
        err |= (ALU_Result >= 1024 || ALU_Result + 1 >= 1024 ? ERR_ADDRESS_OVERFLOW : 0);
        err |= (ALU_Result % 2 != 0 ? ERR_MISALIGNMENT : 0);
        MEM_WB.rt_data = SignExt16(mem.loadHalfWord(ALU_Result));
    } else if (opcode == 0x25 && MemRead) { // lhu
        err |= (ALU_Result >= 1024 || ALU_Result + 1 >= 1024 ? ERR_ADDRESS_OVERFLOW : 0);
        err |= (ALU_Result % 2 != 0 ? ERR_MISALIGNMENT : 0);
        MEM_WB.rt_data = mem.loadHalfWord(ALU_Result) & 0xffff;
    } else if (opcode == 0x20 && MemRead) { // lb
        err |= (ALU_Result >= 1024 ? ERR_ADDRESS_OVERFLOW : 0);
        MEM_WB.rt_data = SignExt8(mem.loadByte(ALU_Result));
    } else if (opcode == 0x24 && MemRead) { // lbu
        err |= (ALU_Result >= 1024 ? ERR_ADDRESS_OVERFLOW : 0);
        MEM_WB.rt_data = mem.loadByte(ALU_Result) & 0xff;
    }
    if (err != 0) throw err;
}

void EX() {
    stages[2] = IR::getOpName(ID_EX.instr);
    EX_MEM.instr = ID_EX.instr;
    EX_MEM.opcode = ID_EX.opcode;
    // fwd_EX-DM, fwd_DM-WB
    bool has_rs = IR::has_rs(EX_MEM.instr);
    if (EX_MEM.RegWrite && EX_MEM.WriteDest != 0 && has_rs && EX_MEM.WriteDest == ID_EX.rs) {
        ID_EX.rs = EX_MEM.WriteDest;
        ID_EX.rs_data = EX_MEM.ALU_Result;
        stages[2] += " fwd_EX-DM_rs_$" + std::to_string(EX_MEM.WriteDest);
    } else if (MEM_WB_t.RegWrite && MEM_WB_t.WriteDest != 0 && has_rs && MEM_WB_t.WriteDest == ID_EX.rs) {
        ID_EX.rs = MEM_WB_t.WriteDest;
        ID_EX.rs_data = MEM_WB_t.rt_data;
        stages[2] += " fwd_DM-WB_rs_$" + std::to_string(MEM_WB_t.WriteDest);
    }
    bool has_rt = IR::has_rt(EX_MEM.instr);
    if (EX_MEM.RegWrite && EX_MEM.WriteDest != 0 && has_rt && EX_MEM.WriteDest == ID_EX.rt) {
        ID_EX.rt = EX_MEM.WriteDest;
        ID_EX.rt_data = EX_MEM.ALU_Result;
        stages[2] += " fwd_EX-DM_rt_$" + std::to_string(EX_MEM.WriteDest);
    } else if (MEM_WB_t.RegWrite && MEM_WB_t.WriteDest != 0 && has_rt && MEM_WB_t.WriteDest == ID_EX.rt) {
        ID_EX.rt = MEM_WB_t.WriteDest;
        ID_EX.rt_data = MEM_WB_t.rt_data;
        stages[2] += " fwd_DM-WB_rt_$" + std::to_string(MEM_WB_t.WriteDest);
    }
    uint32_t err;
    switch (ID_EX.type) {
        case 'R': { err = R_execute(); break; }
        case 'I': { err = I_execute(); break; }
        case 'J': { err = J_execute(); break; }
        case 'S': default: { break; }
    }
    if (err != 0) throw err;
}

void ID() {
    const uint32_t& instr = IF_ID.instr;
    stages[1] = IR::getOpName(instr);
    // stall
    if (IR::isMemRead(ID_EX.instr) && IF_ID.rs != 0 && ID_EX.rt == IF_ID.rs) {
        stall = true;
    }
    if (IR::isMemRead(ID_EX.instr) && IF_ID.rt != 0 && ID_EX.rt == IF_ID.rt) {
        stall = true;
    }
    if (stall) {
        stages[1] += " to_be_stalled";
        ID_EX.type = 'R';
        ID_EX.instr = ID_EX.opcode = ID_EX.rs = ID_EX.rt = ID_EX.rd
            = ID_EX.shamt = ID_EX.funct = ID_EX.C
            = ID_EX.rs_data = ID_EX.rt_data = 0;
        return;
    }
    // ID
    ID_EX.instr = instr;
    ID_EX.type = IR::getType(instr);
    switch (ID_EX.type) {
        case 'R': {
            ID_EX.opcode = (instr >> 26) & 0x3f;
            ID_EX.rs = (instr >> 21) & 0x1f;
            ID_EX.rt = (instr >> 16) & 0x1f;
            ID_EX.rd = (instr >> 11) & 0x1f;
            ID_EX.shamt = (instr >> 6) & 0x1f;
            ID_EX.funct = instr & 0x3f;
            ID_EX.rs_data = reg.getReg(ID_EX.rs);
            ID_EX.rt_data = reg.getReg(ID_EX.rt);
            switch (ID_EX.funct) {
                case 0x00: case 0x02: case 0x03: { break; }
                case 0x08: { break; } // jr
            }
            break;
        }
        case 'I': {
            ID_EX.opcode = (instr >> 26) & 0x3f;
            ID_EX.rs = (instr >> 21) & 0x1f;
            ID_EX.rt = (instr >> 16) & 0x1f;
            ID_EX.C = instr & 0xffff;
            ID_EX.rs_data = reg.getReg(ID_EX.rs);
            ID_EX.rt_data = reg.getReg(ID_EX.rt);
            break;
        }
        case 'J': case 'S': {
            ID_EX.opcode = (instr >> 26) & 0x3f;
            ID_EX.C = instr & 0x3ffffff;
            break;
        }
        default: {
            throw ERR_ILLEGAL;
        }
    }
    // beq, bne, bgtz (signed)
    if (ID_EX.type == 'I' &&
        (ID_EX.opcode == 0x04 || ID_EX.opcode == 0x05 || ID_EX.opcode == 0x07)) {
        // {14'{C[15]}, C, 2'b0}
        EX_MEM.MemWrite = EX_MEM.MemRead = EX_MEM.RegWrite = false;
        EX_MEM.WriteDest = ID_EX.rt;
        const int32_t Caddr = ID_EX.C >> 15 == 0x0 ? (0x0003ffff & (ID_EX.C << 2)) :
            (0xfffc0000 | (ID_EX.C << 2));
        if ((ID_EX.opcode == 0x04 && ID_EX.rs_data == ID_EX.rt_data) ||
            (ID_EX.opcode == 0x05 && ID_EX.rs_data != ID_EX.rt_data) ||
            (ID_EX.opcode == 0x07 && int32_t(ID_EX.rs_data) > 0))
            mem.setPC(int32_t(mem.getPC()) + Caddr);
    }
}

void IF() {
    stages[0] = "";
    // stall
    if (stall) {
        stages[0] = " to_be_stalled";
        stall = false;
        return;
    }
    IF_ID.instr = mem.getInstr();
    mem.setPC(mem.getPC() + 4);
    IF_ID.rs = (IF_ID.instr >> 21) & 0x1f;
    IF_ID.rt = (IF_ID.instr >> 16) & 0x1f;
}

/**
* EX Stage
* R type: R_execute()
* I type: I_execute()
* J type: J_execute()
*/

uint32_t R_execute() {
    const uint32_t& funct = ID_EX.funct,
            rs_data = ID_EX.rs_data,
            rt_data = ID_EX.rt_data;
    uint32_t res = 0, err = 0;
    // EX_MEM
    EX_MEM.MemWrite = EX_MEM.MemRead = false;
    EX_MEM.RegWrite = true;
    EX_MEM.WriteDest = ID_EX.rd;
    if (ID_EX.instr == 0) { // NOP
        EX_MEM.RegWrite = false;
        return 0;
    }
    if (funct == 0x08) {
        // jr
        mem.setPC(rs_data);
    } else if (funct == 0x18) {
        // mult (signed)
        int64_t m = SignExt32(rs_data) * SignExt32(rt_data);
        bool isOverwrite = reg.setHILO(m >> 32, m & 0x00000000ffffffff);
        err |= (isOverwrite ? ERR_OVERWRTIE_REG_HI_LO : 0);
    } else if (funct == 0x19) {
        // multu
        uint64_t m = uint64_t(rs_data) * uint64_t(rt_data);
        bool isOverwrite = reg.setHILO(m >> 32, m & 0x00000000ffffffff);
        err |= (isOverwrite ? ERR_OVERWRTIE_REG_HI_LO : 0);
    } else {
        switch (funct) {
            // add (signed)
            case 0x20: {
                res = int32_t(rs_data) + int32_t(rt_data);
                err |= isOverflow(rs_data, rt_data, res);
                break;
            }
            // addu
            case 0x21: { res = rs_data + rt_data; break; }
            // sub (signed)
            case 0x22: {
                res = int32_t(rs_data) - int32_t(rt_data);
                err |= isOverflow(rs_data, - int32_t(rt_data), res);
                break;
            }
            // and
            case 0x24: { res = rs_data & rt_data; break; }
            // or
            case 0x25: { res = rs_data | rt_data; break; }
            // xor
            case 0x26: { res = rs_data ^ rt_data; break; }
            // nor
            case 0x27: { res = ~(rs_data | rt_data); break; }
            // nand
            case 0x28: { res = ~(rs_data & rt_data); break; }
            // slt (signed)
            case 0x2A: {
                res = int32_t(rs_data) < int32_t(rt_data) ? 1 : 0;
                break;
            }
            // sll, NOP
            case 0x00: { res = rt_data << ID_EX.shamt; break; }
            // srl
            case 0x02: { res = rt_data >> ID_EX.shamt; break; }
            // sra
            case 0x03: { res = int32_t(rt_data) >> ID_EX.shamt; break; }
            // mfhi
            case 0x10: { res = reg.fetchHI(); break; }
            // mflo
            case 0x12: { res = reg.fetchLO(); break; }
        }
        EX_MEM.ALU_Result = res;
        if (ID_EX.rd == 0) err |= ERR_WRITE_REG_ZERO;
    }
    return err;
}

uint32_t I_execute() {
    const uint32_t& opcode = ID_EX.opcode,
            rs_data = ID_EX.rs_data,
            rt_data = ID_EX.rt_data,
            C = ID_EX.C;
    uint32_t res = 0, err = 0;
    // EX_MEM
    EX_MEM.MemWrite = EX_MEM.MemRead = false;
    EX_MEM.RegWrite = true;
    EX_MEM.WriteDest = ID_EX.rt;
    switch (opcode) {
        // addi (signed)
        case 0x08: {
            const int32_t Cext = SignExt16(C);
            res = int32_t(rs_data) + Cext;
            err |= isOverflow(rs_data, Cext, res);
            break;
        }
        // addiu
        case 0x09: { res = rs_data + SignExt16(C); break; }
        //lui
        case 0x0F: { res = C << 16; break; }
        // andi
        case 0x0C: { res = rs_data & ZeroExt16(C); break; }
        // ori
        case 0x0D: { res = rs_data | ZeroExt16(C); break; }
        // nori
        case 0x0E: { res = ~(rs_data | ZeroExt16(C)); break; }
        // slti (signed)
        case 0x0A: {
            res = int32_t(rs_data) < int32_t(SignExt16(C)) ? 1 : 0;
            break;
        }
        // sw, sh, sb
        case 0x2B: case 0x29: case 0x28: {
            const int32_t Cext = SignExt16(C);
            res = int32_t(rs_data) + Cext;
            err |= isOverflow(rs_data, Cext, res);
            EX_MEM.WriteDest = res;
            res = rt_data;
            EX_MEM.MemWrite = true;
            EX_MEM.RegWrite = false;
            break;
        }
        // lw, lh, lhu, lb, lbu
        case 0x23: case 0x21: case 0x25: case 0x20: case 0x24: {
            const int32_t Cext = SignExt16(C);
            res = int32_t(rs_data) + Cext;
            err |= isOverflow(rs_data, Cext, res);
            EX_MEM.MemRead = true;
            break;
        }
    }
    EX_MEM.ALU_Result = res;
    return err;
}

uint32_t J_execute() {
    EX_MEM.MemWrite = EX_MEM.MemRead = EX_MEM.RegWrite = false;
    if (ID_EX.opcode == 0x03) {
        // jal
        EX_MEM.ALU_Result = mem.getPC();
        EX_MEM.WriteDest = 31;
        EX_MEM.RegWrite = true;
    }
    // j && jal: PC = {(PC+4)[31:28], C, 2'b0}
    mem.setPC((mem.getPC() & 0xf0000000) | (ID_EX.C << 2));
    return 0;
}
