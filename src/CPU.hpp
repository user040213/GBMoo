#ifndef CPU_H
#define CPU_H

#include <cstdint>
#include <iostream>
#include <fstream>
#include <chrono>
#include <filesystem>
#include <cstring>


/* UNCONFIGURABLE SPECS */
constexpr uint32_t MEM_MAP_SIZE = 0x10000;
constexpr uint8_t DISPLAY_WIDTH = 144;
constexpr uint8_t DISPLAY_HEIGHT = 160;

/* CONFIGURABLE SPECS */
constexpr uint32_t CLOCK_SPEED;

/* FLAG REGISTER */
constexpr uint8_t F_Z = 7;
constexpr uint8_t F_N = 6;
constexpr uint8_t F_H = 5;
constexpr uint8_t F_C = 4;
// bits 3 to 0 unused

/* REGISTER INDICES */
constexpr uint8_t R_B = 0;
constexpr uint8_t R_C = 1;
constexpr uint8_t R_D = 2;
constexpr uint8_t R_E = 3;
constexpr uint8_t R_H = 4;
constexpr uint8_t R_L = 5;
constexpr uint8_t R_A = 7;

/* MEMORY MAP START VALUES */
constexpr uint8_t ROM_00 = 0x0000;
constexpr uint8_t ROM_NN = 0x4000;
constexpr uint8_t VRAM = 0x8000;
constexpr uint8_t EXT_RAM = 0xA000;
constexpr uint8_t WRAM_0 = 0xC000;
constexpr uint8_t WRAM_N = 0xD000;
constexpr uint8_t ECHO_RAM = 0xE000;
constexpr uint8_t SPRITE_TABLE = 0xFE00;
constexpr uint8_t HRAM = 0xFF80;

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

    private:
        // Opcode Decoding (In Octal)
        uint8_t opcode;

        uint8_t XX;     // bits 7-6
        uint16_t YYY;   // bits 5-3
        uint16_t ZZZ;   // bits 2-0

        uint8_t P;      // bits 5-4
        uint8_t Q;      // bit 3

    /* FUNCTIONS */
    public:
        // CPU Read and Write
        void readMemory(uint16_t addr);
        

        // Opcode Decode and Execution
        void decode();
        void executeOp();

        // Opcodes (format opXXYYYZZZ)
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
            // ZZZ = 3
            // ZZZ = 4
            // ZZZ = 5
            // ZZZ = 6
            // ZZZ = 7
        // XX = 1 (01)
            // ZZZ = 6 AND YYY = 6
            void opHALT();
            // ELSE
            void opLDrr();


        // XX = 2 (10)

        // XX = 3 (11)

        // CB Prefix Opcodes
            // XX = 0 (00)
            void opROT();
            // XX = 1 (01)
            void opBIT();
            // XX = 2 (10)
            void opRES();
            // XX = 3 (11)
            void opSET();


        
};

#endif