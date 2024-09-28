#include "CPU.hpp"
#include "CPUDAATable.cpp"

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
    uint8_t cBit{Helper::getBit(registers[R_F], F_C)};
    
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
    pc++;
    uint8_t addressMSB{readMemory(pc)};
    pc++;
    uint16_t fullAddress{Helper::concatChar(addressMSB, addressLSB)};

    writeMemory(fullAddress, lowerBits);
    fullAddress++;
    writeMemory(fullAddress, upperBits);

    cycleCount += 5;
}

/* Enter low power mode, I found conflicting docs on whether this takes 1 or 0 m cycles
   Also double speed switch in GBC*/
void CPU::opSTOP()
{
    // UNIMPLEMENTED
    opHALT();
}

/* Relative jump to 16 bit address, using signed 8 bit immediate */
void CPU::opJRd()
{
    int8_t offset{readMemory(pc)};
    pc++;

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
        pc++;
        cycleCount += 2;
    }
}

/* Loads immediate 16 bit data into 16 bit register */
void CPU::opLDrpnn()
{
    uint8_t immLSB{readMemory(pc)};
    pc++;
    uint8_t immMSB{readMemory(pc)};
    pc++;

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

    Helper::resetBit(registers[R_F], F_N);

    /* Algorithm: Sum everything up to and including 11th bit 
                  If the 12th bit ticks up to 1 set half carry flag */
    uint16_t halfSum{(originalHL & 0xFFF) + (fullShort & 0xFFF)};
    if(Helper::getBit(halfSum, 12))
    {
        Helper::setBit(registers[R_F], F_H);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_H);
    }

    if(originalHL > (0xFFFF - fullShort))
    {
        Helper::setBit(registers[R_F], F_C);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_C);
    }

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
    cycleCount += 2;
}

/* Load value at address HL into register A then increment or decrement HL */
void CPU::opLDAHLID()
{
    uint16_t combinedHL{Helper::concatChar(registers[R_H], registers[R_L])}; 
    registers[R_A] = readMemory(combinedHL);

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
    uint8_t newVal{0};

    switch(ZZZ)
    {
        case 4:
            operand = 1;
            break;
        case 5:
            operand = -1;
            break;
    }
    uint8_t originalVal{0};
    if(YYY == R_HL)
    {
        cycleCount += 2;
        uint16_t addr{Helper::concatChar(registers[R_H], registers[R_L])};
        originalVal = readMemory(addr);
        newVal = originalVal + operand;
        writeMemory(addr, newVal);
    }
    else
    {
        originalVal = registers[YYY];
        registers[YYY] += operand;
        newVal = registers[YYY];
    }
    

    if(operand > 0)
    {
        if(newVal == 0)
        {
            Helper::setBit(registers[R_F], F_Z);
        }
        else
        {
            Helper::resetBit(registers[R_F], F_Z);
        }

	    Helper::resetBit(registers[R_F], F_N);

        if((originalVal & 0xF) == 0xF)
        {
            Helper::setBit(registers[R_F], F_H);
        }
        else
        {
            Helper::resetBit(registers[R_F], F_H);
        }
    }
    else
    {      
        if(newVal == 0)
        {
            Helper::setBit(registers[R_F], F_Z);
        }
        else
        {
            Helper::resetBit(registers[R_F], F_Z);
        }

	    Helper::setBit(registers[R_F], F_N);

        if((originalVal & 0xF) == 0x0)
        {
            Helper::setBit(registers[R_F], F_H);
        }
        else
        {
            Helper::resetBit(registers[R_F], F_H);
        }
    }

    cycleCount += 1;
}

/* Load immediate data into register */
void CPU::opLDrn()
{
    uint8_t regIndex{YYY};
    if(YYY == R_HL)
    {
        cycleCount += 1;
        uint16_t addr{Helper::concatChar(registers[R_H], registers[R_L])};
        uint8_t newVal = readMemory(pc);
        writeMemory(addr, newVal);
    }
    else
    {
        registers[regIndex] = readMemory(pc);
    }
    pc++;

    cycleCount += 2;
}

void CPU::opRLCA()
{
    uint8_t MSB{Helper::getBit(registers[R_A], 7)};

    registers[R_A] = registers[R_A] << 1;
    if(MSB == 1)
    {
        registers[R_A] |= 0b1;
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

void CPU::opRRCA()
{
    uint8_t LSB{Helper::getBit(registers[R_A], 0)};

    registers[R_A] = registers[R_A] >> 1;
    if(LSB == 1)
    {
        registers[R_A] |= 0b10000000;
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

/* Convert accumulator to BCD equivalent */
void CPU::opDAA()
{
    // max value of BCD is 1001 1001 (99)
    // we use an "error" offset to account for how 1001 is the max (as opposed to 1111)

    /* This is the only instruction 
        I could not get to work naturally, so I used a method from Antonio Niño Díaz, which
        works for all cases */

    uint32_t DAAIndex{(((uint32_t)registers[R_A]) << 4) | ((((uint32_t)registers[R_F] >> 4) & 7) << 1)};
    registers[R_A] = DAA_VAL_TABLE[DAAIndex];
    registers[R_F] = DAA_VAL_TABLE[DAAIndex + 1];
    cycleCount += 1;
    
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

// Conflicting docs on whether this is 0 or 1 cycle
// Halt bug not implemented
void CPU::opHALT()
{
    isHalted = true;
    if((memMap[0xFF0F] & memMap[0xFFFF]) != 0)
    {
        if(IMEflag)
        {
            isHalted = false;
            // we still do interrupt
        }
        else
        {
            isHalted = false;
            // also halt bug happens but its not implemented
        }
    }
    //cycleCount += 1;
}

/* Load second registers into first register */
void CPU::opLDrr()
{
    if((YYY != R_HL) && (ZZZ != R_HL))
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
    bool carry{false};
    if(ZZZ == 6)
    {
        cycleTime = 2;
        operand = readMemory(Helper::concatChar(registers[R_H], registers[R_L]));
    }
    else
    {
        operand = registers[ZZZ];
    }

    if(YYY == 1) // add carry bit to operand
    {
        if(Helper::getBit(registers[R_F], F_C) == 1)
        {
            carry = true;
        }
    }

    registers[R_A] += operand + carry;

    Helper::resetBit(registers[R_F], F_N);

    if((regVal & 0xF) + (operand & 0xF) + (uint8_t)carry > 0x0F)
    {
        Helper::setBit(registers[R_F], F_H);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_H);
    }

    if((uint16_t)regVal + (uint16_t)operand + (uint8_t)carry > 0xFF)
    {
        Helper::setBit(registers[R_F], F_C);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_C);
    }

    if((uint8_t)(regVal + operand + (uint8_t)carry) == 0)
    {
        Helper::setBit(registers[R_F], F_Z);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_Z);

    }
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
    }
    else
    {
        operand = registers[ZZZ];
    }

    registers[R_A] -= operand;

    if (regVal - operand == 0)
    {
        Helper::setBit(registers[R_F], F_Z);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_Z);
    }

    Helper::setBit(registers[R_F], F_N);

    uint8_t minuNib{(regVal & 0b1111)};
    uint8_t subtraNib{(operand & 0b1111)};

    if(subtraNib > minuNib)
    {
        Helper::setBit(registers[R_F], F_H);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_H);
    }

    if((operand) > regVal)
    {
        Helper::setBit(registers[R_F], F_C);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_C);
    }
    
    cycleCount += cycleTime;
}

void CPU::opSBCAr()
{
    uint8_t regVal{0};
    if(ZZZ == 6)
    {
        regVal = readMemory(Helper::concatChar(registers[R_H], registers[R_L]));
        cycleCount += 1;
    }
    else
    {
        regVal = registers[ZZZ];
    }
    int16_t operand = (int16_t)regVal & 0xFF;
    int16_t aReg = (int16_t)registers[R_A] & 0xFF;
    int aTemp = aReg;

    aTemp -= operand;

    if (Helper::getBit(registers[R_F], F_C))
    {
        aTemp -= 1;
    }

    Helper::setBit(registers[R_F], F_N);

    if(aTemp < 0)
    {
        Helper::setBit(registers[R_F], F_C);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_C);
    }

    aTemp &= 0xFF;

    if(aTemp == 0)
    {
        Helper::setBit(registers[R_F], F_Z);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_Z);
    }

    if (((aTemp ^ operand ^ aReg) & 0x10) == 0x10)
    {
        Helper::setBit(registers[R_F], F_H);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_H);
    }

    registers[R_A] = aTemp;
    cycleCount += 1;
}
            
void CPU::opANDAr()
{
    uint8_t cycleTime{1};
    uint8_t operand{0};

    if(ZZZ == 6)
    {
        cycleTime = 2;
        operand = readMemory(Helper::concatChar(registers[R_H], registers[R_L]));
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
    }
    else
    {
        operand = registers[ZZZ];
    }

    if ((registers[R_A] - operand) == 0)
    {
        Helper::setBit(registers[R_F], F_Z);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_Z);
    }

    Helper::setBit(registers[R_F], F_N);

    uint8_t minuNib{(registers[R_A] & 0b1111)};
    uint8_t subtraNib{(operand & 0b1111)};

    if(subtraNib > minuNib)
    {
        Helper::setBit(registers[R_F], F_H);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_H);
    }

    if(operand > registers[R_A])
    {
        Helper::setBit(registers[R_F], F_C);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_C);
    }

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

void CPU::opLDHnA()
{
    uint16_t addr{Helper::concatChar(0xFF, readMemory(pc))};
    pc++;

    writeMemory(addr, registers[R_A]);
    cycleCount += 3;
}

void CPU::opADDSPd()
{
    int8_t operand{0xFF & readMemory(pc)};
    pc++;
    uint8_t originalSP{sp & 0xFF};

    sp += operand;

    if((sp & 0xFF) < (originalSP & 0xFF))
    {
        Helper::setBit(registers[R_F], F_C);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_C);
    }
    if((sp & 0xF) < (originalSP & 0xF))
    {
        Helper::setBit(registers[R_F], F_H);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_H);
    }

    Helper::resetBit(registers[R_F], F_Z);
    Helper::resetBit(registers[R_F], F_N);
    cycleCount += 4;
}

void CPU::opLDHAn()
{
    uint16_t addr{0xFF00 + readMemory(pc)};
    pc++;
    registers[R_A] = readMemory(addr);

    cycleCount += 3;
}

void CPU::opLDHLSPId()
{
    int8_t operand{readMemory(pc)};
    pc++;
    uint16_t sum{sp + operand};
    
    registers[R_H] = Helper::hiBits(sum);
    registers[R_L] = Helper::loBits(sum);

    if((sum & 0xFF) < (sp & 0xFF))
    {
        Helper::setBit(registers[R_F], F_C);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_C);
    }
    if((sum & 0xF) < (sp & 0xF))
    {
        Helper::setBit(registers[R_F], F_H);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_H);
    }

    Helper::resetBit(registers[R_F], F_Z);
    Helper::resetBit(registers[R_F], F_N);

    Helper::resetBit(registers[R_F], F_Z);
    Helper::resetBit(registers[R_F], F_N);
    cycleCount += 3;
}

void CPU::opPOPrp2()
{
    uint8_t hiReg{0};
    uint8_t loReg{0};
    switch(P)
    {
        case 0: // BC
            hiReg = R_B;
            loReg = R_C;
            break;
        case 1: // DE
            hiReg = R_D;
            loReg = R_E;
            break;
        case 2: // HL
            hiReg = R_H;
            loReg = R_L;
            break;
        case 3: // AF
            hiReg = R_A;
            loReg = R_F;
            break;
    }

    registers[loReg] = readMemory(sp);
    sp++;
    registers[hiReg] = readMemory(sp);
    sp++;

    if(P == 3)
    {
        registers[loReg] &= 0xF0;
    }

    cycleCount += 3;
}

void CPU::opRET()
{
    uint8_t lower{0};
    uint8_t upper{0};

    lower = readMemory(sp);
    sp++;

    upper = readMemory(sp);
    sp++;

    pc = Helper::concatChar(upper, lower);

    cycleCount += 4;
}

void CPU::opRETI()
{
    opEI();
    cycleCount--;
    opRET();
}

void CPU::opJPHL()
{
    pc = Helper::concatChar(registers[R_H], registers[R_L]);
    cycleCount += 1;
}

void CPU::opLDSPHL()
{

    sp = Helper::concatChar(registers[R_H], registers[R_L]);
    cycleCount += 2;
}

void CPU::opJPccnn()
{

    if(passedCondition(YYY))
    {
        opJPnn();
    }
    else
    {
        pc += 2;
        cycleCount += 3;
    }
}

void CPU::opLDHCA()
{
    writeMemory(Helper::concatChar(0xFF, registers[R_C]), registers[R_A]);
    cycleCount += 2;
}

void CPU::opLDnnA()
{
    uint8_t lower{readMemory(pc)};
    pc++;
    uint8_t upper{readMemory(pc)};
    pc++;

    writeMemory(Helper::concatChar(upper, lower), registers[R_A]);
    cycleCount += 4;
}

void CPU::opLDHAC()
{
    uint16_t addr{Helper::concatChar(0xFF, registers[R_C])};

    registers[R_A] = readMemory(addr);

    cycleCount += 2;
}

void CPU::opLDAnn()
{
    uint8_t lower{readMemory(pc)};
    pc++;
    uint8_t upper{readMemory(pc)};
    pc++;
    uint16_t addr{Helper::concatChar(upper, lower)};

    registers[R_A] = readMemory(addr);

    cycleCount += 4;
}

void CPU::opJPnn()
{
    uint8_t lower{readMemory(pc)};
    pc++;
    uint8_t upper{readMemory(pc)};
    pc++;

    pc = Helper::concatChar(upper, lower);
    
    cycleCount += 4;
}

void CPU::opDI()
{
    IMEflag = 0;
    cycleCount += 1;
}

void CPU::opEI()
{
    prepareIME = 1;
    nextInstrExecuted = 0;
    cycleCount += 1;
}

void CPU::opCALLccnn()
{
    if(passedCondition(YYY))
    {
        opCALLnn();
    }
    else
    {
        pc += 2;
        cycleCount += 3;
    }
}

/* Put value from register onto the stack */
void CPU::opPUSHrp2()
{
    uint8_t hiReg{0};
    uint8_t loReg{0};

    switch(P)
    {
        case 0: // BC
            hiReg = R_B;
            loReg = R_C;
            break;
        case 1: // DE
            hiReg = R_D;
            loReg = R_E;
            break;
        case 2: // HL
            hiReg = R_H;
            loReg = R_L;
            break;
        case 3: // AF
            hiReg = R_A;
            loReg = R_F;
            break;
    }

    sp--;
    writeMemory(sp, registers[hiReg]);

    sp--;
    writeMemory(sp, registers[loReg]);

    cycleCount += 4;
}

/* Put instruction after CALL onto stack, then jump to n16 (n16 value stored first)*/
void CPU::opCALLnn()
{

    uint8_t lowJP{readMemory(pc)};
    pc++;
    uint8_t highJP{readMemory(pc)};
    pc++;

    // move sp keeping little endianness in mind
    sp--;
    
    // write high to sp location
    writeMemory(sp, Helper::hiBits(pc));

    sp--;
    // write low
    writeMemory(sp, Helper::loBits(pc));

    pc = Helper::concatChar(highJP, lowJP);


    cycleCount += 6;
}

/* Add immediate into A (also covers ADC) */
void CPU::opADDAn()
{
    uint8_t operand{readMemory(pc)};
    pc++;
    uint8_t regVal{registers[R_A]};
    bool carry{false};

    if(YYY == 1) // add carry bit to operand
    {
        if(Helper::getBit(registers[R_F], F_C) == 1)
        {
            carry = true;
        }
    }

    registers[R_A] += operand + carry;

    if((regVal & 0xF) + (operand & 0xF) + (uint8_t)carry > 0x0F)
    {
        Helper::setBit(registers[R_F], F_H);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_H);
    }

    if((uint16_t)regVal + (uint16_t)operand + (uint8_t)carry > 0xFF)
    {
        Helper::setBit(registers[R_F], F_C);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_C);
    }

    if((uint8_t)(regVal + operand + (uint8_t)carry) == 0)
    {
        Helper::setBit(registers[R_F], F_Z);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_Z);

    }
    Helper::resetBit(registers[R_F], F_N);
    cycleCount += 2;
}

void CPU::opSUBAn()
{
    uint8_t operand{readMemory(pc)};
    pc++;
    uint8_t regVal{registers[R_A]};

    registers[R_A] -= operand;

    if ((regVal - operand) == 0)
    {
        Helper::setBit(registers[R_F], F_Z);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_Z);
    }

    Helper::setBit(registers[R_F], F_N);

    uint8_t minuNib{(regVal & 0b1111)};
    uint8_t subtraNib{(operand & 0b1111)};

    if(subtraNib > minuNib)
    {
        Helper::setBit(registers[R_F], F_H);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_H);
    }

    if(operand > regVal)
    {
        Helper::setBit(registers[R_F], F_C);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_C);
    }
    cycleCount += 2;
    
}

void CPU::opSBCAn()
{
    int16_t operand = (int16_t)readMemory(pc) & 0xFF;
    pc++;
    int16_t aReg = (int16_t)registers[R_A] & 0xFF;
    int aTemp = aReg;

    aTemp -= operand;

    if (Helper::getBit(registers[R_F], F_C))
    {
        aTemp -= 1;
    }

    Helper::setBit(registers[R_F], F_N);

    if(aTemp < 0)
    {
        Helper::setBit(registers[R_F], F_C);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_C);
    }

    aTemp &= 0xFF;

    if(aTemp == 0)
    {
        Helper::setBit(registers[R_F], F_Z);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_Z);
    }

    if (((aTemp ^ operand ^ aReg) & 0x10) == 0x10)
    {
        Helper::setBit(registers[R_F], F_H);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_H);
    }

    registers[R_A] = aTemp;
    cycleCount += 2;
}
            
void CPU::opANDAn()
{
    uint8_t operand{readMemory(pc)};
    pc++;

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

    cycleCount += 2;

}
            
void CPU::opXORAn()
{
    uint8_t operand{readMemory(pc)};
    pc++;

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

    cycleCount += 2;
}
            
void CPU::opORAn()
{
    uint8_t operand{readMemory(pc)};
    pc++;

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

    cycleCount += 2;
}
            
void CPU::opCPAn()
{
    uint8_t operand{readMemory(pc)};
    pc++;

    if((registers[R_A] - operand) == 0)
    {
        Helper::setBit(registers[R_F], F_Z);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_Z);
    }

    Helper::setBit(registers[R_F], F_N);

    uint8_t minuNib{(registers[R_A] & 0b1111)};
    uint8_t subtraNib{(operand & 0b1111)};

    if(subtraNib > minuNib)
    {
        Helper::setBit(registers[R_F], F_H);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_H);
    }

    if(operand > registers[R_A])
    {
        Helper::setBit(registers[R_F], F_C);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_C);
    }

    cycleCount += 2;
}

void CPU::opRST()
{

    // move sp keeping little endianness in mind
    sp--;
    
    // write high to sp location
    writeMemory(sp, Helper::hiBits(pc));

    sp--;
    // write low
    writeMemory(sp, Helper::loBits(pc));

    pc = Helper::concatChar(0x00, YYY*8);


    cycleCount += 4;
}

/* BEGIN CB PREFIX OPCODES */

void CPU::opROT()
{
    uint8_t cycleTime{2};
    uint8_t regIndex{ZZZ};
    uint8_t data{0};
    uint8_t returnValue{0};
    
    if(regIndex == R_HL)
    {
        // All rotational opcodes take an additional 2 m-cycles
        cycleTime += 2;

        data = readMemory(Helper::concatChar(registers[R_H], registers[R_L]));
    }
    else
    {
        data = registers[regIndex];
    }
    
    switch(YYY)
    {
        case 0:
            returnValue = opRLCr(data);
            break;
        case 1:
            returnValue = opRRCr(data);
            break;
        case 2:
            returnValue = opRLr(data);
            break;
        case 3:
            returnValue = opRRr(data);
            break;
        case 4:
            returnValue = opSLAr(data);
            break;
        case 5:
            returnValue = opSRAr(data);
            break;
        case 6:
            returnValue = opSWAPr(data);
            Helper::resetBit(registers[R_F], F_C);
            break;
        case 7:
            returnValue = opSRLr(data);
            break;
    }

    if(regIndex == R_HL)
    {
        writeMemory(Helper::concatChar(registers[R_H], registers[R_L]), returnValue);
    }
    else
    {
        if(regIndex != R_F)
        {
            registers[regIndex] = returnValue;
        }
        
    }

    Helper::resetBit(registers[R_F], F_N);
    Helper::resetBit(registers[R_F], F_H);

    if(returnValue == 0)
    {
        Helper::setBit(registers[R_F], F_Z);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_Z);
    }

    cycleCount += cycleTime;
}

uint8_t CPU::opRLCr(uint8_t data)
{
    uint8_t MSB{Helper::getBit(data, 7)};
    uint8_t temp{data};

    temp = (temp << 1) | MSB;

    if(MSB)
    {
        Helper::setBit(registers[R_F], F_C);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_C);
    }

    return temp;

}

uint8_t CPU::opRRCr(uint8_t data)
{
    uint8_t LSB{Helper::getBit(data, 0)};
    uint8_t temp{data};

    temp = (temp >> 1) | (LSB << 7);

    if(LSB)
    {
        Helper::setBit(registers[R_F], F_C);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_C);
    }

    return temp;
}

uint8_t CPU::opRLr(uint8_t data)
{
    uint8_t MSB{Helper::getBit(data, 7)};
    uint8_t cFlag{Helper::getBit(registers[R_F], F_C)};

    data = (data << 1) | cFlag;

    if(MSB)
    {
        Helper::setBit(registers[R_F], F_C);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_C);
    }

    return data;
}

uint8_t CPU::opRRr(uint8_t data)
{
    uint8_t LSB{Helper::getBit(data, 0)};
    uint8_t cFlag{Helper::getBit(registers[R_F], F_C)};

    data = (data >> 1) | (cFlag << 7);

    if(LSB)
    {
        Helper::setBit(registers[R_F], F_C);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_C);
    }

    return data;
}

uint8_t CPU::opSLAr(uint8_t data)
{
    uint8_t MSB{Helper::getBit(data, 7)};

    data = (data << 1);

    if(MSB)
    {
        Helper::setBit(registers[R_F], F_C);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_C);
    }

    return data;
}

uint8_t CPU::opSRAr(uint8_t data)
{
    uint8_t LSB{Helper::getBit(data, 0)};
    uint8_t MSB{Helper::getBit(data, 7)};

    data = (data >> 1) | (MSB << 7);

    if(LSB)
    {
        Helper::setBit(registers[R_F], F_C);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_C);
    }

    return data;
}

uint8_t CPU::opSWAPr(uint8_t data)
{
    uint8_t newUpper{data & 0x0F};
    uint8_t newLower{data & 0xF0};

    newUpper <<= 4;
    newLower >>= 4;

    return (newUpper | newLower); 
    
}

uint8_t CPU::opSRLr(uint8_t data)
{
    uint8_t LSB{Helper::getBit(data, 0)};

    data = (data >> 1);

    if(LSB)
    {
        Helper::setBit(registers[R_F], F_C);
    }
    else
    {
        Helper::resetBit(registers[R_F], F_C);
    }

    return data;
}

void CPU::opBIT()
{
    uint8_t bitPosition{YYY};
    uint8_t regIndex{ZZZ};
    uint8_t data{0};
    uint8_t cycleTime{2};

    if(regIndex == 6)
    {
        data = readMemory(Helper::concatChar(registers[R_H], registers[R_L]));
        cycleTime += 1;
    }
    else
    {
        data = registers[ZZZ];
    }

    if(Helper::getBit(data, bitPosition))
    {
        Helper::resetBit(registers[R_F], F_Z);
    }
    else
    {
        Helper::setBit(registers[R_F], F_Z);
    }

    Helper::resetBit(registers[R_F], F_N);
    Helper::setBit(registers[R_F], F_H);

    cycleCount += cycleTime;
}

void CPU::opRES()
{
    uint8_t bitPosition{YYY};
    uint8_t regIndex{ZZZ};
    uint8_t data{0};
    uint8_t cycleTime{2};

    if(regIndex == 6)
    {
        data = readMemory(Helper::concatChar(registers[R_H], registers[R_L]));
        Helper::resetBit(data, bitPosition);
        writeMemory(Helper::concatChar(registers[R_H], registers[R_L]), data);
        cycleTime += 2;
    }
    else
    {
        Helper::resetBit(registers[ZZZ], bitPosition);
    }


    cycleCount = cycleTime;
}

void CPU::opSET()
{
    uint8_t bitPosition{YYY};
    uint8_t regIndex{ZZZ};
    uint8_t data{0};
    uint8_t cycleTime{2};

    if(regIndex == 6)
    {
        data = readMemory(Helper::concatChar(registers[R_H], registers[R_L]));
        Helper::setBit(data, bitPosition);
        writeMemory(Helper::concatChar(registers[R_H], registers[R_L]), data);
        cycleTime += 2;
    }
    else
    {
        Helper::setBit(registers[ZZZ], bitPosition);
    }

    

    cycleCount += cycleTime;
}
