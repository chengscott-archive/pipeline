#include "main.hpp"

int main() {
    snapshot = fopen("snapshot.rpt", "w");
    error_dump = fopen("error_dump.rpt", "w");
    mem.LoadInstr();
    uint32_t SP = mem.LoadData();
    reg.setReg(29, SP);
    dump_reg(0);
    for (size_t cycle = 1; cycle <= 500000; ++cycle) {
        WB();
        MEM();
        EX();
        ID();
        IF();
        dump_reg(cycle);
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

void dump_reg(const size_t cycle) {
    fprintf(snapshot, "cycle %zu\n", cycle);
    for (int i = 0; i < 32; ++i) {
        if (cycle == 0 || reg.getReg(i) != regt.getReg(i))
            fprintf(snapshot, "$%02d: 0x%08X\n", i, reg.getReg(i));
    }
    if (cycle == 0 || reg.getHI() != regt.getHI())
        fprintf(snapshot, "$HI: 0x%08X\n", reg.getHI());
    if (cycle == 0 || reg.getLO() != regt.getLO())
        fprintf(snapshot, "$LO: 0x%08X\n", reg.getLO());
    fprintf(snapshot, "PC: 0x%08X\n\n\n", mem.getPC());
    // IF, ID, EX, DM WB
    regt = reg;
}

void WB() {

}

void MEM() {

}

void EX() {
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
}

void ID() {
    const uint32_t instr = IF_ID.instr;
    ID_EX.type = getType(instr);
    switch (ID_EX.type) {
        case 'R': {
            ID_EX.opcode = (instr >> 26) & 0x3f;
            ID_EX.rs = (instr >> 21) & 0x1f;
            ID_EX.rt = (instr >> 16) & 0x1f;
            ID_EX.rd = (instr >> 11) & 0x1f;
            ID_EX.shamt = (instr >> 6) & 0x1f;
            ID_EX.funct = instr & 0x3f;
            break;
        }
        case 'I': {
            ID_EX.opcode = (instr >> 26) & 0x3f;
            ID_EX.rs = (instr >> 21) & 0x1f;
            ID_EX.rt = (instr >> 16) & 0x1f;
            ID_EX.C = instr & 0xffff;
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
}

void IF () {
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

void R_execute() {
    const uint32_t funct = ID_EX.funct,
            rs_data = reg.getReg(ID_EX.rs),
            rt_data = reg.getReg(ID_EX.rt);
    uint32_t res = 0, err = 0;
    if (funct == 0x08) {
        // TODO: jr
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
    } else if (funct == 0x00) {
        // TODO: sll, NOP
        if (ID_EX.rd == 0 && !(ID_EX.rt == 0 && ID_EX.shamt == 0))
            err |= ERR_WRITE_REG_ZERO;
        else reg.setReg(ID_EX.rd, rt_data << ID_EX.shamt);
    } else {
        if (funct == 0x20) {
            // add (signed)
            res = int32_t(rs_data) + int32_t(rt_data);
            err |= isOverflow(rs_data, rt_data, res);
        } else if (funct == 0x21) {
            // addu
            res = rs_data + rt_data;
        } else if (funct == 0x22) {
            // sub (signed)
            res = int32_t(rs_data) - int32_t(rt_data);
            err |= isOverflow(rs_data, - int32_t(rt_data), res);
        } else if (funct == 0x24) {
            // and
            res = rs_data & rt_data;
        } else if (funct == 0x25) {
            // or
            res = rs_data | rt_data;
        } else if (funct == 0x26) {
            // xor
            res = rs_data ^ rt_data;
        } else if (funct == 0x27) {
            // nor
            res = ~(rs_data | rt_data);
        } else if (funct == 0x28) {
            // nand
            res = ~(rs_data & rt_data);
        } else if (funct == 0x2A) {
            // slt (signed)
            res = int32_t(rs_data) < int32_t(rt_data) ? 1 : 0;
        } else if (funct == 0x02) {
            // srl
            res = rt_data >> ID_EX.shamt;
        } else if (funct == 0x03) {
            // sra
            res = int32_t(rt_data) >> ID_EX.shamt;
        } else if (funct == 0x10) {
            // TODO: mfhi
            res = reg.fetchHI();
        } else if (funct == 0x12) {
            // TODO: mflo
            res = reg.fetchLO();
        }
        // EX_MEM
        EX_MEM.ALU_Zero = EX_MEM.MemWrite = EX_MEM.MemRead = false;
        if (ID_EX.rd == 0) {
            err |= ERR_WRITE_REG_ZERO;
            EX_MEM.MemtoReg = EX_MEM.RegWrite = false;
        } else {
            EX_MEM.ALU_Result = res;
            EX_MEM.MemtoReg = EX_MEM.RegWrite = true;
        }
    }
    if (err != 0) throw err;
}

void I_execute() {
    const uint32_t opcode = ID_EX.opcode,
            rs_data = reg.getReg(ID_EX.rs),
            rt_data = reg.getReg(ID_EX.rt),
            C = ID_EX.C;
    uint32_t res = 0, err = 0;
    if (opcode == 0x04 || opcode == 0x05 || opcode == 0x07) {
        // beq, bne, bgtz (signed)
        // {14'{C[15]}, C, 2'b0}
        const int32_t Caddr = C >> 15 == 0x0 ? (0x0003ffff & (C << 2)) :
                (0xfffc0000 | (C << 2));
        res = int32_t(mem.getPC()) + Caddr;
        err |= isOverflow(mem.getPC(), Caddr, res);
        if ((opcode == 0x04 && rs_data == rt_data) ||
                (opcode == 0x05 && rs_data != rt_data) ||
                (opcode == 0x07 && int32_t(rs_data) > 0))
            mem.setPC(res);
    } else if (opcode == 0x2B) {
        // sw
        const int32_t Cext = SignExt16(C);
        res = int32_t(rs_data) + Cext;
        err |= (isOverflow(rs_data, Cext, res) |
                isOverflow(rs_data, Cext + 1, res) |
                isOverflow(rs_data, Cext + 2, res) |
                isOverflow(rs_data, Cext + 3, res));
        err |= (res >= 1024 || res + 1 >= 1024 ||
                res + 2 >= 1024 || res + 3 >= 1024 ? ERR_ADDRESS_OVERFLOW : 0);
        err |= (res % 4 != 0 ? ERR_MISALIGNMENT : 0);
        if (err & HALT) throw err;
        mem.saveWord(res, rt_data);
    } else if (opcode == 0x29) {
        // sh
        const int32_t Cext = SignExt16(C);
        res = int32_t(rs_data) + Cext;
        err |= (isOverflow(rs_data, Cext, res) | isOverflow(rs_data, Cext + 1, res));
        err |= (res >= 1024 || res + 1 >= 1024 ? ERR_ADDRESS_OVERFLOW : 0);
        err |= (res % 2 != 0 ? ERR_MISALIGNMENT : 0);
        if (err & HALT) throw err;
        mem.saveHalfWord(res, rt_data);
    } else if (opcode == 0x28) {
        // sb
        const int32_t Cext = SignExt16(C);
        res = int32_t(rs_data) + Cext;
        err |= isOverflow(rs_data, Cext, res);
        err |= (res >= 1024 ? ERR_ADDRESS_OVERFLOW : 0);
        if (err & HALT) throw err;
        mem.saveByte(res, rt_data);
    } else {
        if (ID_EX.rt == 0) err |= ERR_WRITE_REG_ZERO;
        if (opcode == 0x08) {
            // addi (signed)
            const int32_t Cext = SignExt16(C);
            res = int32_t(rs_data) + Cext;
            err |= isOverflow(rs_data, Cext, res);
            reg.setReg(ID_EX.rt, res);
        } else if (opcode == 0x09) {
            // addiu
            reg.setReg(ID_EX.rt, rs_data + SignExt16(C));
        } else if (opcode == 0x0F) {
            // lui
            reg.setReg(ID_EX.rt, C << 16);
        } else if (opcode == 0x0C) {
            // andi
            reg.setReg(ID_EX.rt, rs_data & ZeroExt16(C));
        } else if (opcode == 0x0D) {
            // ori
            reg.setReg(ID_EX.rt, rs_data | ZeroExt16(C));
        } else if (opcode == 0x0E) {
            // nori
            reg.setReg(ID_EX.rt, ~(rs_data | ZeroExt16(C)));
        } else if (opcode == 0x0A) {
            // slti (signed)
            reg.setReg(ID_EX.rt, int32_t(rs_data) < int32_t(SignExt16(C)) ? 1 : 0);
        }  else {
            const int32_t Cext = SignExt16(C);
            res = int32_t(rs_data) + Cext;
            if (opcode == 0x23) {
                // lw
                err |= (isOverflow(rs_data, Cext, res) |
                        isOverflow(rs_data, Cext + 1, res) |
                        isOverflow(rs_data, Cext + 2, res) |
                        isOverflow(rs_data, Cext + 3, res));
                err |= (res >= 1024 || res + 1 >= 1024 ||
                        res + 2 >= 1024 || res + 3 >= 1024 ?
                        ERR_ADDRESS_OVERFLOW : 0);
                err |= (res % 4 != 0 ? ERR_MISALIGNMENT : 0);
                if (err & HALT) throw err;
                reg.setReg(ID_EX.rt, mem.loadWord(res));
            } else if (opcode == 0x21) {
                // lh
                err |= (isOverflow(rs_data, Cext, res) | isOverflow(rs_data, Cext + 1, res));
                err |= (res >= 1024 || res + 1 >= 1024 ? ERR_ADDRESS_OVERFLOW : 0);
                err |= (res % 2 != 0 ? ERR_MISALIGNMENT : 0);
                if (err & HALT) throw err;
                reg.setReg(ID_EX.rt, SignExt16(mem.loadHalfWord(res)));
            } else if (opcode == 0x25) {
                // lhu
                err |= (isOverflow(rs_data, Cext, res) | isOverflow(rs_data, Cext + 1, res));
                err |= (res >= 1024 || res + 1 >= 1024 ? ERR_ADDRESS_OVERFLOW : 0);
                err |= (res % 2 != 0 ? ERR_MISALIGNMENT : 0);
                if (err & HALT) throw err;
                reg.setReg(ID_EX.rt, mem.loadHalfWord(res) & 0xffff);
            } else if (opcode == 0x20) {
                // lb
                err |= isOverflow(rs_data, Cext, res);
                err |= (res >= 1024 ? ERR_ADDRESS_OVERFLOW : 0);
                if (err & HALT) throw err;
                reg.setReg(ID_EX.rt, SignExt8(mem.loadByte(res)));
            } else if (opcode == 0x24) {
                // lbu
                err |= isOverflow(rs_data, Cext, res);
                err |= (res >= 1024 ? ERR_ADDRESS_OVERFLOW : 0);
                if (err & HALT) throw err;
                reg.setReg(ID_EX.rt, mem.loadByte(res) & 0xff);
            }
        }
    }
    if (err != 0) throw err;
}

void J_execute() {
    if (ID_EX.opcode == 0x03) {
        // jal
        reg.setReg(31, mem.getPC());
    }
    // j && jal: PC = {(PC+4)[31:28], C, 2'b0}
    mem.setPC((mem.getPC() & 0xf0000000) | (ID_EX.C << 2));
}
