
#include "control_unit.h"
#include "instruction_decoder.h"
#include "isa_opcodes.h"
#include <stdexcept>

namespace control {

ControlUnit::ControlUnit()
    : program_(),
      registers_{},
      registerFile_(),
      memory_(65536),
      microPC_(-1),
      programLoaded_(false),
      instructionActive_(false) {
}

void ControlUnit::loadProgram(const MicrocodeProgram& program) {
    if (program.size() == 0) {
        throw std::invalid_argument(
            "Cannot load an empty microcode program"
        );
    }

    program_ = program;
    registers_.fill(0);

    microPC_ = -1;
    programLoaded_ = true;
    instructionActive_ = false;
}

void ControlUnit::loadInstruction(
    std::uint64_t instruction,
    std::uint64_t instructionAddress
) {
    if (!programLoaded_) {
        throw std::logic_error(
            "Load a microcode program before loading an instruction"
        );
    }

    const DecodedInstruction decoded =
        InstructionDecoder::decode(instruction);

    const int entryAddress =
        program_.getEntryAddress(decoded.opcode);

    if (entryAddress < 0) {
        throw std::invalid_argument(
            "No microcode routine registered for this ISA opcode"
        );
    }

    // Save the instruction and its architectural address.
    registers_[registerIndex(InternalRegister::IR)] =
        static_cast<std::int64_t>(instruction);

    registers_[registerIndex(InternalRegister::PC)] =
        static_cast<std::int64_t>(instructionAddress);

    // Save the decoded fields.
    registers_[registerIndex(InternalRegister::I)] =
        decoded.immediateMode ? 1 : 0;

    registers_[registerIndex(InternalRegister::RD)] = decoded.rd;
    registers_[registerIndex(InternalRegister::RS1)] = decoded.rs1;
    registers_[registerIndex(InternalRegister::RS2)] = decoded.rs2;

    registers_[registerIndex(InternalRegister::IMMX)] =
        decoded.immediate;

    registers_[registerIndex(InternalRegister::BRANCH_TARGET)] =
        decoded.isBranch ? decoded.branchOffset : 0;

    // Begin at the microcode routine for this opcode.
    microPC_ = 0;
    instructionActive_ = true;
    pcModified_ = false;
}

bool ControlUnit::stepMicroinstruction() {
    if (!programLoaded_ || !instructionActive_) {
        return false;
    }

    if (microPC_ < 0 ||
        static_cast<std::size_t>(microPC_) >= program_.size()) {
        instructionActive_ = false;
        throw std::out_of_range(
            "MicroPC is outside Control Memory"
        );
    }

    const Microinstruction& micro =
        program_.controlMemory[static_cast<std::size_t>(microPC_)];

    // Default: execute the current instruction, then advance.
    const int nextMicroPC = microPC_ + 1;

    switch (micro.type) {

        case MicroType::MMOV: {
            if (micro.src < 0 || micro.dest < 0 ||
                static_cast<std::size_t>(micro.src) >=
                    INTERNAL_REGISTER_COUNT ||
                static_cast<std::size_t>(micro.dest) >=
                    INTERNAL_REGISTER_COUNT) {
                throw std::out_of_range(
                    "MMOV uses an invalid internal register"
                );
            }

            const auto srcIndex = static_cast<std::size_t>(micro.src);
            const auto destIndex = static_cast<std::size_t>(micro.dest);

            if (micro.unit == UnitId::NONE) {
                registers_[destIndex] = registers_[srcIndex];

                // Track explicit writes to the architectural PC.
                if (destIndex == registerIndex(InternalRegister::PC)) {
                    pcModified_ = true;
                }
            }
                        
            else if (micro.unit == UnitId::ALU) {
                const std::int32_t operandA =
                    static_cast<std::int32_t>(registers_[srcIndex]);

                const std::int32_t operandB =
                    static_cast<std::int32_t>(registers_[destIndex]);

                std::int32_t result = 0;

                if (micro.opcode == control::isa::ADD) {
                    result = alu_.add(operandA, operandB);
                }
                else if (micro.opcode == control::isa::SUB) {
                    result = alu_.sub(operandA, operandB);
                }
                else if (micro.opcode == control::isa::MUL) {
                    result = alu_.mul(operandA, operandB);
                }
                else if (micro.opcode == control::isa::POW) {
                    result = alu_.pow(operandA, operandB);
                }
                else if (micro.opcode == control::isa::DIV) {
                    result = alu_.div(operandA, operandB);
                }
                else if (micro.opcode == control::isa::MOD) {
                    result = alu_.mod(operandA, operandB);
                }
                else if (micro.opcode == control::isa::CMP) {
                    alu_.cmp(operandA, operandB);

                    registers_[registerIndex(InternalRegister::FLAG_E)] =
                        alu_.getE() ? 1 : 0;

                    registers_[registerIndex(InternalRegister::FLAG_GT)] =
                        alu_.getGT() ? 1 : 0;

                    registers_[registerIndex(InternalRegister::FLAG_ST)] =
                        alu_.getST() ? 1 : 0;

                    result = 0;
                }
                else if (micro.opcode == control::isa::AND) {
                    result = alu_.AND(operandA, operandB);
                }
                else if (micro.opcode == control::isa::OR) {
                    result = alu_.OR(operandA, operandB);
                }
                else if (micro.opcode == control::isa::NOT) {
                    result = alu_.NOT(operandA);
                }
                else if (micro.opcode == control::isa::XOR) {
                    result = alu_.XOR(operandA, operandB);
                }
                else if (micro.opcode == control::isa::LSL) {
                    result = alu_.lsl(operandA, operandB);
                }
                else if (micro.opcode == control::isa::LSR) {
                    result = alu_.lsr(operandA, operandB);
                }
                else if (micro.opcode == control::isa::ASR) {
                    result = alu_.asr(operandA, operandB);
                }
                else if (micro.opcode == control::isa::ROTL) {
                    result = alu_.rotl(operandA, operandB);
                }
                else if (micro.opcode == control::isa::ROTR) {
                    result = alu_.rotr(operandA, operandB);
                }
                else if (micro.opcode == control::isa::PCNT) {
                    result = alu_.pcnt(operandA);
                }
                else {
                    throw std::invalid_argument(
                        "MMOV: unsupported ALU opcode"
                    );
                }

                registers_[registerIndex(InternalRegister::ALU_RESULT)] = result;
            }

            else if (micro.unit == UnitId::REGISTER_FILE) {
                const std::int64_t registerIndexValue = registers_[srcIndex];

                if (registerIndexValue < 0 ||
                    registerIndexValue >=
                        static_cast<std::int64_t>(
                            registers::RegisterFile::REGISTER_COUNT
                        )) {
                    throw std::out_of_range(
                        "MMOV: invalid architectural register index"
                    );
                }

                const auto registerIndex =
                    static_cast<std::size_t>(registerIndexValue);

                if (micro.opcode ==
                    static_cast<int>(RegisterFileAction::READ)) {
                    // Read the architectural register into the internal destination.
                    registers_[destIndex] = registerFile_.read(registerIndex);
                }
                else if (micro.opcode ==
                        static_cast<int>(RegisterFileAction::WRITE)) {
                    // Write the internal source value to the architectural register.
                    registerFile_.write(
                        registerIndex,
                        static_cast<std::int32_t>(registers_[destIndex])
                    );
                }
                else {
                    throw std::invalid_argument(
                        "MMOV: invalid register-file action"
                    );
                }
            }

            else if (micro.unit == UnitId::FPU) {
                // Treat internal register values as raw IEEE-754 bit patterns.
                const std::uint32_t operandA =
                    static_cast<std::uint32_t>(
                        static_cast<std::int32_t>(registers_[srcIndex])
                    );

                const std::uint32_t operandB =
                    static_cast<std::uint32_t>(
                        static_cast<std::int32_t>(registers_[destIndex])
                    );

                std::uint32_t result = 0;

                switch (micro.opcode) {
                    case isa::FADD:
                        result = fpu_.fadd(operandA, operandB);
                        break;

                    case isa::FSUB:
                        result = fpu_.fsub(operandA, operandB);
                        break;

                    case isa::FMUL:
                        result = fpu_.fmul(operandA, operandB);
                        break;

                    case isa::FDIV:
                        result = fpu_.fdiv(operandA, operandB);
                        break;

                    case isa::FCMP: {
                        const std::int32_t comparison =
                            fpu_.fcmp(operandA, operandB);

                        registers_[registerIndex(InternalRegister::ALU_RESULT)] =
                            comparison;

                        registers_[registerIndex(InternalRegister::FLAG_E)] =
                            fpu_.getE() ? 1 : 0;

                        registers_[registerIndex(InternalRegister::FLAG_GT)] =
                            fpu_.getGT() ? 1 : 0;

                        registers_[registerIndex(InternalRegister::FLAG_ST)] =
                            fpu_.getST() ? 1 : 0;

                        break;
                    }

                    case isa::SQRT:
                        result = fpu_.sqrt(operandA);
                        break;

                    case isa::FABS:
                        result = fpu_.fabs(operandA);
                        break;

                    case isa::SIN:
                        result = fpu_.sin(operandA);
                        break;

                    case isa::COS:
                        result = fpu_.cos(operandA);
                        break;

                    case isa::ACOS:
                        result = fpu_.acos(operandA);
                        break;

                    default:
                        throw std::invalid_argument(
                            "MMOV: unsupported FPU opcode"
                        );
                }

                // FCMP has already stored its signed comparison result.
                if (micro.opcode != isa::FCMP) {
                    registers_[registerIndex(InternalRegister::ALU_RESULT)] =
                        static_cast<std::int32_t>(result);
                }
            }

            else if (micro.unit == UnitId::MEMORY) {
                const auto addressValue = registers_[srcIndex];

                if (addressValue < 0) {
                    throw std::out_of_range(
                        "MMOV: negative memory address"
                    );
                }

                const auto address =
                    static_cast<std::size_t>(addressValue);

                if (micro.opcode ==
                    static_cast<int>(MemoryAction::READ_WORD)) {
                    registers_[destIndex] = memory_.readWord(address);
                }
                else if (micro.opcode ==
                        static_cast<int>(MemoryAction::WRITE_WORD)) {
                    memory_.writeWord(
                        address,
                        static_cast<std::uint32_t>(
                            registers_[destIndex]
                        )
                    );
                }
                else {
                    throw std::invalid_argument(
                        "MMOV: invalid memory action"
                    );
                }
            }

            else {
                throw std::invalid_argument(
                    "MMOV: unsupported functional unit"
                );
            }

            microPC_ = nextMicroPC;
            break;
        }

        case MicroType::MMOVI: {
            if (micro.dest < 0 ||
                static_cast<std::size_t>(micro.dest) >=
                    INTERNAL_REGISTER_COUNT) {
                throw std::out_of_range(
                    "MMOVI uses an invalid destination register"
                );
            }

            registers_[static_cast<std::size_t>(micro.dest)] =
                micro.uimm;
            microPC_ = nextMicroPC;
            break;
        }

        case MicroType::MADD: {
            if (micro.dest < 0 ||
                static_cast<std::size_t>(micro.dest) >=
                    INTERNAL_REGISTER_COUNT) {
                throw std::out_of_range(
                    "MADD uses an invalid destination register"
                );
            }

            registers_[static_cast<std::size_t>(micro.dest)] +=
                micro.uimm;
            microPC_ = nextMicroPC;
            break;
        }

        case MicroType::MB: {
            if (micro.seqAddr < 0 ||
                static_cast<std::size_t>(micro.seqAddr) >=
                    program_.size()) {
                throw std::out_of_range(
                    "MB target is outside Control Memory"
                );
            }

            const int currentMicroPC = microPC_;
            microPC_ = micro.seqAddr;

            // Convention: returning to address 0 from another
            // microaddress completes the current ISA instruction.
            if (currentMicroPC != 0 && microPC_ == 0) {
                instructionActive_ = false;
            }

            break;
        }

        case MicroType::MBEQ: {
            if (micro.src < 0 ||
                static_cast<std::size_t>(micro.src) >=
                    INTERNAL_REGISTER_COUNT) {
                throw std::out_of_range(
                    "MBEQ uses an invalid source register"
                );
            }

            if (micro.seqAddr < 0 ||
                static_cast<std::size_t>(micro.seqAddr) >=
                    program_.size()) {
                throw std::out_of_range(
                    "MBEQ target is outside Control Memory"
                );
            }

            
            const std::int64_t value =
                registers_[static_cast<std::size_t>(micro.src)];

            
            if (value == micro.uimm) {
                const int currentMicroPC = microPC_;
                microPC_ = micro.seqAddr;

                if (currentMicroPC != 0 && microPC_ == 0) {
                    instructionActive_ = false;
                }
            } else {
                microPC_ = nextMicroPC;
            }


            break;
        }

        case MicroType::MLOAD_IR:{
            // loadInstruction() has already placed the instruction in IR.
            // The real instruction-memory read will be connected later.
            microPC_ = nextMicroPC;
            break;
        }

        case MicroType::MDECODE: {
            const std::uint64_t instruction =
                static_cast<std::uint64_t>(
                    registers_[registerIndex(InternalRegister::IR)]
                );

            const DecodedInstruction decoded =
                InstructionDecoder::decode(instruction);

            registers_[registerIndex(InternalRegister::I)] =
                decoded.immediateMode ? 1 : 0;

            registers_[registerIndex(InternalRegister::RD)] = decoded.rd;
            registers_[registerIndex(InternalRegister::RS1)] = decoded.rs1;
            registers_[registerIndex(InternalRegister::RS2)] = decoded.rs2;

            registers_[registerIndex(InternalRegister::IMMX)] =
                decoded.immediate;

            registers_[registerIndex(InternalRegister::BRANCH_TARGET)] =
                decoded.isBranch ? decoded.branchOffset : 0;

            microPC_ = nextMicroPC;
            break;
        }

        case MicroType::MSWITCH: {
            const std::uint64_t instruction =
                static_cast<std::uint64_t>(
                    registers_[registerIndex(InternalRegister::IR)]
                );

            const std::uint8_t opcode =
                static_cast<std::uint8_t>((instruction >> 57) & 0x7F);

            const int entryAddress = program_.getEntryAddress(opcode);

            if (entryAddress < 0) {
                throw std::invalid_argument(
                    "MSWITCH: no routine registered for opcode"
                );
            }

            microPC_ = entryAddress;
            break;
        }

        default:
            throw std::logic_error(
                "Unknown microinstruction type"
            );
    }


    // Reaching the end of Control Memory ends this routine.
    if (microPC_ < 0 ||
        static_cast<std::size_t>(microPC_) >= program_.size()) {
        instructionActive_ = false;
    }

    return true;
}

std::int64_t ControlUnit::readInternal(
    InternalRegister reg
) const {
    const std::size_t index = registerIndex(reg);

    if (index >= INTERNAL_REGISTER_COUNT) {
        throw std::out_of_range(
            "Invalid internal register index"
        );
    }

    return registers_[index];
}

void ControlUnit::writeInternal(
    InternalRegister reg,
    std::int64_t value
) {
    const std::size_t index = registerIndex(reg);

    if (index >= INTERNAL_REGISTER_COUNT) {
        throw std::out_of_range(
            "Invalid internal register index"
        );
    }

    registers_[index] = value;
}

int ControlUnit::getMicroPC() const {
    return microPC_;
}

const MicrocodeProgram& ControlUnit::getProgram() const {
    return program_;
}

void ControlUnit::executeInstruction() {
    if (!programLoaded_) {
        throw std::runtime_error(
            "Cannot execute instruction: microcode is not loaded."
        );
    }

    if (!instructionActive_) {
        throw std::runtime_error(
            "Cannot execute instruction: no active instruction."
        );
    }

    // Each instruction is 8 bytes.
    const std::int64_t instructionAddress =
        registers_[registerIndex(InternalRegister::PC)];

    pcModified_ = false;

    std::size_t steps = 0;
    const std::size_t MAX_MICROSTEPS = 10000;

    while (instructionActive_) {
        if (++steps > MAX_MICROSTEPS) {
            throw std::runtime_error(
                "Microcode execution exceeded the maximum step limit."
            );
        }

        stepMicroinstruction();
    }

    // Advance PC only if no microinstruction changed it.
    if (!pcModified_) {
        registers_[registerIndex(InternalRegister::PC)] =
            instructionAddress + 8;
    }
}

std::int32_t ControlUnit::readRegister(std::size_t index) const {
    return registerFile_.read(index);
}

void ControlUnit::writeRegister(
    std::size_t index,
    std::int32_t value
) {
    registerFile_.write(index, value);
}

std::uint32_t ControlUnit::readMemoryWord(
    std::size_t address
) const {
    return memory_.readWord(address);
}

void ControlUnit::writeMemoryWord(
    std::size_t address,
    std::uint32_t value
) {
    memory_.writeWord(address, value);
}


} // namespace control
