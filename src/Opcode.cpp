#include "CPU.hpp"
/* OPCODE DEFINITIONS */
/* All opcode cycles use m-cycles */

/* Decode an opcode so we can use our algorithm
   to figure out what instruction we do */
void CPU::decode()
{
    XX  = (opcode & 0b11000000) >> 6;
    YYY = (opcode & 0b00111000) >> 3;
    ZZZ = (opcode & 0b00000111);

    P = (YYY & 0b110) >> 1;
    Q = YYY & 0b1;
}

/* OPCODE HELPERS */

bool CPU::passedCondition(uint8_t condition)
{
    bool passed{false};
    
    uint8_t zBit{Helper::getBit(registers[R_F], F_Z)};
    uint8_t cBit{Helper::getBit(registers[F_Z], F_C)};
    
    switch(condition)
    {
        
        case F_NO_Z:
            passed = !zBit;
            break;
        case F_YES_Z:
            passed = zBit;
            break;
        case F_NO_C:
            passed = !cBit;
            break;
        case F_YES_C:
            passed = cBit;
            break;
    }

    return passed;
}

void CPU::add16Bit(uint16_t operand1, uint16_t operand2)
{
    Helper::resetBit(registers[R_F], F_N);

    // Probably the hardest flag and this might be wrong too
    /* Algorithm: Sum everything up to and including 11th bit 
                  If the 12th bit ticks up to 1 set half carry flag */
    uint16_t halfSum{(operand1 & 0xFFF) + (operand2 & 0xFFF)};
    if(Helper::getBit(halfSum, 12))
    {
        Helper::setBit(registers[R_F], F_H);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_H);
    }

    if(operand1 > (0xFFFF - operand2))
    {
        Helper::setBit(registers[R_F], F_C);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_C);
    }


}

/* Just calculates flags set for an 8 bit add */
void CPU::add8Bit(uint8_t operand1, uint8_t operand2)
{
    Helper::resetBit(registers[R_F], F_N);

    uint8_t halfSum{(operand1 & 0xF) + (operand2 & 0xF)};

    if(Helper::getBit(halfSum, 4))
    {
        Helper::setBit(registers[R_F], F_H);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_H);
    }

    if(operand1 > (0xFF - operand2))
    {
        Helper::setBit(registers[R_F], F_C);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_C);
    }

    if((operand1 + operand2) == 0)
    {
        Helper::setBit(registers[R_F], F_Z);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_Z);

    }

}

/* Sets all flags needed for subtraction on 8 bits */
void CPU::sub8Bit(uint8_t minuend, uint8_t subtrahend)
{
    if ((minuend - subtrahend) == 0)
    {
        Helper::setBit(registers[R_F], F_Z);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_Z);
    }

    Helper::setBit(registers[R_F], F_N);

    uint8_t minuNib{(minuend & 0b1111)};
    uint8_t subtraNib{(subtrahend & 0b1111)};

    if(subtraNib > minuNib)
    {
        Helper::setBit(registers[R_F], F_H);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_H);
    }

    if(subtrahend > minuend)
    {
        Helper::setBit(registers[R_F], F_C);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_C);
    }
}

void CPU::inc8Bit(uint8_t data)
{
    uint8_t carry{Helper::getBit(registers[R_F], F_C)};

    add8Bit(data, 1);

    if(carry)
    {
        Helper::setBit(registers[R_F], F_C);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_C);
    }
}
void CPU::dec8Bit(uint8_t data)
{
    uint8_t carry{Helper::getBit(registers[R_F], F_C)};

    sub8Bit(data, 1);

    if(carry)
    {
        Helper::setBit(registers[R_F], F_C);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_C);
    }
}

/* OPCODES BEGIN */
/* Does nothing */
void CPU::opNOP()
{
    cycleCount++;
}

/* Store value in SP at address pointed to by immediate data
   Use little endian format (LSB at earlier address)*/
void CPU::opLDnnSP()
{
    uint8_t upperBits{(sp & 0xFF00) >> 8};
    uint8_t lowerBits{sp & 0xFF};
    uint8_t addressLSB{readMemory(pc)};
    uint8_t addressMSB{readMemory(pc)};
    uint16_t fullAddress{Helper::concatChar(addressMSB, addressLSB)};

    writeMemory(fullAddress, lowerBits);
    fullAddress++;
    writeMemory(fullAddress, upperBits);

    cycleCount += 5;
}

/* Enter low power mode, doesn't take any cycles 
   Also double speed switch in GBC*/
void CPU::opSTOP()
{
    // UNIMPLEMENTED
    return;
}

/* Relative jump to 16 bit address, using signed 8 bit immediate */
void CPU::opJRd()
{
    int8_t offset{readMemory(pc)};

    pc = pc + offset;

    cycleCount += 3;
}

/* Same as above but only conditionally jumps */
void CPU::opJRccd()
{
    if(passedCondition(YYY - 4))
    {
        opJRd();
    }
    else
    {
        cycleCount += 2;
    }
}

/* Loads immediate 16 bit data into 16 bit register */
void CPU::opLDrpnn()
{
    uint8_t immLSB{readMemory(pc)};
    uint8_t immMSB{readMemory(pc)};

    switch(P)
    {
        case 0:
            registers[1] = immLSB;
            registers[0] = immMSB;
            break;
        case 1:
            registers[3] = immLSB;
            registers[2] = immMSB;
            break;
        case 2:
            registers[5] = immLSB;
            registers[4] = immMSB;
            break;
        case 3:
            sp = Helper::concatChar(immMSB, immLSB);
            break;
    }

    cycleCount += 3;
}

/* Adds HL with some 16 bit register and store in HL */
void CPU::opADDHLrp()
{
    uint16_t originalHL{Helper::concatChar(registers[R_H], registers[R_L])};
    uint16_t fullShort{0};
    switch(P)
    {
        case 0:
            fullShort = Helper::concatChar(registers[R_B], registers[R_C]);
            break;
        case 1:
            fullShort = Helper::concatChar(registers[R_D], registers[R_E]);
            break;
        case 2:
            fullShort = Helper::concatChar(registers[R_H], registers[R_L]);
            break;
        case 3:
            fullShort = sp;
            break;
    }
    
    uint16_t sum{originalHL + fullShort};
    registers[R_H] = Helper::hiBits(sum);
    registers[R_L] = Helper::loBits(sum);

    add16Bit(originalHL, fullShort);

    cycleCount += 2;
}

/* Load value of register A into address stored in 16 bit register */
void CPU::opLDrrA()
{
    uint8_t MSB{0};
    uint8_t LSB{0};
    switch(P)
    {
        case 0:
            MSB = registers[R_B];
            LSB = registers[R_C];
            break;
        case 1:
            MSB = registers[R_D];
            LSB = registers[R_E];
            break;
    }
    uint16_t addr{Helper::concatChar(MSB, LSB)};
    writeMemory(addr, registers[R_A]);

    cycleCount += 2;

}

/* Load value in register A into address pointed to by HL then increment/decrement HL */
void CPU::opLDHLIDA()
{
    uint16_t combinedHL{Helper::concatChar(registers[R_H], registers[R_L])};
    writeMemory(combinedHL, registers[R_A]);

    switch(P)
    {
        case 2:
            combinedHL++;
            break;
        case 3:
            combinedHL--;
            break;
    }

    registers[R_H] = Helper::hiBits(combinedHL);
    registers[R_L] = Helper::loBits(combinedHL);
    cycleCount += 2;
}

/* Load value from memory at address given by register value into A*/
void CPU::opLDArr()
{
    uint8_t MSB{0};
    uint8_t LSB{0};
    switch(P)
    {
        case 0:
            MSB = registers[R_B];
            LSB = registers[R_C];
            break;
        case 1:
            MSB = registers[R_D];
            LSB = registers[R_E];
            break;
    }

    uint16_t addr{Helper::concatChar(MSB, LSB)};

    registers[R_A] = readMemory(addr);
    pc--;
    cycleCount += 2;
}

/* Load value at address HL into register A then increment or decrement HL */
void CPU::opLDAHLID()
{
    uint16_t combinedHL{Helper::concatChar(registers[R_H], registers[R_L])}; 

    registers[R_A] = readMemory(combinedHL);
    pc--;

    switch(P)
    {
        case 2:
            combinedHL++;
            break;
        case 3:
            combinedHL--;
            break;
    }

    registers[R_H] = Helper::hiBits(combinedHL);
    registers[R_L] = Helper::loBits(combinedHL);
    cycleCount += 2;  
}

/* Increments or decrements 16 bit register, no flags set */
void CPU::opINCDECrp()
{
    uint16_t concatenated{0};
    bool onSP{false};

    uint8_t hiReg{0};
    uint8_t loReg{0};
    int8_t operand{0};

    switch(P)
    {
        case 0:
            hiReg = R_B;
            loReg = R_C;
            break;
        case 1:
            hiReg = R_D;
            loReg = R_E;
            break;
        case 2:
            hiReg = R_H;
            loReg = R_L;
            break;
        case 3:
            onSP = true;
            break;
    }
    switch(Q)
    {
        case 0: // incrementing
            operand = 1;
            break;
        case 1: // decrementing
            operand = -1;
            break;
    }
    
    if(!onSP)
    {
        concatenated = Helper::concatChar(registers[hiReg], registers[loReg]);
        concatenated += operand;

        registers[hiReg] = Helper::hiBits(concatenated);
        registers[loReg] = Helper::loBits(concatenated);
    }
    else
    {
        sp += operand;
    }
    cycleCount += 2;
}

void CPU::opINCDECr()
{
    int8_t operand{0};

    switch(Q)
    {
        case 0:
            operand = 1;
            break;
        case 1:
            operand = -1;
            break;
    }

    uint8_t originalVal = registers[YYY];
    registers[YYY] += operand;

    if(operand > 0)
    {
        inc8Bit(originalVal);
    }
    else
    {
        dec8Bit(originalVal);
    }

    cycleCount += 1;
}

/* Load immediate data into register */
void CPU::opLDrn()
{
    uint8_t regIndex{YYY};
    registers[regIndex] = readMemory(pc);

    cycleCount += 2;
}

void CPU::opRLCA()
{
    uint8_t MSB{Helper::getBit(registers[R_A], 7)};

    registers[R_A] = registers[R_A] << 1;
    if(MSB == 1)
    {
        registers[R_A] |= 0b1;
        Helper::setBit(registers[R_F], R_C);
    }
    else
    {
        Helper::resetBit(registers[R_F], R_C);
    }

    Helper::resetBit(registers[R_F], F_Z);
    Helper::resetBit(registers[R_F], F_N);
    Helper::resetBit(registers[R_F], F_H);

    

    cycleCount += 1;
}

void CPU::opRRCA()
{
    uint8_t LSB{Helper::getBit(registers[R_A], 0)};

    registers[R_A] = registers[R_A] >> 1;
    if(LSB == 1)
    {
        registers[R_A] |= 0b10000000;
        Helper::setBit(registers[R_F], R_C);
    }
    else
    {
        Helper::resetBit(registers[R_F], R_C);
    }

    Helper::resetBit(registers[R_F], F_Z);
    Helper::resetBit(registers[R_F], F_N);
    Helper::resetBit(registers[R_F], F_H);

    

    cycleCount += 1;
}

void CPU::opRLA()
{
    uint8_t carryIn{Helper::getBit(registers[R_F], F_C)};
    uint8_t MSB{Helper::getBit(registers[R_A], 7)};

    registers[R_A] = registers[R_A] << 1;

    if(carryIn == 1)
    {
        registers[R_A] |= 0b1;
    }

    if(MSB == 1)
    {
        Helper::setBit(registers[R_F], F_C);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_C);
    }

    Helper::resetBit(registers[R_F], F_Z);
    Helper::resetBit(registers[R_F], F_N);
    Helper::resetBit(registers[R_F], F_H);

    cycleCount += 1;
}

void CPU::opRRA()
{
    uint8_t carryIn{Helper::getBit(registers[R_F], F_C)};
    uint8_t LSB{Helper::getBit(registers[R_A], 0)};

    registers[R_A] = registers[R_A] >> 1;

    if(carryIn == 1)
    {
        registers[R_A] |= 0b10000000;
    }

    if(LSB == 1)
    {
        Helper::setBit(registers[R_F], F_C);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_C);
    }

    Helper::resetBit(registers[R_F], F_Z);
    Helper::resetBit(registers[R_F], F_N);
    Helper::resetBit(registers[R_F], F_H);

    cycleCount += 1;
}

void CPU::opDAA()
{

}

/* Complements the accumulator */
void CPU::opCPL()
{
    registers[R_A] = ~registers[R_A];

    Helper::setBit(registers[R_F], F_N);
    Helper::setBit(registers[R_F], F_H);

    cycleCount += 1;
}

/* Sets carry flag and resets some other ones */
void CPU::opSCF()
{
    Helper::resetBit(registers[R_F], F_N);
    Helper::resetBit(registers[R_F], F_H);
    Helper::setBit(registers[R_F], F_C);
    cycleCount += 1;
}

/* Complement the carry flag */
void CPU::opCCF()
{
    registers[R_F] ^= (0b1 << F_C);

    Helper::resetBit(registers[R_F], F_N);
    Helper::resetBit(registers[R_F], F_H);

    cycleCount += 1;
}

void CPU::opHALT()
{

}

/* Load second registers into first register */
void CPU::opLDrr()
{
    if((YYY != R_HL) & (ZZZ) != R_HL)
    {
        registers[YYY] = registers[ZZZ];

        cycleCount += 1;
    }
    else
    {
        // Load into memory location pointed to by HL
        if(YYY == R_HL) // YYY == R_HL and ZZZ == R_HL won't both be true ever in this opcode
        {
            writeMemory(Helper::concatChar(registers[R_H], registers[R_L]), registers[ZZZ]);
        }

        // Load memory at HL into register
        else
        {
            registers[YYY] = readMemory(Helper::concatChar(registers[R_H], registers[R_L]));
            pc--;
        }
        
        cycleCount += 2;
    }
    
}

/* Add register into A (also covers ADC) */
void CPU::opADDAr()
{
    uint8_t cycleTime{1};
    uint8_t operand{0};
    uint8_t regVal{registers[R_A]};
    if(ZZZ == 6)
    {
        cycleTime = 2;
        operand = readMemory(Helper::concatChar(registers[R_H], registers[R_L]));
        pc--;
    }
    else
    {
        operand = registers[ZZZ];
    }

    if(YYY == 1) // add carry bit to operand
    {
        if(Helper::getBit(registers[R_F], R_C) == 1)
        {
            operand++;
        }
    }

    registers[R_A] += operand;

    add8Bit(regVal, operand);
    cycleCount += cycleTime;
}

void CPU::opSUBAr()
{
    uint8_t cycleTime{1};
    uint8_t operand{0};
    uint8_t regVal{registers[R_A]};

    if(ZZZ == 6)
    {
        cycleTime = 2;
        operand = readMemory(Helper::concatChar(registers[R_H], registers[R_L]));
        pc--;
    }
    else
    {
        operand = registers[ZZZ];
    }

    if(YYY == 3) // add carry bit to operand
    {
        if(Helper::getBit(registers[R_F], R_C) == 1)
        {
            operand++;
        }
    }

    registers[R_A] -= operand;

    sub8Bit(regVal, operand);
    cycleCount += cycleTime;
}
            
void CPU::opANDAr()
{
    uint8_t cycleTime{1};
    uint8_t operand{0};

    if(ZZZ == 6)
    {
        cycleTime = 2;
        operand = readMemory(Helper::concatChar(registers[R_H], registers[R_L]));
        pc--;
    }
    else
    {
        operand = registers[ZZZ];
    }

    registers[R_A] &= operand;
    if(registers[R_A] == 0)
    {
        Helper::setBit(registers[R_F], F_Z);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_Z);
    }

    Helper::resetBit(registers[R_F], F_N);
    Helper::setBit(registers[R_F], F_H);
    Helper::resetBit(registers[R_F], F_C);

    cycleCount += cycleTime;

}
            
void CPU::opXORAr()
{
    uint8_t cycleTime{1};
    uint8_t operand{0};
    if(ZZZ == 6)
    {
        cycleTime = 2;
        operand = readMemory(Helper::concatChar(registers[R_H], registers[R_L]));
        pc--;
    }
    else
    {
        operand = registers[ZZZ];
    }

    registers[R_A] ^= operand;

    if(registers[R_A] == 0)
    {
        Helper::setBit(registers[R_F], F_Z);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_Z);
    }

    Helper::resetBit(registers[R_F], F_N);
    Helper::resetBit(registers[R_F], F_H);
    Helper::resetBit(registers[R_F], F_C);

    cycleCount += cycleTime;
}
            
void CPU::opORAr()
{
    uint8_t cycleTime{1};
    uint8_t operand{0};
    if(ZZZ == 6)
    {
        cycleTime = 2;
        operand = readMemory(Helper::concatChar(registers[R_H], registers[R_L]));
        pc--;
    }
    else
    {
        operand = registers[ZZZ];
    }

    registers[R_A] |= operand;

    if(registers[R_A] == 0)
    {
        Helper::setBit(registers[R_F], F_Z);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_Z);
    }

    Helper::resetBit(registers[R_F], F_N);
    Helper::resetBit(registers[R_F], F_H);
    Helper::resetBit(registers[R_F], F_C);

    cycleCount += cycleTime;
}
            
void CPU::opCPAr()
{
    uint8_t cycleTime{1};
    uint8_t operand{0};
    if(ZZZ == 6)
    {
        cycleTime = 2;
        operand = readMemory(Helper::concatChar(registers[R_H], registers[R_L]));
        pc--;
    }
    else
    {
        operand = registers[ZZZ];
    }

    sub8Bit(registers[R_A], operand);

    cycleCount += cycleTime;
}

void CPU::opRETcc()
{
    if(passedCondition(YYY))
    {
        opRET();
        cycleCount += 1;
    }
    else
    {
        cycleCount += 2;
    }
}

void CPU::opLDoffnA()
{
    uint16_t addr{Helper::concatChar(0xFF00, readMemory(pc))};

    writeMemory(addr, registers[R_A]);
    cycleCount += 3;
}

void CPU::opADDSPd()
{
    int8_t operand{0xFF & readMemory(pc)};
    uint8_t originalSP{sp & 0xFF};

    sp += operand;

    if(operand < 0)
    {
        sub8Bit(originalSP, operand);
    }
    else
    {
        add8Bit(originalSP, operand);
    }

    Helper::resetBit(registers[R_F], F_Z);
    Helper::resetBit(registers[R_F], F_N);
    cycleCount += 4;
}

void CPU::opLDAoffn()
{
    uint16_t addr{Helper::concatChar(0xFF, readMemory(pc))};
    registers[R_A] = readMemory(addr);
    pc--;

    cycleCount += 3;
}

void CPU::opLDHLSPId()
{
    int8_t operand{0xFF & readMemory(pc)};
    uint8_t originalSP{sp & 0xFF};
    uint16_t sum{sp + operand};
    
    registers[R_H] = Helper::hiBits(sum);
    registers[R_L] = Helper::loBits(sum);

    if(operand < 0)
    {
        sub8Bit(originalSP, operand);
    }
    else
    {
        add8Bit(originalSP, operand);
    }

    Helper::resetBit(registers[R_F], F_Z);
    Helper::resetBit(registers[R_F], F_N);
    cycleCount += 3;
}