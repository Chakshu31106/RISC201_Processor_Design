-- For compiling --
g++ -std=c++17 risc201_assembler.cpp -o assembler

g++ -std=c++17 risc201_disassembler.cpp -o disassembler  

-- For encoding asm file --
./assembler test.asm test.hex 

-- For decoding hex file -- 
./disassembler test.hex   


left - not and macro preprocesooor
