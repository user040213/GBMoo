#include "CPU.hpp"
#include <sstream>

/* 
    Try to emulate 59.7 Hz refresh rate (to be hardware accurate)
    Some Math: 4194304 Hz / 59.7 Hz is around 70256 cycles per frame (rounded) 
    Since we counted using m-cycles, which is approximately a quarter of a cycle, we should multiply by 4 (t cycles)
*/
constexpr int32_t CYCLES_PER_FRAME = 70256; 
constexpr bool DEBUG = true;

std::ofstream logFile("GBMoo.log");
void CPU::runCPU()
{
    
    while(totalCycleCount < CYCLES_PER_FRAME)
    {   
        if(DEBUG)
        {
            debugLog(logFile);
        }

        if(prepareIME)
        {
            nextInstrExecuted = 1;
        }
        if(!isHalted)
        {
            opcode = readMemory(pc);
            pc++;
            decode();
            executeOp();

        }
        else
        {
            opNOP();
        }

        if(nextInstrExecuted && prepareIME)
        {
            IMEflag = 1;
            prepareIME = 0;
            nextInstrExecuted = 0;
        }

        mPPU.PPUCycle(cycleCount, memMap);
        mTimerControl.tickTimer(cycleCount, memMap);
        totalCycleCount += cycleCount * 4;
        handleInterrupt();
        cycleCount = 0;
        
    }

    totalCycleCount -= CYCLES_PER_FRAME;
}

void CPU::handleInterrupt()
{
    if(mPPU.reqLCDInterrupt)
    {
        isHalted = false;
        mPPU.reqLCDInterrupt = false;
        Helper::setBit(memMap[0xFF0F], 1);

    }
    if(mPPU.reqVBInterrupt)
    {
        isHalted = false;
        mPPU.reqVBInterrupt = false;
        Helper::setBit(memMap[0xFF0F], 0);
    }
    if(mTimerControl.reqInterrupt)
    {
        isHalted = false;
        mTimerControl.reqInterrupt = false;
        Helper::setBit(memMap[0xFF0F], 2);
    }
    if(mJoypad.reqInterrupt)
    {
        isHalted = false;
        mTimerControl.reqInterrupt = false;
        Helper::setBit(memMap[0xFF0F], 4);
    }
    if(IMEflag)
    {   
        isHalted = false;
        for(uint8_t i{0}; i < 5; i++)
        {
            uint8_t interruptEnable{Helper::getBit(memMap[0xFFFF], i)};
            uint8_t interruptReq{Helper::getBit(memMap[0xFF0F], i)};
            if((interruptEnable & interruptReq) > 0)
            {
                isHalted = false;
                Helper::resetBit(memMap[0xFF0F], i);
                IMEflag = false;
                sp--;
                uint8_t loPC{Helper::loBits(pc)};
                uint8_t hiPC{Helper::hiBits(pc)};
                writeMemory(sp, hiPC);
                sp--;
                writeMemory(sp, loPC);

                switch(i)
                {
                    case 0:
                        pc = 0x40;
                        break;
                    case 1:
                        pc = 0x48;
                        break;
                    case 2:
                        pc = 0x50;
                        break;
                    case 3:
                        pc = 0x58;
                        break;
                    case 4:
                        pc = 0x60;
                        break;
                }
                
            }
        }
    }
}

void CPU::handleJoypadInput(SDL_Scancode inputIndex, bool pressed)
{
    uint8_t i{0};
    bool dPad{false};
    switch(inputIndex)
    {
        case SDL_SCANCODE_D:
            dPad = true;
            i = 0;
            break;
        case SDL_SCANCODE_A:
            dPad = true;
            i = 1;
            break;
        case SDL_SCANCODE_W:
            dPad = true;
            i = 2;
            break;
        case SDL_SCANCODE_S:
            dPad = true;
            i = 3;
            break;
        case SDL_SCANCODE_G:
            i = 0;
            break;
        case SDL_SCANCODE_F:
            i = 1;
            break;
        case SDL_SCANCODE_SPACE:
            i = 2;
            break;
        case SDL_SCANCODE_RETURN:
            i = 3;
            break;
        default:
            break;
    }
    if (pressed)
    {
        mJoypad.setJoypadState(dPad, i, memMap);
    }
    else
    {
        mJoypad.resetJoypadState(dPad, i);
    }

}

void CPU::executeOp()
{
    // do CB prefix instead
    if(opcode == 0xCB)
    {
        opcode = readMemory(pc);
        pc++;
        decode();
        switch(XX)
        {
            case 0:
                opROT();
                break;
            case 1:
                opBIT();
                break;
            case 2:
                opRES();
                break;
            case 3:
                opSET();
                break;
        }

        return;
    }

    if(XX == 0)
    {
        switch(ZZZ)
        {
            case 0:
                x0z0Decode();
                break;
            case 1:
                switch(Q)
                {
                    case 0:
                        opLDrpnn();
                        break;
                    case 1:
                        opADDHLrp();
                        break;
                }
                break;
            case 2:
                x0z2Decode();
                break;
            case 3:
                opINCDECrp();
                break;
            case 4:
            case 5:
                opINCDECr();
                break;
            case 6:
                opLDrn();
                break;
            case 7:
                x0z7Decode();
                break;
        }
    }
    else if(XX == 1)
    {
        if(ZZZ == 6 && YYY == 6)
        {
            opHALT();
        }
        else
        {
            opLDrr();
        } 
        
    }
    else if(XX == 2)
    {
        switch(YYY)
        {
            case 0:
            case 1:
                opADDAr();
                break;
            case 2:
                opSUBAr();
                break;
            case 3:
                opSBCAr();
                break;
            case 4:
                opANDAr();
                break;
            case 5:
                opXORAr();
                break;
            case 6:
                opORAr();
                break;
            case 7:
                opCPAr();
                break;
        }     
    }
    else
    {
        switch(ZZZ)
        {
            case 0:
                x3z0Decode();
                break;
            case 1:
                x3z1Decode();
                break;
            case 2:
                x3z2Decode();
                break;
            case 3:
                switch(YYY)
                {
                    case 0:
                        opJPnn();
                        break;
                    case 6:
                        opDI();
                        break;
                    case 7:
                        opEI();
                        break;
                }
                break;
            case 4:
                switch(YYY)
                {
                    case 0:
                    case 1:
                    case 2:
                    case 3:
                        opCALLccnn();
                        break;
                }
                break;
            case 5:
                switch(Q)
                {
                    case 0:
                        opPUSHrp2();
                        break;
                    case 1:
                        if(P == 0){opCALLnn();}
                        break;
                }
                break;
            case 6:
                switch(YYY)
                {
                    case 0:
                    case 1:
                        opADDAn();
                        break;
                    case 2:
                        opSUBAn();
                        break;
                    case 3:
                        opSBCAn();
                        break;
                    case 4:
                        opANDAn();
                        break;
                    case 5:
                        opXORAn();
                        break;
                    case 6:
                        opORAn();
                        break;
                    case 7:
                        opCPAn();
                        break;
                }
                break;
            case 7:
                opRST();
                break;
        }
        
    }
}

void CPU::x0z0Decode()
{
    switch(YYY)
    {
        case 0:
            opNOP();
            break;
        case 1:
            opLDnnSP();
            break;
        case 2:
            opSTOP();
            break;
        case 3:
            opJRd();
            break;
        case 4:
        case 5:
        case 6:
        case 7:
            opJRccd();
            break;
    }
}
void CPU::x0z2Decode()
{
    if(Q == 0)
    {
        switch(P)
        {
            case 0:
            case 1:
                opLDrrA();
                break;
            case 2:
            case 3:
                opLDHLIDA();
                break;  
        }
    }
    else
    {
        switch(P)
        {
            case 0:
            case 1:
                opLDArr();
                break;
            case 2:
            case 3:
                opLDAHLID();
                break;
        }
    }
}
void CPU::x0z7Decode()
{
    switch(YYY)
    {
        case 0:
            opRLCA();
            break;
        case 1:
            opRRCA();
            break;
        case 2:
            opRLA();
            break;
        case 3:
            opRRA();
            break;
        case 4:
            opDAA();
            break;
        case 5:
            opCPL();
            break;
        case 6:
            opSCF();
            break;
        case 7:
            opCCF();
            break;
    }
}
void CPU::x3z0Decode()
{
    switch(YYY)
    {
        case 0:
        case 1:
        case 2:
        case 3:
            opRETcc();
            break;
        case 4:
            opLDHnA();
            break;
        case 5:
            opADDSPd();
            break;
        case 6:
            opLDHAn();
            break;
        case 7:
            opLDHLSPId();
            break;
    }
}
void CPU::x3z1Decode()
{
    if(Q == 0)
    {
        opPOPrp2();
    }
    else
    {  
        switch(P)
        {
            case 0:
                opRET();
                break;
            case 1:
                opRETI();
                break;
            case 2:
                opJPHL();
                break;
            case 3:
                opLDSPHL();
                break;
        }
    }
}
void CPU::x3z2Decode()
{
    switch(YYY)
    {
        case 0:
        case 1:
        case 2:
        case 3:
            opJPccnn();
            break;
        case 4: 
            opLDHCA();
            break;
        case 5:
            opLDnnA();
            break;
        case 6:
            opLDHAC();
            break;
        case 7:
            opLDAnn();
            break;
    }
}
