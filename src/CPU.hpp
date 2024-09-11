#ifndef CPU_H
#define CPU_H

#include <cstdint>
#include <vector>
#include <iostream>
#include <fstream>
#include <chrono>
#include <filesystem>
#include <cstring>

#include "Helper.hpp"


/* UNCONFIGURABLE SPECS */
constexpr uint32_t MEM_MAP_SIZE = 0x10000;
constexpr uint8_t DISPLAY_WIDTH = 144;
constexpr uint8_t DISPLAY_HEIGHT = 160;

/* CONFIGURABLE SPECS */
constexpr uint32_t CLOCK_SPEED = 0;

/* FLAG REGISTER */
constexpr uint8_t F_Z = 7;
constexpr uint8_t F_N = 6;
constexpr uint8_t F_H = 5;
constexpr uint8_t F_C = 4;
// bits 3 to 0 unused

/* CONDITIONAL EXPRESSIONS */
constexpr uint8_t F_NO_Z = 0;
constexpr uint8_t F_YES_Z = 1;
constexpr uint8_t F_NO_C = 2;
constexpr uint8_t F_YES_C = 3;

/* REGISTER INDICES */
constexpr uint8_t R_B = 0;
constexpr uint8_t R_C = 1;
constexpr uint8_t R_D = 2;
constexpr uint8_t R_E = 3;
constexpr uint8_t R_H = 4;
constexpr uint8_t R_L = 5;
constexpr uint8_t R_HL = 6; // this is unique it refers to data in address pointed to by HL
constexpr uint8_t R_A = 7;

constexpr uint8_t R_F = 6; // register index 6 will be for F register

/* MEMORY MAP START VALUES */
constexpr uint16_t ROM_00 = 0x0000;
constexpr uint16_t ROM_NN = 0x4000;
constexpr uint16_t VRAM = 0x8000;
constexpr uint16_t EXT_RAM = 0xA000;
constexpr uint16_t WRAM_0 = 0xC000;
constexpr uint16_t WRAM_N = 0xD000;
constexpr uint16_t ECHO_RAM = 0xE000;
constexpr uint16_t SPRITE_TABLE = 0xFE00;
constexpr uint16_t HRAM = 0xFF80;

class CPU
{

    /* VARIABLES */
    public:
        // All CPU registers
        uint8_t registers[8];
        uint16_t sp;
        uint16_t pc;

        // Memory Map
        std::vector<uint8_t> memMap;

        // CPU cycle tracker (m-cycles)
        uint8_t cycleCount;

        // Interrupt flag
        bool IMEflag;
    private:

        // Opcode Decoding (In Octal)
        uint8_t opcode;

        uint8_t XX;     // bits 7-6
        uint8_t YYY;   // bits 5-3
        uint8_t ZZZ;   // bits 2-0

        uint8_t P;      // bits 5-4
        uint8_t Q;      // bit 3

    /* FUNCTIONS */
    public:
        // Constructor
        CPU();

        // CPU Read and Write
        uint8_t readMemory(uint16_t addr);
        void writeMemory(uint16_t addr, uint8_t data);

        /* BEGIN: Definitions in Opcode.cpp */
        // Opcode Decode and Execution
        void decode();
        void executeOp();

        /* OPCODES (format opXXYYYZZZ) */
        // Opcode helpers
        bool passedCondition(uint8_t condition);
        void add16Bit(uint16_t operand1, uint16_t operand2); // only sets flags
        void add8Bit(uint8_t operand1, uint8_t operand2); // only sets flags
        void sub8Bit(uint8_t minuend, uint8_t subtrahend); // only sets flags
        // used to preserve carry flags
        void inc8Bit(uint8_t data);
        void dec8Bit(uint8_t data);

        // XX = 0 (00)
            // ZZZ = 0
                // YYY = 0
                void opNOP();
                // YYY = 1
                void opLDnnSP();
                // YYY = 2
                void opSTOP();
                // YYY = 3
                void opJRd();
                // YYY = 4-7
                void opJRccd();
            // ZZZ = 1
                // Q = 0
                void opLDrpnn();
                // Q = 1
                void opADDHLrp();
            // ZZZ = 2
                // Q = 0
                    // P = 0-1
                    void opLDrrA();
                    
                    // P = 2-3
                    void opLDHLIDA();
                    
                // Q = 1
                    // P = 0-1
                    void opLDArr();
                    
                    // P = 2
                    void opLDAHLID();

            // ZZZ = 3
                // Q = 0-1
                void opINCDECrp();
            // ZZZ = 4-5
                void opINCDECr();
            // ZZZ = 6
                void opLDrn();
            // ZZZ = 7
                // YYY = 0
                    void opRLCA();
                // YYY = 1
                    void opRRCA();
                // YYY = 2
                    void opRLA();
                // YYY = 3
                    void opRRA();
                // YYY = 4
                    void opDAA();
                // YYY = 5
                    void opCPL();
                // YYY = 6
                    void opSCF();
                // YYY = 7
                    void opCCF();
        
        // XX = 1 (01)
            // ZZZ = 6 AND YYY = 6
            void opHALT();
            // ELSE
            void opLDrr();
        
        // XX = 2 (10) (ALU using register)
            // YYY = 0-1
            void opADDAr();
            // YYY = 2-3
            void opSUBAr();
            // YYY = 4
            void opANDAr();
            // YYY = 5
            void opXORAr();
            // YYY = 6
            void opORAr();
            // YYY = 7
            void opCPAr();

       
        // XX = 3 (11)
            // ZZZ = 0
                // YYY = 0-3
                void opRETcc();
                // YYY = 4
                void opLDoffnA();
                // YYY = 5
                void opADDSPd();
                // YYY = 6
                void opLDAoffn();
                // YYY = 7
                void opLDHLSPId();
            // ZZZ = 1
                // Q = 0
                    void opPOPrp2();
                // Q = 1
                    // P = 0
                    void opRET();
                    // P = 1
                    void opRETI();
                    // P = 2
                    void opJPHL();
                    // P = 3
                    void opLDSPHL();
            // ZZZ = 2
                // YYY = 0-3
                void opJPccnn();
                // YYY = 4
                void opLDoffCA();
                // YYY = 5
                void opLDnnA();
                // YYY = 6
                void opLDAoffC();
                // YYY = 7
                void opLDAnn();
            // ZZZ = 3
                // YYY = 0
                void opJPnn();
                // YYY = 1 (NONE; goes to CB prefix)
                // YYY = 2 (NONE)
                // YYY = 3 (NONE)
                // YYY = 4 (NONE)
                // YYY = 5 (NONE)
                // YYY = 6
                void opDI();
                // YYY = 7
                void opEI();
            // ZZZ = 4
                // YYY = 0-3
                void opCALLccnn();
                // YYY = 4-7 (NONE)
            // ZZZ = 5
                // Q = 0
                    void opPUSHrp2();
                // Q = 1
                    // P = 0
                    void opCALLnn();
                    // P = 1-3 (NONE)
            // ZZZ = 6
                void opALUn(); // ALU using immediate
            // ZZZ = 7
                void opRST();
        
        // CB Prefix Opcodes
            // XX = 0 (00)
            void opROT();
            
            // XX = 1 (01)
            void opBIT();
            
            // XX = 2 (10)
            void opRES();
            
            // XX = 3 (11)
            void opSET();
        /* END of definitions in Opcode.cpp */


        
};

#endif