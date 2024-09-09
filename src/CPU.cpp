#include "CPU.hpp"

CPU::CPU()
{
    // Register resetting
    memset(registers, 0, 8);

    sp = 0;
    pc = 0;

    // Initialize memory map
    memMap = std::vector<uint8_t>(MEM_MAP_SIZE, 0); 

    // resetting opcode decode variables
    XX = 0;
    YYY = 0;
    ZZZ = 0;

    P = 0;
    Q = 0;
}