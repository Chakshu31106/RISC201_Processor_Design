
#include "ALU/alu/alu.h"
#include "test_framework.h"



TEST(alu_add_carries_across_blocks) {
    ALU alu;
    CHECK_EQ(alu.add(2, 3), 5);
    CHECK_EQ(alu.add(-1, 1), 0);
    CHECK_EQ(alu.add(0xFF, 1), 0x100);
    CHECK_EQ(alu.add(0xFFFF, 1), 0x10000);
    CHECK_EQ(alu.add(0x00FFFFFF, 1), 0x01000000);
    CHECK_EQ(alu.add(-5, -7), -12);
}

TEST(alu_sub) {
    ALU alu;
    CHECK_EQ(alu.sub(10, 3), 7);
    CHECK_EQ(alu.sub(3, 10), -7);
    CHECK_EQ(alu.sub(0, 0), 0);
    CHECK_EQ(alu.sub(0x100, 1), 0xFF);
}

TEST(alu_mul_booth) {
    ALU alu;
    CHECK_EQ(alu.mul(6, 7), 42);
    CHECK_EQ(alu.mul(-6, 7), -42);
    CHECK_EQ(alu.mul(-6, -7), 42);
    CHECK_EQ(alu.mul(0, 5), 0);
    CHECK_EQ(alu.mul(123456, 1000), 123456000);
    CHECK_EQ(alu.mul(24, 5), 120);
}

TEST(alu_div_and_mod_signs) {
    ALU alu;
    CHECK_EQ(alu.div(20, 3), 6);
    CHECK_EQ(alu.div(-20, 3), -6);
    CHECK_EQ(alu.div(20, -3), -6);
    CHECK_EQ(alu.div(-20, -3), 6);
    CHECK_EQ(alu.mod(20, 3), 2);
    CHECK_EQ(alu.mod(-20, 3), -2);
    CHECK_EQ(alu.mod(20, -3), 2);
}

TEST(alu_division_by_zero_placeholder_returns_zero) {
    // Placeholder convention until the ISA / exception framework decides.
    ALU alu;
    CHECK_EQ(alu.div(5, 0), 0);
    CHECK_EQ(alu.mod(5, 0), 0);
}

TEST(alu_pow_incr_decr_abs) {
    ALU alu;
    CHECK_EQ(alu.pow(2, 10), 1024);
    CHECK_EQ(alu.pow(3, 0), 1);
    CHECK_EQ(alu.pow(-2, 3), -8);
    CHECK_EQ(alu.pow(5, -1), 0);
    CHECK_EQ(alu.incr(-1), 0);
    CHECK_EQ(alu.decr(0), -1);
    CHECK_EQ(alu.abs(-9), 9);
    CHECK_EQ(alu.abs(9), 9);
}

TEST(alu_logic) {
    ALU alu;
    CHECK_EQ(alu.AND(12, 10), 8);
    CHECK_EQ(alu.OR(12, 10), 14);
    CHECK_EQ(alu.XOR(12, 10), 6);
    CHECK_EQ(alu.NOT(0), -1);
    CHECK_EQ(alu.NOR(12, 10), -15);
    CHECK_EQ(alu.NAND(12, 10), -9);
}

TEST(alu_shifts_and_rotates) {
    ALU alu;
    CHECK_EQ(alu.lsl(1, 4), 16);
    CHECK_EQ(alu.lsl(1, 32), 0);
    CHECK_EQ(alu.lsr(-1, 28), 15);
    CHECK_EQ(alu.asr(-16, 2), -4);
    CHECK_EQ(alu.rotl(static_cast<int32_t>(0x80000001u), 1), 3);
    CHECK_EQ(alu.rotr(1, 1), static_cast<int32_t>(0x80000000u));
}

TEST(alu_bit_manipulation) {
    ALU alu;
    CHECK_EQ(alu.pcnt(0xFF), 8);
    CHECK_EQ(alu.pcnt(-1), 32);
    CHECK_EQ(alu.tog(0, 3), 8);
    CHECK_EQ(alu.tog(8, 3), 0);
    CHECK_EQ(alu.sbit(0, 31), static_cast<int32_t>(0x80000000u));
    CHECK_EQ(alu.cbit(15, 0), 14);
    CHECK_EQ(alu.tog(5, 40), 5);   // out-of-range bit index: unchanged
}

TEST(alu_cmp_sets_exactly_one_flag) {
    ALU alu;
    CHECK(!alu.getE() && !alu.getGT() && !alu.getST());
    alu.cmp(1, 2);
    CHECK(alu.getST() && !alu.getE() && !alu.getGT());
    alu.cmp(2, 1);
    CHECK(alu.getGT() && !alu.getE() && !alu.getST());
    alu.cmp(3, 3);
    CHECK(alu.getE() && !alu.getGT() && !alu.getST());
}

