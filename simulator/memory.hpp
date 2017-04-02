#pragma once
#include <fstream>
#define ToBig(x) (__builtin_bswap32(x))

class memory {
public:
    void LoadInstr();
    uint32_t LoadData();
    const uint32_t& getPC() const { return PC_; }
    void setPC(const uint32_t& rhs) { PC_ = rhs; }
    const uint32_t getInstr();
    const uint32_t loadWord(const size_t) const;
    const uint32_t loadHalfWord(const size_t) const;
    const uint32_t loadByte(const size_t) const;
    void saveWord(const size_t, const uint32_t);
    void saveHalfWord(const size_t, const uint32_t);
    void saveByte(const size_t, const uint32_t);

private:
    uint32_t PC_ = 0, PC0_ = 0;
    size_t icount_ = 0, dcount_ = 0;
    uint32_t instr_[1024] = {}, data_[4096] = {};
};
