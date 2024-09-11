#include "Helper.hpp"

uint16_t Helper::concatChar(uint8_t MSB, uint8_t LSB)
{
    uint16_t returnVal{MSB};
    returnVal = (returnVal << 8) | LSB;
    return returnVal;
}

uint8_t Helper::getBit(uint8_t data, uint8_t position)
{
    uint8_t returnValue{(data >> position) & 0b1};

    return returnValue;
}

uint16_t Helper::getBit(uint16_t data, uint8_t position)
{
    uint16_t returnValue{(data >> position) & 0b1};

    return returnValue;
}

void Helper::setBit(uint8_t &data, uint8_t position)
{
    uint8_t temp{data | (0b1 << position)};
    data = temp;
}

void Helper::resetBit(uint8_t &data, uint8_t position)
{
    uint8_t temp{data & (~(0b1 << position))};
    data = temp;
}

uint8_t Helper::hiBits(uint16_t data)
{
    return ((data & 0xFF00) >> 8);
}

uint8_t Helper::loBits(uint16_t data)
{
    return (data & 0x00FF);
}