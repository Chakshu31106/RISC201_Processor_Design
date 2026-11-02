cmake_minimum_required(VERSION 3.16)
project(risc201_processor_design LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(BUILD_TESTS "Build the test suites" ON)

# Headers are included relative to src/ (for example "datapath/memory.h"). src/Control is
# also on the include path because the control-unit files and tests use bare names such as "control_unit.h".
add_library(project_options INTERFACE)
target_include_directories(project_options INTERFACE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/Control)
# risc201_common.h carries its own definitions; only risc201_common.cpp compiles them (see that file).
target_compile_definitions(project_options INTERFACE RISC201_COMMON_NO_IMPLEMENTATION)
target_compile_options(project_options INTERFACE
  $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra>)

# add_lib(<name> <dir under src/> <deps...>): static library of every .cpp in that
# folder except programs that define main() and standalone test programs.
function(add_lib name dir)
  file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/${dir}/*.cpp)
  list(FILTER sources EXCLUDE REGEX "(main|risc201_assembler|risc201_disassembler|test_fpu|test_div|isa_pipeline_sim)\\.cpp$")
  add_library(${name} STATIC ${sources})
  target_link_libraries(${name} PUBLIC project_options ${ARGN})
endfunction()

#       name        folder        dependencies
add_lib(isa         ISA)
add_lib(alu         ALU/alu)
add_lib(fpu         ALU/fpu)
add_lib(datapath    datapath      isa)
add_lib(assembler   Assembler)
add_lib(control     Control       isa alu fpu datapath)
add_lib(simulator   Simulator     isa alu fpu datapath assembler control)

# ---- Programs ----
add_executable(processor src/Simulator/main.cpp)            # simulator / debugger
target_link_libraries(processor PRIVATE simulator)

add_executable(assembler_cli src/Assembler/risc201_assembler.cpp)   # .asm -> .hex + .bin
set_target_properties(assembler_cli PROPERTIES OUTPUT_NAME assembler)
target_link_libraries(assembler_cli PRIVATE assembler)

# Team's standalone pipeline timing tool (own parser, includes <bits/stdc++.h>, so GCC/Clang only).
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_executable(isa_sim src/Simulator/isa_pipeline_sim.cpp)
  target_compile_options(isa_sim PRIVATE -O2)
endif()

add_executable(disassembler src/Assembler/risc201_disassembler.cpp)
target_link_libraries(disassembler PRIVATE assembler)

# ---- Tests ----
if(BUILD_TESTS)
  enable_testing()

  # Simulator, assembler and ALU unit tests (one binary).
  add_executable(unit_tests
    tests/test_main.cpp tests/alu_tests.cpp tests/assembler_tests.cpp tests/simulator_tests.cpp)
  target_link_libraries(unit_tests PRIVATE simulator)
  target_compile_definitions(unit_tests PRIVATE RISC201_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
  add_test(NAME unit_tests COMMAND unit_tests)

  # Control-unit checks: each src/Control/tests/check_*.cpp is its own program.
  file(GLOB checks CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/Control/tests/check_*.cpp)
  foreach(src ${checks})
    get_filename_component(name ${src} NAME_WE)
    add_executable(${name} ${src})
    target_link_libraries(${name} PRIVATE control)
    set_target_properties(${name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/control_unit)
    add_test(NAME ${name} COMMAND ${name})
  endforeach()

  # FPU self-check program (prints results; not a pass/fail test).
  add_executable(test_fpu src/ALU/fpu/test_fpu.cpp)
  target_link_libraries(test_fpu PRIVATE fpu)
endif()
