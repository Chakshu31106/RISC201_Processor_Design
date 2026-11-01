#include <cstdint>
#include <cstring>
#include <iostream>
#include <cmath>
#include <exception>
#include "control_unit.h"
#include "microcode_setup.h"
#include "isa_opcodes.h"
static std::int32_t floatToBits(float x) { std::uint32_t u; std::memcpy(&u,&x,4); std::int32_t s; std::memcpy(&s,&u,4); return s; }
static float bitsToFloat(std::int32_t x) { float f; std::memcpy(&f,&x,4); return f; }
int main() {
 try {
  control::ControlUnit cu; cu.loadProgram(control::buildDefaultMicrocode());
  cu.writeRegister(1,floatToBits(0.5f));
  std::uint64_t ins=(std::uint64_t(control::isa::ACOS)<<57)|(std::uint64_t(3)<<52)|(std::uint64_t(1)<<47);
  cu.loadInstruction(ins,0); cu.executeInstruction();
  float got=bitsToFloat(cu.readRegister(3));
  std::cout << "ACOS result: " << got << '\n';
  if (std::fabs(got-std::acos(0.5f))>1e-5f) return 1;
  std::cout << "ACOS execution test passed!\n"; return 0;
 } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
