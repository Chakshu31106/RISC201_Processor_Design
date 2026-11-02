#include "Simulator/micro_executor.h"

#include "Control/internal_registers.h"
#include "Control/microcode_setup.h"

namespace risc201 {

using control::InternalRegister;

MicroExecutor::MicroExecutor() { cu_.loadProgram(control::buildDefaultMicrocode()); }

bool MicroExecutor::can_run(const DecodedInstr& d) const {
    if (cu_.getProgram().getEntryAddress(d.opcode) < 0) return false;
    return true;
}

namespace {

// Data-memory words an instruction reads before it runs and writes while it runs.
struct MemUse { int64_t read_addr = 0, write_addr = 0; int read_words = 0, write_words = 0; };

MemUse memory_use(const DecodedInstr& d, const RegisterFile& regs) {
    MemUse u;
    const std::string& m = d.mnemonic;
    const int64_t sp = static_cast<uint32_t>(regs.sp());
    auto eff = [&](uint8_t base) { return static_cast<int64_t>(static_cast<uint32_t>(regs.read(base))) + d.imm; };
    if (m == "ld")        { u.read_addr = eff(d.rs1);  u.read_words = 1; }
    else if (m == "vld")  { u.read_addr = eff(d.rs1);  u.read_words = 3; }
    else if (m == "st")   { u.write_addr = eff(d.rd);  u.write_words = 1; }   // for st/vst, rd is the base register
    else if (m == "vst")  { u.write_addr = eff(d.rd);  u.write_words = 3; }
    else if (m == "push") { u.write_addr = sp - 4;     u.write_words = 1; }
    else if (m == "call") { u.write_addr = sp - 8;     u.write_words = 2; }
    else if (m == "pop" || m == "peek") { u.read_addr = sp; u.read_words = 1; }
    else if (m == "ret")  { u.read_addr = sp;          u.read_words = 2; }
    return u;
}

bool in_memory(int64_t addr, int words, const memory::Memory& dmem) {
    return words > 0 && addr >= 0 && addr % 4 == 0 && addr + 4 * words <= static_cast<int64_t>(dmem.size());
}

}  // namespace

uint32_t MicroExecutor::execute(const DecodedInstr& d, uint32_t pc, RegisterFile& regs, memory::Memory& dmem) {
    // Copy the architectural state into the control unit.
    int32_t before[RegisterFile::NUM_REGS];
    for (int i = 0; i < RegisterFile::NUM_REGS; ++i) {
        before[i] = regs.read(static_cast<uint8_t>(i));
        cu_.writeRegister(static_cast<std::size_t>(i), before[i]);
    }
    cu_.writeInternal(InternalRegister::FLAG_E, regs.flag_e());
    cu_.writeInternal(InternalRegister::FLAG_GT, regs.flag_gt());
    cu_.writeInternal(InternalRegister::FLAG_ST, regs.flag_st());

    // Copy the memory words the instruction reads (skipped if the address is bad:
    // the control unit then reports the error itself).
    const MemUse use = memory_use(d, regs);
    if (in_memory(use.read_addr, use.read_words, dmem))
        for (int w = 0; w < use.read_words; ++w) {
            const std::size_t a = static_cast<std::size_t>(use.read_addr + 4 * w);
            cu_.writeMemoryWord(a, dmem.readWord(a));
        }

    cu_.loadInstruction(d.raw, pc);
    cu_.executeInstruction();

    // Copy the results back.
    for (int i = 0; i < RegisterFile::NUM_REGS; ++i) {
        int32_t now = cu_.readRegister(static_cast<std::size_t>(i));
        if (now != before[i]) regs.write(static_cast<uint8_t>(i), now);
    }
    regs.set_flags(cu_.readInternal(InternalRegister::FLAG_E) != 0,
                   cu_.readInternal(InternalRegister::FLAG_GT) != 0,
                   cu_.readInternal(InternalRegister::FLAG_ST) != 0);
    if (in_memory(use.write_addr, use.write_words, dmem))
        for (int w = 0; w < use.write_words; ++w) {
            const std::size_t a = static_cast<std::size_t>(use.write_addr + 4 * w);
            dmem.writeWord(a, cu_.readMemoryWord(a));
        }

    return static_cast<uint32_t>(cu_.readInternal(InternalRegister::PC));
}

}  // namespace risc201
