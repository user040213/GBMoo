#include "CPU.hpp"

void CPU::readMemory(uint16_t addr)
{
    
}

void CPU::decode()
{
    XX  = (opcode & 0b11000000) >> 6;
    YYY = (opcode & 0b00111000) >> 3;
    ZZZ = (opcode & 0b00000111);

    P = (YYY & 0b110) >> 1;
    Q = YYY & 0b1;
}

void CPU::opNOP()
{

}

void CPU::opLDnnSP()
{

}

void CPU::opSTOP()
{

}

void CPU::opJRd()
{

}

void CPU::opJRccd()
{

}