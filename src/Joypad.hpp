#ifndef JOYPAD_H
#define JOYPAD_H

#include <cstdint>
#include <vector>

class Joypad
{
    public:
        Joypad();
        ~Joypad();

        uint8_t getJoypadState(const std::vector<uint8_t> &memMap);
        void setJoypadState(bool dPad, uint8_t input, std::vector<uint8_t> &memMap);
        void resetJoypadState(bool dPad, uint8_t input);
        bool reqInterrupt;

    private:
        // 1 is dPad, 0 is buttons
        uint8_t mJoypadArray[2];
};

#endif