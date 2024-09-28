#ifndef TIMERS_H
#define TIMERS_H

#include <cstdint>
#include <vector>

/* TIMER REGISTER LOCATIONS */
constexpr uint16_t DIV_LOC = 0xFF04;
constexpr uint16_t TIMA_LOC = 0xFF05;
constexpr uint16_t TMA_LOC = 0xFF06;
constexpr uint16_t TAC_LOC = 0xFF07;

class Timers
{
    public:
        Timers();
        ~Timers();

        bool reqInterrupt;

    private:
        uint16_t curCycles;
        uint16_t divCycles;
        uint16_t timeCycles;

        uint16_t timaMaxTime;

        void cycleDivReg(std::vector<uint8_t>& memMap);
        void cycleTIMA(std::vector<uint8_t>& memMap);

    public:
        void tickTimer(uint16_t thisCycles, std::vector<uint8_t>& memMap);
        uint16_t getTimeCycles();
};

#endif