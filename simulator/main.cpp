#include "main.hpp"

int main() {
    snapshot = fopen("snapshot.rpt", "w");
    error_dump = fopen("error_dump.rpt", "w");
    mem.LoadInstr();
    uint32_t SP = mem.LoadData();
    reg.setReg(29, SP);
    string stages[5];
    for (size_t cycle = 0; cycle <= 500000; ++cycle) {
        fprintf(snapshot, "cycle %zu\n", cycle);
        stages[4] = WB();
        stages[3] = MEM();
        stages[2] = EX();
        stages[1] = ID();
        if (cycle == 0) {
            for (int i = 0; i < 32; ++i) {
                fprintf(snapshot, "$%02d: 0x%08X\n", i, 0);
            }
            fprintf(snapshot, "$HI: 0x%08X\n$LO: 0x%08X\n", 0, 0);
        }
        fprintf(snapshot, "PC: 0x%08X\n", mem.getPC());
        IF();
        fprintf(snapshot, "IF: 0x%08X\nID: %s\nEX: %s\nDM: %s\nWB: %s\n\n\n",
            IF_ID.instr, stages[1].c_str(), stages[2].c_str(),
            stages[3].c_str(), stages[4].c_str());
        if (getOpName(IF_ID.instr) == "HALT" && stages[1] == "HALT" &&
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
}

string WB() {
    //ID_EX.RegWrite = MEM_WB.RegWrite;
    if (MEM_WB.MemtoReg) {
        reg.setReg(MEM_WB.WriteDest, MEM_WB.rt_data);
    }
    return getOpName(MEM_WB.instr);
}

string MEM() {
    MEM_WB.instr = EX_MEM.instr;
    MEM_WB.MemtoReg = EX_MEM.MemtoReg;
    MEM_WB.RegWrite = EX_MEM.RegWrite;
    MEM_WB.WriteDest = EX_MEM.WriteDest;
    const uint32_t& opcode = EX_MEM.opcode,
            MemWrite = EX_MEM.MemWrite,
            MemRead = EX_MEM.MemRead,
            ALU_Result = EX_MEM.ALU_Result;
    // TODO: error
    if (opcode == 0x2B && MemWrite) { // sw
        mem.saveWord(EX_MEM.WriteDest, ALU_Result);
    } else if (opcode == 0x29 && MemWrite) { // sh
        mem.saveHalfWord(EX_MEM.WriteDest, ALU_Result);
    } else if (opcode == 0x28 && MemWrite) { // sb
        mem.saveByte(EX_MEM.WriteDest, ALU_Result);
    } else if (opcode == 0x23 && MemRead) { // lw
        MEM_WB.rt_data = mem.loadWord(ALU_Result);
    } else if (opcode == 0x21 && MemRead) { // lh
        MEM_WB.rt_data = SignExt16(mem.loadHalfWord(ALU_Result));
    } else if (opcode == 0x25 && MemRead) { // lhu
        MEM_WB.rt_data = mem.loadHalfWord(ALU_Result) & 0xffff;
    } else if (opcode == 0x20 && MemRead) { // lb
        MEM_WB.rt_data = SignExt8(mem.loadByte(ALU_Result));
    } else if (opcode == 0x24 && MemRead) { // lbu
        MEM_WB.rt_data = mem.loadByte(ALU_Result) & 0xff;
    }
    return getOpName(EX_MEM.instr);
}

string EX() {
    EX_MEM.instr = ID_EX.instr;
    EX_MEM.opcode = ID_EX.opcode;
    try {
        switch (ID_EX.type) {
            case 'R': { R_execute(); break; }
            case 'I': { I_execute(); break; }
            case 'J': { J_execute(); break; }
            case 'S': { break; }
            default: {
                printf("illegal instruction found at 0x%08X\n", mem.getPC());
                //break;
            }
        }
    } catch (uint32_t ex) {
        //dump_error(ex, cycle);
        //if (ex & HALT) break;
    }
    return getOpName(ID_EX.instr);
}

string ID() {
    const uint32_t instr = IF_ID.instr;
    ID_EX.instr = instr;
    ID_EX.type = getType(instr);
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
            // Invalid Instr
        }
    }
    return getOpName(instr);
}

void IF() {
    // TODO: stall
    IF_ID.instr = mem.getInstr();
}

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

string getFunctName(const uint32_t funct) {
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

string getOpName(const uint32_t instr) {
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

void R_execute() {
    const uint32_t& funct = ID_EX.funct,
            rs_data = ID_EX.rs_data,
            rt_data = ID_EX.rt_data;
    uint32_t res = 0, err = 0;
    // EX_MEM
    EX_MEM.ALU_Zero = EX_MEM.MemWrite = EX_MEM.MemRead = EX_MEM.MemtoReg = false;
    EX_MEM.RegWrite = true;
    EX_MEM.WriteDest = ID_EX.rd;
    if (funct == 0x08) {
        // jr
        mem.setPC(rs_data);
    } else if (funct == 0x18) {
        // TODO: mult (signed)
        int64_t m = SignExt32(rs_data) * SignExt32(rt_data);
        bool isOverwrite = reg.setHILO(m >> 32, m & 0x00000000ffffffff);
        err |= (isOverwrite ? ERR_OVERWRTIE_REG_HI_LO : 0);
    } else if (funct == 0x19) {
        // TODO: multu
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
    if (err != 0) throw err;
}

void I_execute() {
    const uint32_t& opcode = ID_EX.opcode,
            rs_data = ID_EX.rs_data,
            rt_data = ID_EX.rt_data,
            C = ID_EX.C;
    uint32_t res = 0, err = 0;
    // EX_MEM
    EX_MEM.ALU_Zero = EX_MEM.MemWrite = EX_MEM.MemRead = EX_MEM.MemtoReg = false;
    EX_MEM.RegWrite = true;
    EX_MEM.WriteDest = ID_EX.rt;
    switch (opcode) {
        // beq, bne, bgtz (signed)
        case 0x04: case 0x05: case 0x07: {
            // {14'{C[15]}, C, 2'b0}
            const int32_t Caddr = C >> 15 == 0x0 ? (0x0003ffff & (C << 2)) :
                (0xfffc0000 | (C << 2));
            res = int32_t(mem.getPC()) + Caddr;
            err |= isOverflow(mem.getPC(), Caddr, res);
            if ((opcode == 0x04 && rs_data == rt_data) ||
                (opcode == 0x05 && rs_data != rt_data) ||
                (opcode == 0x07 && int32_t(rs_data) > 0))
            mem.setPC(res);
            EX_MEM.RegWrite = false;
            break;
        }
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
            res = ID_EX.rt_data;
            EX_MEM.MemWrite = true;
            EX_MEM.RegWrite = false;
            break;
        }
        // lw, lh, lhu, lb, lbu
        case 0x23: case 0x21: case 0x25: case 0x20: case 0x24: {
            const int32_t Cext = SignExt16(C);
            res = int32_t(rs_data) + Cext;
            err |= isOverflow(rs_data, Cext, res);
            EX_MEM.MemRead = EX_MEM.MemtoReg = true;
            break;
        }
    }
    EX_MEM.ALU_Result = res;
    if (err != 0) throw err;
}

void J_execute() {
    EX_MEM.ALU_Zero = EX_MEM.MemWrite =
        EX_MEM.MemRead = EX_MEM.MemtoReg = EX_MEM.RegWrite = false;
    if (ID_EX.opcode == 0x03) {
        // jal
        EX_MEM.ALU_Result = mem.getPC();
        EX_MEM.WriteDest = 31;
        EX_MEM.RegWrite = true;
    }
    // j && jal: PC = {(PC+4)[31:28], C, 2'b0}
    mem.setPC((mem.getPC() & 0xf0000000) | (ID_EX.C << 2));
}
