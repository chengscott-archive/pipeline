#include <fstream>
#include "memory.hpp"
#include "regfile.hpp"
#include "irfile.hpp"
// ERR constant
#define ERR_WRITE_REG_ZERO 0x1 // continue
#define ERR_NUMBER_OVERFLOW 0x10  // continue
#define ERR_OVERWRTIE_REG_HI_LO 0x100 // continue
#define ERR_ADDRESS_OVERFLOW 0x1000 // halt
#define ERR_MISALIGNMENT 0x10000 // halt
#define HALT (ERR_ADDRESS_OVERFLOW | ERR_MISALIGNMENT) // halt code
// 32-bit C sign extend to 64-bit
#define SignExt32(C) (((C) >> 31 == 0x0) ?\
    ((C) & 0x00000000ffffffff) : ((C) | 0xffffffff00000000))
// 16-bit C sign extend to 32-bit
#define SignExt16(C) (((C) >> 15 == 0x0) ? ((C) & 0x0000ffff) : ((C) | 0xffff0000))
// 8-bit C sign extend to 32-bit
#define SignExt8(C) (((C) >> 7 == 0x0) ? ((C) & 0x000000ff) : ((C) | 0xffffff00))
// 16-bit C zero extend to 32-bit
#define ZeroExt16(C) ((C) & 0x0000ffff)
// check if a+b overflow
#define isOverflow(a, b, c)\
    (((int32_t(a) > 0 && int32_t(b) > 0 && int32_t(c) <= 0) ||\
    (int32_t(a) < 0 && int32_t(b) < 0 && int32_t(c) >= 0)) ?\
    ERR_NUMBER_OVERFLOW : 0)

memory mem;
regfile reg, regt;
FILE *snapshot, *error_dump;

void dump_reg(const size_t);
void R_execute(const uint32_t);
void I_execute(const uint32_t);
void J_execute(const uint32_t);
