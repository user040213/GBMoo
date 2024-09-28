
#include "CPU.hpp"

void CPU::debugLog(std::ostream& logFile)
{
    logFile << "A:"; 
            if(registers[R_A] == 0)
            {
                logFile << "00";
            }
            else
            {
                logFile << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)registers[R_A];
            }
            logFile << " F:"; 
            if(registers[R_F] == 0)
            {
                logFile << "00";
            }
            else
            {
                logFile << std::hex << std::uppercase << std::setfill('0') << std::setw(2) <<(int)registers[R_F];
            }
            logFile << " B:"; 
            if(registers[R_B] == 0)
            {
                logFile << "00";
            }
            else
            {
                logFile << std::hex << std::uppercase << std::setfill('0') << std::setw(2) <<(int)registers[R_B];
            }
            logFile << " C:"; 
            if(registers[R_C] == 0)
            {
                logFile << "00";
            }
            else
            {
                logFile << std::hex << std::uppercase << std::setfill('0') << std::setw(2) <<(int)registers[R_C];
            }
            logFile << " D:"; 
            if(registers[R_D] == 0)
            {
                logFile << "00";
            }
            else
            {
                logFile << std::hex << std::uppercase << std::setfill('0') << std::setw(2) <<(int)registers[R_D];
            }
            logFile << " E:"; 
            if(registers[R_E] == 0)
            {
                logFile << "00";
            }
            else
            {
                logFile << std::hex << std::uppercase << std::setfill('0') << std::setw(2) <<(int)registers[R_E];
            }
            logFile << " H:"; 
            if(registers[R_H] == 0)
            {
                logFile << "00";
            }
            else
            {
                logFile << std::hex << std::uppercase << std::setfill('0') << std::setw(2) <<(int)registers[R_H];
            }
            logFile << " L:"; 
            if(registers[R_L] == 0)
            {
                logFile << "00";
            }
            else
            {
                logFile << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)registers[R_L];
            }
            logFile << std::setw(4) << " SP:";
            if(sp == 0)
            {
                logFile << "0000";
            }
            else
            {
                logFile << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << sp;
            }
            logFile << " PC:";
            if(pc == 0)
            {
                logFile << "0000";
            }
            else
            {
                logFile << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << pc;
            }
            logFile << " PCMEM:";
            for(int i{0}; i < 4; i++)
            {
                if(memMap[pc + i] == 0)
                {
                    logFile << "00";
                }
                else
                {
                    logFile << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)memMap[pc + i];
                }
                if(i != 3)
                {
                    logFile << ",";
                }
            }
            logFile << ")\n";
            //logFile << " IME: " << IMEflag << " IE: " << (int)memMap[0xFFFF] << " IF: " << (int)memMap[0xFF0F];
            //logFile << " TIMA: " << (int)memMap[TIMA_LOC] << " TAC: " << (int)memMap[TAC_LOC] << " TIMA Cyc: " << mTimerControl.getTimeCycles() << "\n";
}