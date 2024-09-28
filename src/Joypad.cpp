#include "Joypad.hpp"
#include "Helper.hpp"

Joypad::Joypad()
{
    mJoypadArray[0] = 0xFF;
    mJoypadArray[1] = 0xFF;
    reqInterrupt = false;
}

Joypad::~Joypad()
{

}

uint8_t Joypad::getJoypadState(const std::vector<uint8_t> &memMap)
{
    uint8_t registerState{memMap[0xFF00]};
    uint8_t returnVal = registerState ^ 0xFF;

    // dPad
    if(!Helper::getBit(registerState, 4))
    {
        returnVal &= (mJoypadArray[1] | 0xF0);
    }
    else if(!Helper::getBit(registerState, 5))
    {
        returnVal &= (mJoypadArray[0] | 0xF0);
    }
    

    return returnVal;
}

void Joypad::setJoypadState(bool dPad, uint8_t inputIndex, std::vector<uint8_t> &memMap)
{
    uint8_t registerState{memMap[0xFF00]};
    bool goingToLow{false};

    if(Helper::getBit(mJoypadArray[dPad], inputIndex) != 0)
    {
        goingToLow = true;
        Helper::resetBit(mJoypadArray[dPad], inputIndex);
    }

    if(dPad && !(Helper::getBit(registerState, 4)))
    {
        if(goingToLow)
        {
            reqInterrupt = true;
        }
    }

    else if(!dPad && !(Helper::getBit(registerState, 5)))
    {
        if(goingToLow)
        {
            reqInterrupt = true;
        }
    }
}

void Joypad::resetJoypadState(bool dPad, uint8_t input)
{
    Helper::setBit(mJoypadArray[dPad], input);
}