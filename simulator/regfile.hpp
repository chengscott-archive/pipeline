#pragma once
class regfile {
public:
    regfile() {}
    regfile(const regfile& rhs) {
        for (size_t i = 0; i < 32; ++i) {
            reg_[i] = rhs.reg_[i];
        }
        HI_ = rhs.HI_;
        LO_ = rhs.LO_;
        HILOlock_ = rhs.HILOlock_;
    }
    const uint32_t& getReg(const size_t& idx) const { return reg_[idx]; }
    void setReg(const size_t& idx, const uint32_t& rhs) {
        if (idx != 0) reg_[idx] = rhs;
    }
    const uint32_t& getHI() const { return HI_; }
    const uint32_t& fetchHI() {
        HILOlock_ = false;
        return HI_;
    }
    const uint32_t& getLO() const { return LO_; }
    const uint32_t& fetchLO() {
        HILOlock_ = false;
        return LO_;
    }
    bool setHILO(const uint32_t& lhs, const uint32_t& rhs) {
        bool ret = HILOlock_;
        HI_ = lhs;
        LO_ = rhs;
        HILOlock_ = true;
        return ret;
    }

private:
    uint32_t reg_[32] = {}, HI_ = 0, LO_ = 0;
    bool HILOlock_ = false;
};
