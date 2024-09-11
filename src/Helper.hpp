#ifndef HELPER_H
#define HELPER_H

#include <cstdint>

/* A bunch of helper functions */
class Helper
{
    public:
        // Combining registers or other 8 bit data to 16
        static uint16_t concatChar(uint8_t MSB, uint8_t LSB);
        
        // gets bit at position
        static uint8_t getBit(uint8_t data, uint8_t position);
        static uint16_t getBit(uint16_t data, uint8_t position);

        // set bit at position
        static void setBit(uint8_t &data, uint8_t position);

        // reset bit at position
        static void resetBit(uint8_t &data, uint8_t position);

        // get upper 8 bits
        static uint8_t hiBits(uint16_t data);

        // get lower 8 bits
        static uint8_t loBits(uint16_t data);



};



#endif