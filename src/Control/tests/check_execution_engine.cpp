
#include <cassert>
#include <cstdint>
#include <vector>

#include "control_unit.h"
#include "isa_opcodes.h"

int main() {
    using namespace control;
 
    MicrocodeProgram program;
    std::vector<Microinstruction> routine;

    // Address 0: TMP1 = 5
    Microinstruction m0{};
    m0.type = MicroType::MMOVI;
    m0.dest = static_cast<int>(
        registerIndex(InternalRegister::TMP1)
    );
    m0.uimm = 5;
    routine.push_back(m0);

    // Address 1: TMP1 = TMP1 + 3
    Microinstruction m1{};
    m1.type = MicroType::MADD;
    m1.dest = static_cast<int>(
        registerIndex(InternalRegister::TMP1)
    );
    m1.uimm = 3;
    routine.push_back(m1);

    // Address 2: TMP2 = TMP1
    Microinstruction m2{};
    m2.type = MicroType::MMOV;
    m2.src = static_cast<int>(
        registerIndex(InternalRegister::TMP1)
    );
    m2.dest = static_cast<int>(
        registerIndex(InternalRegister::TMP2)
    );
    routine.push_back(m2);

    // Address 3: if TMP2 == 8, jump to address 5.
    Microinstruction m3{};
    m3.type = MicroType::MBEQ;
    m3.src = static_cast<int>(
        registerIndex(InternalRegister::TMP2)
    );
    m3.uimm = 8;
    m3.seqAddr = 5;
    routine.push_back(m3);

    // Address 4: skipped by MBEQ.
    Microinstruction m4{};
    m4.type = MicroType::MMOVI;
    m4.dest = static_cast<int>(
        registerIndex(InternalRegister::TMP3)
    );
    m4.uimm = 99;
    routine.push_back(m4);

    // Address 5: unconditional jump to address 7.
    Microinstruction m5{};
    m5.type = MicroType::MB;
    m5.seqAddr = 7;
    routine.push_back(m5);

    // Address 6: skipped by MB.
    Microinstruction m6{};
    m6.type = MicroType::MMOVI;
    m6.dest = static_cast<int>(
        registerIndex(InternalRegister::TMP3)
    );
    m6.uimm = 100;
    routine.push_back(m6);

    // Address 7: TMP4 = 42
    Microinstruction m7{};
    m7.type = MicroType::MMOVI;
    m7.dest = static_cast<int>(
        registerIndex(InternalRegister::TMP4)
    );
    m7.uimm = 42;
    routine.push_back(m7);

    program.addRoutine(isa::ADD, routine);

    ControlUnit cu;
    cu.loadProgram(program);

    // ADD r0, r0, r0 — its fields are irrelevant to this test.
    const std::uint64_t instruction =
        static_cast<std::uint64_t>(isa::ADD) << 57;

    cu.loadInstruction(instruction, 0);

    // Execute the six instructions reached by the microbranches.
    for (int i = 0; i < 6; ++i) {
        assert(cu.stepMicroinstruction());
    }

    assert(cu.readInternal(InternalRegister::TMP1) == 8);
    assert(cu.readInternal(InternalRegister::TMP2) == 8);

    // Both skipped instructions must leave TMP3 unchanged.
    assert(cu.readInternal(InternalRegister::TMP3) == 0);

    assert(cu.readInternal(InternalRegister::TMP4) == 42);

    // The routine has reached the end of Control Memory.
    assert(!cu.stepMicroinstruction());

    return 0;
}
