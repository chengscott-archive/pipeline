#include "main.hpp"

int main() {
    snapshot = fopen("snapshot.rpt", "w");
    error_dump = fopen("error_dump.rpt", "w");
    mem.LoadInstr();
    uint32_t SP = mem.LoadData();
    reg.setReg(29, SP);
    dump_reg(0);
    for (size_t cycle = 1; cycle <= 500000; ++cycle) {
        uint32_t instr = mem.getInstr();
        const char type = IR::getType(instr);
        try {
            if (type == 'R') R_execute(instr);
            else if (type == 'I') I_execute(instr);
            else if (type == 'J') J_execute(instr);
            else if (type == 'S') break;
            else {
                printf("illegal instruction found at 0x%08X\n", mem.getPC());
                break;
            }
        } catch (uint32_t ex) {
            // dump error
            if (ex & ERR_WRITE_REG_ZERO) {
                fprintf(error_dump, "In cycle %zu: Write $0 Error\n", cycle);
            }
            if (ex & ERR_NUMBER_OVERFLOW) {
                fprintf(error_dump, "In cycle %zu: Number Overflow\n", cycle);
            }
            if (ex & ERR_OVERWRTIE_REG_HI_LO) {
                fprintf(error_dump,
                        "In cycle %zu: Overwrite HI-LO registers\n",
                        cycle);
            }
            if (ex & ERR_ADDRESS_OVERFLOW) {
                fprintf(error_dump, "In cycle %zu: Address Overflow\n", cycle);
            }
            if (ex & ERR_MISALIGNMENT) {
                fprintf(error_dump, "In cycle %zu: Misalignment Error\n", cycle);
            }
            if (ex & HALT) break;
        }
        dump_reg(cycle);
    }
    fclose(snapshot);
    fclose(error_dump);
    return 0;
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
    regt = reg;
}

void R_execute(const uint32_t rhs) {
    const IR::R_type instr = IR::R_decode(rhs);
    const uint32_t funct = instr.funct,
            rs = reg.getReg(instr.rs),
            rt = reg.getReg(instr.rt);
    uint32_t res = 0, err = 0;
    if (funct == 0x08) {
        // jr
        mem.setPC(rs);
    } else if (funct == 0x18) {
        // mult (signed)
        int64_t m = SignExt32(rs) * SignExt32(rt);
        bool isOverwrite = reg.setHILO(m >> 32, m & 0x00000000ffffffff);
        err |= (isOverwrite ? ERR_OVERWRTIE_REG_HI_LO : 0);
    } else if (funct == 0x19) {
        // multu
        uint64_t m = uint64_t(rs) * uint64_t(rt);
        bool isOverwrite = reg.setHILO(m >> 32, m & 0x00000000ffffffff);
        err |= (isOverwrite ? ERR_OVERWRTIE_REG_HI_LO : 0);
    } else if (funct == 0x00) {
        // sll, NOP
        if (instr.rd == 0 && !(instr.rt == 0 && instr.shamt == 0))
            err |= ERR_WRITE_REG_ZERO;
        else reg.setReg(instr.rd, rt << instr.shamt);
    } else {
        if (funct == 0x20) {
            // add (signed)
            res = int32_t(rs) + int32_t(rt);
            err |= isOverflow(rs, rt, res);
        } else if (funct == 0x21) {
            // addu
            res = rs + rt;
        } else if (funct == 0x22) {
            // sub (signed)
            res = int32_t(rs) - int32_t(rt);
            err |= isOverflow(rs, - int32_t(rt), res);
        } else if (funct == 0x24) {
            // and
            res = rs & rt;
        } else if (funct == 0x25) {
            // or
            res = rs | rt;
        } else if (funct == 0x26) {
            // xor
            res = rs ^ rt;
        } else if (funct == 0x27) {
            // nor
            res = ~(rs | rt);
        } else if (funct == 0x28) {
            // nand
            res = ~(rs & rt);
        } else if (funct == 0x2A) {
            // slt (signed)
            res = int32_t(rs) < int32_t(rt) ? 1 : 0;
        } else if (funct == 0x02) {
            // srl
            res = rt >> instr.shamt;
        } else if (funct == 0x03) {
            // sra
            res = int32_t(rt) >> instr.shamt;
        } else if (funct == 0x10) {
            // mfhi
            res = reg.fetchHI();
        } else if (funct == 0x12) {
            // mflo
            res = reg.fetchLO();
        }
        if (instr.rd == 0) err |= ERR_WRITE_REG_ZERO;
        else reg.setReg(instr.rd, res);
    }
    if (err != 0) throw err;
}

void I_execute(const uint32_t rhs) {
    const IR::I_type instr = IR::I_decode(rhs);
    const uint32_t opcode = instr.opcode,
            rs = reg.getReg(instr.rs),
            rt = reg.getReg(instr.rt),
            C = instr.C;
    uint32_t res = 0, err = 0;
    if (opcode == 0x04 || opcode == 0x05 || opcode == 0x07) {
        // beq, bne, bgtz (signed)
        // {14'{C[15]}, C, 2'b0}
        const int32_t Caddr = C >> 15 == 0x0 ? (0x0003ffff & (C << 2)) :
                (0xfffc0000 | (C << 2));
        res = int32_t(mem.getPC()) + Caddr;
        err |= isOverflow(mem.getPC(), Caddr, res);
        if ((opcode == 0x04 && rs == rt) ||
                (opcode == 0x05 && rs != rt) ||
                (opcode == 0x07 && int32_t(rs) > 0))
            mem.setPC(res);
    } else if (opcode == 0x2B) {
        // sw
        const int32_t Cext = SignExt16(C);
        res = int32_t(rs) + Cext;
        err |= (isOverflow(rs, Cext, res) |
                isOverflow(rs, Cext + 1, res) |
                isOverflow(rs, Cext + 2, res) |
                isOverflow(rs, Cext + 3, res));
        err |= (res >= 1024 || res + 1 >= 1024 ||
                res + 2 >= 1024 || res + 3 >= 1024 ? ERR_ADDRESS_OVERFLOW : 0);
        err |= (res % 4 != 0 ? ERR_MISALIGNMENT : 0);
        if (err & HALT) throw err;
        mem.saveWord(res, rt);
    } else if (opcode == 0x29) {
        // sh
        const int32_t Cext = SignExt16(C);
        res = int32_t(rs) + Cext;
        err |= (isOverflow(rs, Cext, res) | isOverflow(rs, Cext + 1, res));
        err |= (res >= 1024 || res + 1 >= 1024 ? ERR_ADDRESS_OVERFLOW : 0);
        err |= (res % 2 != 0 ? ERR_MISALIGNMENT : 0);
        if (err & HALT) throw err;
        mem.saveHalfWord(res, rt);
    } else if (opcode == 0x28) {
        // sb
        const int32_t Cext = SignExt16(C);
        res = int32_t(rs) + Cext;
        err |= isOverflow(rs, Cext, res);
        err |= (res >= 1024 ? ERR_ADDRESS_OVERFLOW : 0);
        if (err & HALT) throw err;
        mem.saveByte(res, rt);
    } else {
        if (instr.rt == 0) err |= ERR_WRITE_REG_ZERO;
        if (opcode == 0x08) {
            // addi (signed)
            const int32_t Cext = SignExt16(C);
            res = int32_t(rs) + Cext;
            err |= isOverflow(rs, Cext, res);
            reg.setReg(instr.rt, res);
        } else if (opcode == 0x09) {
            // addiu
            reg.setReg(instr.rt, rs + SignExt16(C));
        } else if (opcode == 0x0F) {
            // lui
            reg.setReg(instr.rt, C << 16);
        } else if (opcode == 0x0C) {
            // andi
            reg.setReg(instr.rt, rs & ZeroExt16(C));
        } else if (opcode == 0x0D) {
            // ori
            reg.setReg(instr.rt, rs | ZeroExt16(C));
        } else if (opcode == 0x0E) {
            // nori
            reg.setReg(instr.rt, ~(rs | ZeroExt16(C)));
        } else if (opcode == 0x0A) {
            // slti (signed)
            reg.setReg(instr.rt, int32_t(rs) < int32_t(SignExt16(C)) ? 1 : 0);
        }  else {
            const int32_t Cext = SignExt16(C);
            res = int32_t(rs) + Cext;
            if (opcode == 0x23) {
                // lw
                err |= (isOverflow(rs, Cext, res) |
                        isOverflow(rs, Cext + 1, res) |
                        isOverflow(rs, Cext + 2, res) |
                        isOverflow(rs, Cext + 3, res));
                err |= (res >= 1024 || res + 1 >= 1024 ||
                        res + 2 >= 1024 || res + 3 >= 1024 ?
                        ERR_ADDRESS_OVERFLOW : 0);
                err |= (res % 4 != 0 ? ERR_MISALIGNMENT : 0);
                if (err & HALT) throw err;
                reg.setReg(instr.rt, mem.loadWord(res));
            } else if (opcode == 0x21) {
                // lh
                err |= (isOverflow(rs, Cext, res) | isOverflow(rs, Cext + 1, res));
                err |= (res >= 1024 || res + 1 >= 1024 ? ERR_ADDRESS_OVERFLOW : 0);
                err |= (res % 2 != 0 ? ERR_MISALIGNMENT : 0);
                if (err & HALT) throw err;
                reg.setReg(instr.rt, SignExt16(mem.loadHalfWord(res)));
            } else if (opcode == 0x25) {
                // lhu
                err |= (isOverflow(rs, Cext, res) | isOverflow(rs, Cext + 1, res));
                err |= (res >= 1024 || res + 1 >= 1024 ? ERR_ADDRESS_OVERFLOW : 0);
                err |= (res % 2 != 0 ? ERR_MISALIGNMENT : 0);
                if (err & HALT) throw err;
                reg.setReg(instr.rt, mem.loadHalfWord(res) & 0xffff);
            } else if (opcode == 0x20) {
                // lb
                err |= isOverflow(rs, Cext, res);
                err |= (res >= 1024 ? ERR_ADDRESS_OVERFLOW : 0);
                if (err & HALT) throw err;
                reg.setReg(instr.rt, SignExt8(mem.loadByte(res)));
            } else if (opcode == 0x24) {
                // lbu
                err |= isOverflow(rs, Cext, res);
                err |= (res >= 1024 ? ERR_ADDRESS_OVERFLOW : 0);
                if (err & HALT) throw err;
                reg.setReg(instr.rt, mem.loadByte(res) & 0xff);
            }
        }
    }
    if (err != 0) throw err;
}

void J_execute(const uint32_t rhs) {
    const IR::J_type instr = IR::J_decode(rhs);
    if (instr.opcode == 0x03) {
        // jal
        reg.setReg(31, mem.getPC());
    }
    // j && jal: PC = {(PC+4)[31:28], C, 2'b0}
    mem.setPC((mem.getPC() & 0xf0000000) | (instr.C << 2));
}
