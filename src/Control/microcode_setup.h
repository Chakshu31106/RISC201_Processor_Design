
#ifndef MICROCODE_SETUP_H
#define MICROCODE_SETUP_H

#include "microcode_builder.h"
#include "arithmetic_microcode/add_microcode.h"
#include "arithmetic_microcode/sub_microcode.h"
#include "arithmetic_microcode/mul_microcode.h"
#include "arithmetic_microcode/div_microcode.h"
#include "arithmetic_microcode/mod_microcode.h"
#include "arithmetic_microcode/cmp_microcode.h"
#include "arithmetic_microcode/incr_microcode.h"
#include "arithmetic_microcode/decr_microcode.h"
#include "arithmetic_microcode/abs_microcode.h"
#include "arithmetic_microcode/pow_microcode.h"
#include "logical_microcode/and_microcode.h"
#include "logical_microcode/or_microcode.h"
#include "logical_microcode/not_microcode.h"
#include "logical_microcode/nor_microcode.h"
#include "logical_microcode/nand_microcode.h"
#include "logical_microcode/xor_microcode.h"
#include "logical_microcode/lsl_microcode.h"
#include "logical_microcode/lsr_microcode.h"
#include "logical_microcode/asr_microcode.h"
#include "logical_microcode/rotl_microcode.h"
#include "logical_microcode/rotr_microcode.h"
#include "logical_microcode/pcnt_microcode.h"
#include "logical_microcode/tog_microcode.h"
#include "logical_microcode/sbit_microcode.h"
#include "logical_microcode/cbit_microcode.h"
#include "branch_microcode/beq_microcode.h"
#include "branch_microcode/bgt_microcode.h"
#include "branch_microcode/bst_microcode.h"
#include "branch_microcode/b_microcode.h"
#include "branch_microcode/call_microcode.h"
#include "branch_microcode/ret_microcode.h"
#include "fpu_microcode/fadd_microcode.h"
#include "fpu_microcode/fsub_microcode.h"
#include "fpu_microcode/fmul_microcode.h"
#include "fpu_microcode/fdiv_microcode.h"
#include "fpu_microcode/fcmp_microcode.h"
#include "fpu_microcode/fsqrt_microcode.h"
#include "fpu_microcode/fabs_microcode.h"
#include "fpu_microcode/fsin_microcode.h"
#include "fpu_microcode/fcos_microcode.h"
#include "fpu_microcode/acos_microcode.h"
#include "vector_microcode/vadd_microcode.h"
#include "vector_microcode/vsub_microcode.h"
#include "vector_microcode/vdot_microcode.h"
#include "vector_microcode/vcrs_microcode.h"
#include "vector_microcode/vmag_microcode.h"
#include "memory_microcode/ld_microcode.h"
#include "memory_microcode/st_microcode.h"
#include "nop_microcode.h"
#include "mov_microcode.h"
#include "vector_microcode/vld_microcode.h"
#include "vector_microcode/vst_microcode.h"
#include "vector_microcode/vang_microcode.h"
#include "stack_microcode/push_microcode.h"
#include "stack_microcode/pop_microcode.h"
#include "stack_microcode/mpop_microcode.h"
#include "stack_microcode/peek_microcode.h"
#include "stack_microcode/clr_microcode.h"

namespace control {

inline MicrocodeProgram buildDefaultMicrocode() {
    MicrocodeBuilder builder;
    MicrocodeProgram program;

    // Shared preamble starts at microaddress 0.
    builder.beginRoutine("PREAMBLE", -1);

    // Alias for the existing ADD routine's return target.
    builder.label("MICROCODE_END");

    Microinstruction instruction{};

    instruction.type = MicroType::MLOAD_IR;
    builder.append(instruction);

    instruction = Microinstruction{};
    instruction.type = MicroType::MDECODE;
    builder.append(instruction);

    instruction = Microinstruction{};
    instruction.type = MicroType::MSWITCH;
    builder.append(instruction);

    builder.finishRoutine();

    // Add the opcode-specific ADD routine.
    buildAddRoutine(builder);
    buildSubRoutine(builder);
    buildMulRoutine(builder);
    buildDivRoutine(builder);
    buildModRoutine(builder);
    buildCmpRoutine(builder);
    buildBeqRoutine(builder);
    buildBgtRoutine(builder);
    buildBstRoutine(builder);
    buildBRoutine(builder);
    buildIncrRoutine(builder);
    buildDecrRoutine(builder);
    buildPowRoutine(builder);
    buildAbsRoutine(builder);
    buildAndRoutine(builder);
    buildOrRoutine(builder);
    buildNotRoutine(builder);
    buildNorRoutine(builder);
    buildNandRoutine(builder);
    buildXorRoutine(builder);
    buildLslRoutine(builder);
    buildLsrRoutine(builder);
    buildAsrRoutine(builder);
    buildRotlRoutine(builder);
    buildRotrRoutine(builder);
    buildPcntRoutine(builder);
    buildTogRoutine(builder);
    buildSbitRoutine(builder);
    buildCbitRoutine(builder);
    buildCallRoutine(builder);
    buildRetRoutine(builder);
    buildFaddRoutine(builder);
    buildFsubRoutine(builder);
    buildFmulRoutine(builder);
    buildFdivRoutine(builder);
    buildFcmpRoutine(builder);
    buildSqrtRoutine(builder);
    buildFabsRoutine(builder);
    buildSinRoutine(builder);
    buildCosRoutine(builder);
    buildAcosRoutine(builder);
    buildVaddRoutine(builder);
    buildVsubRoutine(builder);
    buildVdotRoutine(builder);
    buildVcrsRoutine(builder);
    buildVmagRoutine(builder);
    buildLdRoutine(builder);
    buildStRoutine(builder);
    buildNopRoutine(builder);
    buildMovRoutine(builder);
    buildVldRoutine(builder);
    buildVstRoutine(builder);
    buildVangRoutine(builder);
    buildPushRoutine(builder);
    buildPopRoutine(builder);
    buildMpopRoutine(builder);
    buildPeekRoutine(builder);
    buildClrRoutine(builder);

    // Resolve labels and load all routines into Control Memory.
    builder.finalize(program);

    return program;
}

} // namespace control

#endif
