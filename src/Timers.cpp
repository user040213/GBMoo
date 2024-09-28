#include "Timers.hpp"
#include "Helper.hpp"
#include <iostream>

Timers::Timers()
{
    curCycles = 0;
    divCycles = 0;
    timeCycles = 0;
    timaMaxTime = 256;
}

Timers::~Timers()
{

}

void Timers::cycleDivReg(std::vector<uint8_t> &memMap)
{
    // 16384 Hz cycle
    // at 4.194304 MHz means one tick every 256 clocks
    divCycles += curCycles * 4;
    if(divCycles >= 256)
    {
        divCycles -= 256;
        memMap[DIV_LOC]++;
    }

}

void Timers::cycleTIMA(std::vector<uint8_t> &memMap)
{
    uint8_t cycleSelect{memMap[TAC_LOC] & 0b11};
    uint16_t newMax{0};
    switch(cycleSelect)
    {
        case 0:
            newMax = 256;
            break;
        case 1:
            newMax = 4;
            break;
        case 2:
            newMax = 16;
            break;
        case 3:
            newMax = 64;
            break;

    }
    if(newMax != timaMaxTime)
    {
        timeCycles = 0;
        timaMaxTime = newMax;
    }

    timeCycles += curCycles;
    if(timeCycles >= timaMaxTime)
    {
        timeCycles -= timaMaxTime;
        if(memMap[TIMA_LOC] == 0xFF)
        {
            uint8_t resetVal{memMap[TMA_LOC]};
            memMap[TIMA_LOC] = resetVal;
            // request interrupt
            reqInterrupt = true;
        }
        else
        {
            memMap[TIMA_LOC]++;
        }
        
    }

}

void Timers::tickTimer(uint16_t thisCycles, std::vector<uint8_t> &memMap)
{
    curCycles = thisCycles;

    // Div always ticks regardless of TAC values
    cycleDivReg(memMap);

    bool clockEn{Helper::getBit(memMap[TAC_LOC], 2)};
    if(clockEn)
    {
        cycleTIMA(memMap);
    }
}

uint16_t Timers::getTimeCycles()
{
    return timeCycles;
}