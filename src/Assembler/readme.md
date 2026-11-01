-- For compiling --
g++ -std=c++17 risc201_assembler.cpp -o assembler

g++ -std=c++17 risc201_disassembler.cpp -o disassembler  

-- For encoding asm file --
./assembler all_tests.asm all_tests.hex 

-- For decoding hex file -- 
./disassembler all_tests.hex   