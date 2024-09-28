#include "CPU.hpp"

CPU::CPU()
{
    isHalted = false;
    prepareIME = false;
    IMEflag = false;
    nextInstrExecuted = false;
    lastIReqs = 0;

    mPPU = PPU();
    mTimerControl = Timers();
    mJoypad = Joypad();

    // Register resetting
    memset(registers, 0, 8);

    // Initialize memory map
    memMap = std::vector<uint8_t>(0x10000, 0);

    maxRAMBanks = 1;
    maxROMBanks = 2;
    curROMBank = 1;
    curRAMBank = 1;

    enableRAM = false;
    doingROMBanking = true;

    // Reset cycle count
    cycleCount = 0;
    totalCycleCount = 0;

    // Resetting opcode decode variables
    opcode = 0;

    XX = 0;
    YYY = 0;
    ZZZ = 0;

    P = 0;
    Q = 0;

    // Boot values
    sp = 0xFFFE;
    pc = 0x100;
    
    registers[R_L] = 0x4D;
    registers[R_H] = 0x01;
    registers[R_E] = 0xD8;
    registers[R_D] = 0x00;
    registers[R_C] = 0x13;
    registers[R_B] = 0x00;
    registers[R_F] = 0xB0;
    registers[R_A] = 0x01;


    memMap[0xFF00] = 0xFF;
    memMap[0xFF01] = 0x00;
    memMap[0xFF02] = 0x7E;
    memMap[0xFF04] = 0x18;
    memMap[0xFF05] = 0x00;
    memMap[0xFF06] = 0x00;
    memMap[0xFF07] = 0xf8;
    memMap[0xFF0F] = 0xE1;
    memMap[0xFF10] = 0x80;
    memMap[0xFF11] = 0xBF;
    memMap[0xFF12] = 0xF3;
    memMap[0xFF13] = 0xFF;
    memMap[0xFF14] = 0xBF;
    memMap[0xFF16] = 0x3F;
    memMap[0xFF17] = 0x00;
    memMap[0xFF18] = 0xFF;
    memMap[0xFF19] = 0xBF;
    memMap[0xFF1A] = 0x7F;
    memMap[0xFF1B] = 0xFF;
    memMap[0xFF1C] = 0x9F;
    memMap[0xFF1D] = 0xFF;
    memMap[0xFF1E] = 0xBF;
    memMap[0xFF20] = 0xFF;
    memMap[0xFF21] = 0x00;
    memMap[0xFF22] = 0x00;
    memMap[0xFF23] = 0xBF;
    memMap[0xFF24] = 0x77;
    memMap[0xFF25] = 0xF3;
    memMap[0xFF26] = 0xF1;
    memMap[0xFF40] = 0x91;
    memMap[0xFF41] = 0x81;
    memMap[0xFF42] = 0x00;
    memMap[0xFF43] = 0x00;
    memMap[0xFF44] = 0x00;
    memMap[0xFF45] = 0x00;
    memMap[0xFF46] = 0xFF;
    memMap[0xFF47] = 0xFC;
    memMap[0xFF4A] = 0x00;
    memMap[0xFF4B] = 0x00;
    memMap[0xFFFF] = 0x00;



}

CPU::~CPU()
{

}

uint8_t* CPU::getPPUArray()
{
    return mPPU.mPixelArray.data();
}
